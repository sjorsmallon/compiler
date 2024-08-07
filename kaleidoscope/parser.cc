#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"


#include <string>
#include <vector>
#include <memory>
#include <map>

using namespace std;
using namespace llvm; 

// The lexer returns tokens [0-255] if it is an unknown character, otherwise one
// of these for known things.
enum Token {
  TOKEN_EOF = -1,

  // commands
  TOKEN_DEF = -2,
  TOKEN_EXTERN = -3,

  // primary
  TOKEN_IDENTIFIER = -4,
  TOKEN_NUMBER = -5,
};

static string g_identifier_string; // Filled in if TOKEN_IDENTIFIER (wow, so not part of a union?)
static double g_number_value;  


//===----------------------------------------------------------------------===//
// Abstract Syntax Tree (aka Parse Tree)
//===----------------------------------------------------------------------===//

namespace {

/// expr_ast - Base class for all expression nodes.
class expr_ast
{
public:
    virtual ~expr_ast() = default;
    virtual Value* codegen() = 0; // inheritance != polymorphism :~)
};

/// number_expr_ast - Expression class for numeric literals like "1.0".
class number_expr_ast : public expr_ast
{
  double value;

public:
  number_expr_ast(double value) : value(value) {}
  Value* codegen() override;

};



/// variable_expr_ast - Expression class for referencing a variable, like "a".
class variable_expr_ast : public expr_ast
{
    std::string name;

public:
    variable_expr_ast(const std::string &name) : name(name) {}
    Value* codegen() override;
};

/// binary_expr_ast - Expression class for a binary operator.
class binary_expr_ast : public expr_ast
{
  char op;
  std::unique_ptr<expr_ast> lhs, rhs;

public:
    binary_expr_ast(char op, std::unique_ptr<expr_ast> lhs,
                std::unique_ptr<expr_ast> rhs)
      : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

    Value* codegen() override;
};

/// call_expr_ast - Expression class for function calls.
class call_expr_ast : public expr_ast
{
  std::string callee;
  std::vector<std::unique_ptr<expr_ast>> arguments;

public:
  call_expr_ast(const std::string &callee,
              std::vector<std::unique_ptr<expr_ast>> arguments)
      : callee(callee), arguments(std::move(arguments)) {}


    Value* codegen() override;
};

/// prototype_ast - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
class prototype_ast
{
  std::string name;
  std::vector<std::string> arguments;

public:
  prototype_ast(const std::string &name, std::vector<std::string> arguments)
      : name(name), arguments(std::move(arguments)) {}


    Function* codegen();
    const std::string &getName() const { return name; }
};

/// function_ast - This class represents a function definition itself.
class function_ast {
  std::unique_ptr<prototype_ast> prototype;
  std::unique_ptr<expr_ast> body;

public:
  function_ast(std::unique_ptr<prototype_ast> prototype,
              std::unique_ptr<expr_ast> body)
      : prototype(std::move(prototype)), body(std::move(body)) {}

    Function* codegen();
};

} // end anonymous namespace


//@NOTE(SJM): I hate this naming convention. what is current token vs get_token???
/// get_token - Return the next(!!!!) token from standard input. the çurrent" token is just stored in g_current_token.
// we just return an enum value, and the client knows that for specific tokens,
// they can access some properties here (e.g. g_identifier_string or g_number_value).
// that seems like an awful thing but whatever.
static int get_token()
{
    
    static int last_character = ' ';
    // Skip any whitespace.
    while (isspace(last_character))
    {
     last_character = getchar();
    }

    if (isalpha(last_character)) // identifier: [a-zA-Z][a-zA-Z0-9]*
    { 
        g_identifier_string = last_character;

        while (isalnum((last_character = getchar())))
        {
            g_identifier_string += last_character;
        }

        if (g_identifier_string == "def") return TOKEN_DEF;
        if (g_identifier_string == "extern") return TOKEN_EXTERN;

        return TOKEN_IDENTIFIER;
    }

    if (isdigit(last_character) || last_character == '.')
    {   // Number: [0-9.]+
        std::string number_string;
        do
        {
            number_string += last_character;
            last_character = getchar();
        } while (isdigit(last_character) || last_character == '.');

      g_number_value = strtod(number_string.c_str(), 0);
      return TOKEN_NUMBER;
    }


    if (last_character == '#')
    {
        do
        {
            last_character = getchar();
        } while(last_character != EOF && last_character != '\n' && last_character != '\r');


        if (last_character != EOF) return get_token();
    }

    // Check for end of file.  Don't eat the EOF.
    if (last_character == EOF) return TOKEN_EOF;

    // Otherwise, just return the character as its ascii value.
    int this_character = last_character;
    last_character = getchar();
     
    return this_character;
}


