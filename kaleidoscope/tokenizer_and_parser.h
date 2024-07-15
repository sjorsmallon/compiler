#ifndef INCLUDED_TOKENIZER_
#define INCLUDED_TOKENIZER_

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

static std::string g_identifier_string; // Filled in if TOKEN_IDENTIFIER (wow, so not part of a union?)
static double g_number_value;  



//@NOTE(SJM): I hate this naming convention. what is current token vs get_token???
/// get_token - Return the next(!!!!) token from standard input. the çurrent" token is just stored in g_current_token.
// we just return an enum value, and the client knows that for specific tokens,
// they can access some properties here (e.g. g_identifier_string or g_number_value).
// that seems like an awful thing but whatever.

static Token get_token()
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
std::unique_ptr<expr_ast> log_error(const char *message) {
  fprintf(stderr, "Error: %s\n", message);
  return nullptr;
}
std::unique_ptr<prototype_ast> log_error_p(const char *message) {
  log_error(message);
  return nullptr;
}

// Parser stuff

// numberexpr ::= number
static std::unique_ptr<expr_ast> parse_number_expr() {
  auto result = std::make_unique<number_expr_ast>(number_value);
  get_next_token(); // consume the number (THIS IS SO CONFUSING!)
  return std::move(result);
}


static std::unique_ptr<expr_ast> parse_parentheses_expr() {
    get_next_token(); // eat (.

    auto value = parse_expression();
    if (!value) return nullptr;

    if (g_current_token != ')') return log_error("expected ')'");

    get_next_token(); // eat ).

    return value;
}


// this is invoked if current_token is TOKEN_IDENTIFIER.
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
            if (auto argument = ParseExpression())
            {
                function_arguments.push_back(std::move(argument));
            }
            else
            {
                return nullptr;
            }

            if (g_current_token == ')') break;

            if (g_current_token != ',') return LogError("Expected ')' or ',' in argument list");

            getNextToken();
        }
    }

    // Eat the ')'.
    get_next_token();

    return std::make_unique<call_expr_ast>(identifier_name, std::move function_arguments));
}


#endif