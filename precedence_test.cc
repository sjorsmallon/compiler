#include <iostream>
#include <cstdint>
#include <string>
#include <cassert>

enum token_type_t
{
    TOKEN_NUMBER,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_DIVIDE,
    TOKEN_MULTIPLY,
    TOKEN_END
};

struct token_t
{
    token_type_t token_type;
    double value;
    char my_operator;
};

struct tokenizer_t
{
    std::string input;
    size_t position;
};


token_t next_token(tokenizer_t& tokenizer)
{
    const char* input = tokenizer.input.c_str();
    size_t position = tokenizer.position;
    while (input[position] && isspace(input[position]))
    {
        ++position;
    }

    if (!input[position])
    {
        return token_t{TOKEN_END, 0, 0};
    }

    char current = input[position];

    if (isdigit(current) || current == '.')
    {
        size_t start = position;

        // keep advancing until no digits.
        while (isdigit(input[position]) || input[position] == '.')
        {
            position++;
        }

        tokenizer.position = position;
        return token_t{TOKEN_NUMBER, strtod(&input[start], NULL), 0};
    }

    tokenizer.position = position + 1; // start from this position next time.
    switch (current)
    {
        case '+':
        {
            return token_t{TOKEN_PLUS, 0, '+'};
        }
        case '-':
        {
            return token_t{TOKEN_MINUS, 0, '-'};
        }
        case '*':
        {
            return token_t{TOKEN_MULTIPLY, 0, '*'};
        }
        case '/':
        {
            return token_t{TOKEN_DIVIDE, 0, '/'};
        }
        default:
        {
            std::cerr << "Unknown character: " << current << '\n';
            exit(1);
        }
    }
}

struct parser_t
{
    tokenizer_t tokenizer;
    token_t current_token;
};


enum node_type
{
    NODE_TYPE_NUMBER,
    NODE_TYPE_BINARY_OP
};


// Base Node struct
struct node_t {
    // type definition
    struct binary_operator_data_t{
            char my_operator;
            node_t* lhs;
            node_t* rhs;
    };
    // type definition
    union data_t
    {
        double value;
        binary_operator_data_t binary_operator_data;
    };

    node_type type;
    data_t data;
};


void print_node(node_t* node) {
    if (node->type == NODE_TYPE_NUMBER) {
        printf("%f", node->data.value);
    } else if (node->type == NODE_TYPE_BINARY_OP) {
        printf("(");
        print_node(node->data.binary_operator_data.lhs);
        printf(" %c ", node->data.binary_operator_data.my_operator);
        print_node(node->data.binary_operator_data.rhs);
        printf(")");
    }
}

void free_node(node_t* node) {
    if (node->type == NODE_TYPE_BINARY_OP) {
        free_node(node->data.binary_operator_data.lhs);
        free_node(node->data.binary_operator_data.rhs);
    }
    delete node;
}


// these are just constructors
node_t* create_number_node(double value)
{
    node_t* number_node = new node_t{
        .type = NODE_TYPE_NUMBER,
        .data = node_t::data_t{
            .value = value
        }
    };

    return number_node;
}


// these are just constructors
node_t*  create_binary_operator_node(char my_operator, node_t* lhs, node_t* rhs)
{
    node_t* binary_operator_node = new node_t{};
    binary_operator_node->type = NODE_TYPE_BINARY_OP;
    binary_operator_node->data.binary_operator_data = node_t::binary_operator_data_t{
            .my_operator = my_operator,
            .lhs = lhs,
            .rhs = rhs
    };
    return binary_operator_node;
}

// <expression>    ::= <term> { ("+" | "-") <term> }
// <term>          ::= <factor> { ("*" | "/") <factor> }
// <factor>        ::= <number> | "(" <expression> ")"
// <number>        ::= <digit> { <digit> } [ "." <digit> { <digit> } ]
// <digit>         ::= "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"



//FIXME(SMIA) no support for parenthesis now. we will add that later.
node_t* parse_factor(parser_t& parser)
{
    if (parser.current_token.token_type == TOKEN_NUMBER)
    {
        node_t* node = create_number_node(parser.current_token.value);
        parser.current_token = next_token(parser.tokenizer);
        return node;
    };

    assert(false && "tried to find a number but could not find any.\n");
    return nullptr;
}

/// aaah, this chews through sequential 1 * 2 * 3 * 4.
// creating:
// 1 * (2 * (3 * 4))))
node_t* parse_term(parser_t& parser)
{
    node_t* node = parse_factor(parser);
    while (parser.current_token.token_type == TOKEN_MULTIPLY || parser.current_token.token_type == TOKEN_DIVIDE) {
        char op = (parser.current_token.token_type == TOKEN_MULTIPLY) ? '*' : '/';
        parser.current_token = next_token(parser.tokenizer);
        node = create_binary_operator_node(op, node, parse_factor(parser));
    };
    return node;
}


node_t* parse_expression(parser_t& parser)
{
     node_t* node = parse_term(parser);
     while(parser.current_token.token_type == TOKEN_PLUS || parser.current_token.token_type == TOKEN_MINUS)
     {
       char op = (parser.current_token.token_type == TOKEN_PLUS) ? '+' : '-';
       parser.current_token = next_token(parser.tokenizer);
       node = create_binary_operator_node(op, node, parse_term(parser));
     }

     return node;
}


int main()
{
    auto string_to_parse = std::string{"1 + 2 * 3 + 4"};

    auto parser = parser_t{
        .tokenizer = tokenizer_t{
            .input = string_to_parse,
            .position = std::size_t{0}
        },
        .current_token = {}
    };
    parser.current_token = next_token(parser.tokenizer);

    node_t* root_node = parse_expression(parser);

    print_node(root_node);
    free_node(root_node);





}