static int g_current_token;

static int get_next_token() {
    g_current_token = get_token();
    return g_current_token;
}

/// log_error* - These are little helper functions for error handling.
std::unique_ptr<expr_ast> log_error(const char *message)
{
  fprintf(stderr, "Error: %s\n", message);
  return nullptr;
}
std::unique_ptr<prototype_ast> log_error_p(const char *message)
{
  log_error(message);
  return nullptr;
}


// Parser stuff


// awful forward declaration because of circular thbngs.
static std::unique_ptr<expr_ast> parse_expression();


// numberexpr ::= number
static std::unique_ptr<expr_ast> parse_number_expr() {
  auto result = std::make_unique<number_expr_ast>(g_number_value);
  get_next_token();
  return std::move(result);
}

/// parenexpr ::= '(' expression ')'
static std::unique_ptr<expr_ast> parse_parentheses_expr() {
    get_next_token(); // eat (.

    auto value = parse_expression();
    if (!value) return nullptr;

    if (g_current_token != ')') return log_error("expected ')'");

    get_next_token(); // eat ).

    return value;
}


// this is invoked if g_current_token is TOKEN_IDENTIFIER.
// identifier is any string that is not "def"or "extern". This will mostly refer to variable names.
/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
static std::unique_ptr<expr_ast> parse_identifier_expr()
{
    std::string identifier_name = g_identifier_string;
    get_next_token();  // eat identifier.

    // Simple variable ref
    if (g_current_token != '(')  return std::make_unique<variable_expr_ast>(identifier_name);

    // Call.
    get_next_token();  // eat (
    auto function_arguments = std::vector<std::unique_ptr<expr_ast>>{};

    if (g_current_token != ')')
    {
        while (true)
        {
            if (auto argument = parse_expression())
            {
                function_arguments.push_back(std::move(argument));
            }
            else
            {
                return nullptr;
            }

            if (g_current_token == ')') break;

            if (g_current_token != ',') return log_error("Expected ')' or ',' in argument list");

            get_next_token();
        }
    }

    // Eat the ')'.
    get_next_token();

    return std::make_unique<call_expr_ast>(identifier_name, std::move(function_arguments));
}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
static std::unique_ptr<expr_ast> parse_primary() {
  switch (g_current_token)
  {
    default:
        return log_error("unknown token when expecting an expression");
    case TOKEN_IDENTIFIER:
        return parse_identifier_expr();
    case TOKEN_NUMBER:
         return parse_number_expr();
    case '(':
        return parse_parentheses_expr();
  }
}



/// binary_operator_precedence - This holds the precedence for each binary operator that is
/// defined.
auto binary_operator_precedence = std::map<char, int>{
  {'<', 10},
  {'+', 20},
  {'-', 20},
  {'*', 40} 
};

/// get_token_precedence - Get the precedence of the pending binary operator token.
static int get_token_precedence() {
  if (!isascii(g_current_token)) return -1;

  // Make sure it's a declared binop.
  int token_precedence = binary_operator_precedence[g_current_token];

  if (token_precedence <= 0) return -1;

  return token_precedence;
}



static std::unique_ptr<expr_ast> parse_binary_operator_rhs(int expr_precedence, std::unique_ptr<expr_ast> lhs)
{
    // if this is a binop, find the precedence

    while (true)
    {
        int token_precedence = get_token_precedence();

        // if this is a binop that binds at least as tightly as the current binop, 
        // consume it, otherwise, we are done (I guess this means pairs of + and -, or pairs of * and /)
        if (token_precedence < expr_precedence)
        {
            return lhs;
        }


        // okay, we know this is a binary operator
        int binary_operator = g_current_token;
        get_next_token(); // eat binary operator

        // parse the primary expression aftyer the binary operator./
        auto rhs = parse_primary();
        if (!rhs) return nullptr;

        // if the binary operator binds less tightly with rths and the operator after rhs,
        // let the pending operator take rhs as its lhs. (1 + 2 * 3 -> 2 becomes lhs instead of rhs.)
        int next_precedence = get_token_precedence();
        if (token_precedence < next_precedence)
        {
            rhs = parse_binary_operator_rhs(token_precedence + 1, std::move(rhs));

            if (!rhs) return nullptr;
        }

        // merge lhs/ rhs
        lhs = std::make_unique<binary_expr_ast>(binary_operator, std::move(lhs), std::move(rhs));
    }
}

