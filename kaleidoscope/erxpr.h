
#ifndef INCLUDE_EXPR_
#define INCLUDE_EXPR_

/// expr_ast - Base class for all expression nodes.
// we tie it together via pointers to this.
// I do wonder how we resolve the actual type later on though. or do we implement some dynamic dispatch interfaces?
class expr_ast {
public:
  virtual ~expr_ast() = default;
};

/// Numberexpr_ast - Expression class for numeric literals like "1.0".
class number_expr_ast : public expr_ast
{
double value;

public:
    number_expr_ast(double value)
    :
        value(value)
  {}
};

/// variable_expr_ast - Expression class for referencing a variable, like "a".
// @NOTE(SMIA): why does this not have a value associated? where is that stored?
// am I confusing something?
class variable_expr_ast : public expr_ast {
    std::string name;

public:
    variable_expr_ast(const std::string &name) : name(name) {}
};

/// binary_expr_ast - Expression class for a binary operator.
class binary_expr_ast : public expr_ast {
    char my_operator;
    std::unique_ptr<expr_ast> lhs;
    std::unique_ptr<expr_ast> rhs;

public:
  binary_expr_ast(
    char my_operator,
    std::unique_ptr<expr_ast> lhs,
    std::unique_ptr<expr_ast> rhs)
    :
        my_operator(my_operator),
        lhs(std::move(lhs)),
        rhs(std::move(rhs)) {}
};


/// call_expr_ast - Expression class for function calls.
class call_expr_ast : public expr_ast {
  std::string callee;
  std::vector<std::unique_ptr<expr_ast>> arguments;

public:
  call_expr_ast(
    const std::string &callee,
    std::vector<std::unique_ptr<expr_ast>> arguments)
    :
        callee(callee),
        arguments(std::move(arguments))
    {}
};

/// prototype_ast - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
class prototype_ast {
  std::string name;
  std::vector<std::string> arguments;

public:
  prototype_ast(
    const std::string &name,
    std::vector<std::string> arguments)
    :
        name(name),
        arguments(std::move(arguments))
    {}

  const std::string &get_name() const { return name; }
};




/// function_ast - This class represents a function definition itself.
class function_ast {
  std::unique_ptr<prototype_ast> prototype;
  std::unique_ptr<expr_ast> body;

public:
  function_ast(
    std::unique_ptr<prototype_ast> prototype,
    std::unique_ptr<expr_ast> body
    )
    :
        prototype(std::move(prototype)),
        body(std::move(body))
    {}
};


#endif