/// expression
///   ::= primary binoprhs
///
static std::unique_ptr<expr_ast> parse_expression()
{
  auto lhs = parse_primary();
  if (!lhs) return nullptr;

  return parse_binary_operator_rhs(0, std::move(lhs));
}

static std::unique_ptr<prototype_ast> parse_prototype()
{
    if (g_current_token != TOKEN_IDENTIFIER)
    {
        return log_error_p("expected function name in prototype.");
    }

    std::string function_name = g_identifier_string;
    get_next_token();


    if (g_current_token != '(')
    {
        return log_error_p("Expected '(' in prototype");
    }
    
    // read the list of argument names.
    std::vector<std::string> argument_names;
    while(get_next_token() == TOKEN_IDENTIFIER)
    {
        argument_names.push_back(g_identifier_string);
       
        

    }

     if (g_current_token != ')')
    {
        return log_error_p("Expecrted ')' in prototype.");
    }

    // success.
    get_next_token(); // eat ')'

    return std::make_unique<prototype_ast>(function_name, std::move(argument_names));

}

// defintion ::= 'def' prototype expression

static std::unique_ptr<function_ast> parse_definition()
{
    get_next_token(); // eat def
    auto prototype = parse_prototype();

    if (!prototype) return nullptr;

    if (auto expr = parse_expression())
    {
        return std::make_unique<function_ast>(std::move(prototype), std::move(expr));
    }

    return nullptr;
}


static std::unique_ptr<function_ast> parse_top_level_expr()
{
    if (auto expr = parse_expression())
    {
        // make an anonymous function prototype.
        auto prototype = std::make_unique<prototype_ast>("", std::vector<std::string>());
        return std::make_unique<function_ast>(std::move(prototype), std::move(expr));
    }

    return nullptr;
}



/// external ::= 'extern' prototype
static std::unique_ptr<prototype_ast> parse_extern()
{
    get_next_token(); // eat extern.
    return parse_prototype();
}


//===----------------------------------------------------------------------===//
// Code Generation
//===----------------------------------------------------------------------===//

static std::unique_ptr<LLVMContext> llvm_context;
static std::unique_ptr<IRBuilder<>> ir_builder;
static std::unique_ptr<Module> module;
static std::map<std::string, Value*> named_values;

Value* log_error_v(const char *message)
{
  log_error(message);
  return nullptr;
}

Value* number_expr_ast::codegen()
{
  return ConstantFP::get(*llvm_context, APFloat(this->value)); // now i'm using "this"!
}


Value* variable_expr_ast::codegen()
{
  // Look this variable up in the function.
  Value *value = named_values[name];
  if (!value) log_error_v("Unknown variable name");

  return value;
}



Value* binary_expr_ast::codegen()
{

  Value* lhs_value = lhs->codegen();
  Value* rhs_value = rhs->codegen();

  if (!lhs_value || !rhs_value) return nullptr;

  switch (op) {
  case '+':
    return ir_builder->CreateFAdd(lhs_value, rhs_value, "addtmp");
  case '-':
    return ir_builder->CreateFSub(lhs_value, rhs_value, "subtmp");
  case '*':
    return ir_builder->CreateFMul(lhs_value, rhs_value, "multmp");
  case '<':
    lhs_value = ir_builder->CreateFCmpULT(lhs_value, rhs_value, "cmptmp");
    // Convert bool 0/1 to double 0.0 or 1.0
    return ir_builder->CreateUIToFP(lhs_value, Type::getDoubleTy(*llvm_context),
                                 "booltmp");
  default:
    return log_error_v("invalid binary operator");
  }
}

Value* call_expr_ast::codegen() {
  // Look up the name in the global module table.
  Function* callee_function = module->getFunction(this->callee);

  if (!callee_function) return log_error_v("Unknown function referenced");

  // If argument mismatch error.
  if (callee_function->arg_size() != this->arguments.size()) return log_error_v("Incorrect # arguments passed");

  // wow, what is even going on here
  std::vector<Value*> value_arguments;
  for (unsigned i = 0, e = value_arguments.size(); i != e; ++i)
  {
    value_arguments.push_back(this->arguments[i]->codegen());
    if (!value_arguments.back()) return nullptr;
  }

  return ir_builder->CreateCall(callee_function, value_arguments, "calltmp");
}



Function* prototype_ast::codegen()
{
    // Make the function type:  double(double,double) etc.
    std::vector<Type*> doubles(
        this->arguments.size(),
        Type::getDoubleTy(*llvm_context));


    FunctionType* function_type = FunctionType::get(Type::getDoubleTy(*llvm_context), doubles, false);

    Function* function = Function::Create(function_type, Function::ExternalLinkage, this->name, module.get());

    // Set names for all arguments.
    unsigned idx = 0;
    for (auto &argument : function->args())
    {
      argument.setName(this->arguments[idx++]);
    }
    return function;
}

Function* function_ast::codegen()
{
    // First, check for an existing function from a previous 'extern' declaration.
    Function* function = module->getFunction(this->prototype->getName());

    if (!function) function = this->prototype->codegen();

    if (!function) return nullptr;

    if (!function->empty()) return (Function*)log_error_v("Function cannot be redefined.");


    // Create a new basic block to start insertion into.
    BasicBlock* basic_block = BasicBlock::Create(*llvm_context, "entry", function);
    ir_builder->SetInsertPoint(basic_block);

    // Record the function arguments in the NamedValues map.
    named_values.clear();
    for (auto &arg : function->args())
    {
        named_values[std::string(arg.getName())] = &arg;
    }


    if (Value *RetVal = this->body->codegen())
    {
        // Finish off the function.
        ir_builder->CreateRet(RetVal);

        // Validate the generated code, checking for consistency.
        verifyFunction(*function);

        return function;
    }

    // Error reading body, remove function.
    function->eraseFromParent();
    return nullptr;
}



//===----------------------------------------------------------------------===//
// Top-Level parsing and JIT driver
//===----------------------------------------------------------------------===//

static void initialize_module()
{
  // Open a new context and module.
  llvm_context = std::make_unique<LLVMContext>();
  module = std::make_unique<Module>("my cool jit", *llvm_context);

  // Create a new ir_builder for the module.
  ir_builder = std::make_unique<IRBuilder<>>(*llvm_context);
}

static void handle_definition()
{
    if (auto function_ast = parse_definition())
    {
        if (auto *function_ir = function_ast->codegen()) 
        {
            fprintf(stderr, "Read function definition:");
            function_ir->print(errs());
            fprintf(stderr, "\n");
        }
      
    } else
    {
        // Skip token for error recovery.
        get_next_token();
    }
}

static void handle_extern()
{
    if (auto prototype_ast = parse_extern())
    {
        if (auto* prototype_ir = prototype_ast->codegen())
        {
            fprintf(stderr, "read extern\n");
            prototype_ir->print(errs()); // this just means: print to stderr.
            fprintf(stderr, "\n"); 
        }
    } else
    {
        // Skip token for error recovery.
        get_next_token();
    }
}

static void handle_top_level_expression() {
  // Evaluate a top-level expression into an anonymous function.
  if (auto top_level_expr_function_ast = parse_top_level_expr())
  {
    if (auto* top_level_expr_function_ir = top_level_expr_function_ast->codegen())
    {
        fprintf(stderr, "read top-level-expression\n");
        top_level_expr_function_ir->print(errs());
        fprintf(stderr, "\n");

        top_level_expr_function_ir->eraseFromParent();
    }
  } else
  {
    // Skip token for error recovery.
    get_next_token();
  }
}


/// top ::= definition | external | expression | ';'
static void main_loop()
{
    while (true) {
        fprintf(stderr, "ready> ");
        switch (g_current_token) {
        case TOKEN_EOF:
        return;
        case ';': // ignore top-level semicolons.
        get_next_token();
        break;
        case TOKEN_DEF:
        handle_definition();
        break;
        case TOKEN_EXTERN:
        handle_extern();
        break;
        default:
        handle_top_level_expression();
        break;
    }
  }
}





// handle arbitrary top-level expressions.
/// toplevelexpr ::= expression






int main()
{
    fprintf(stderr, "ready> ");

    get_next_token();

    // Make the module, which holds all the code.
    initialize_module();

    main_loop();

  // Print out all of the generated code.
    module->print(errs(), nullptr);

    return 0;
}
