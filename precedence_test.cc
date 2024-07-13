#include <string>
#include <iostream>
#include <vector>
#include <cctype>
#include <cstdlib>
#include <cstdint>
#include <cassert>

// I dislike the continue name, every time I read it I get confused
// This is highly illegal but ít's just me now!
#define skip_this_character continue
#define go_to_next_character continue

using namespace std;

enum tag_t
{
	EMPTY = 0,
	LITERAL,
	OPERATOR
};

std::vector<std::string> tag_to_string{"EMPTY",
"LITERAL",
"OPERATOR"
};

union data_t
{
	int literal;
	uint8_t my_operator;
};



struct node_t
{
	tag_t tag;
	data_t data;
	node_t* lhs;
	node_t* rhs;
};


// recursive function.
void print_nodes(const node_t* node)
{
	if (nullptr == node)
	{
		std::cout << "null" << '\n';
	}
	if (node->lhs)
	{
		print_nodes(node->lhs);
	}
	if (node->rhs)
	{
		print_nodes(node->rhs);
	}
	std::cout << "tag:" << tag_to_string[node->tag] << " \n" << "data: ";

	if (node->tag == LITERAL)
	{
		std::cout << node->data.literal;
	}
	if (node->tag == OPERATOR)
	{
		std::cout << (char*)(&node->data.my_operator);
	}
	std::cout << '\n';
}

int evaluate(const node_t* node)
{
	if (nullptr == node)
	{
		return 0;
	}

	if (node->tag == OPERATOR)
	{

		// we can do some dispatch here with lambda functions or whatever but I don't care.
		if (node->data.my_operator == '+')
		{
			return evaluate(node->lhs) + evaluate(node->rhs);
		}
	}

	if (node->tag == LITERAL)
	{
		return node->data.literal;
	}

	assert(true);
	return 0;
}



static bool isoperator(char c)
{	
	auto operators = std::string{"+-*/"};

	auto found = operators.find(c);

	return (found != string::npos);
}

//@NOTE(SJM): the only thing this verifies is that if we are an operator node,
// we have both lhs and rhs completed. this helps us detect the situation 1 +  + 2.
// that is illegal. However, 1 + 2 + 3 is legal. In this case, the "most recent" node (or top level node)
// after parsing 1 + 2 would be a node containing '+', but is "complete" in the sence that both its lhs and rhs contain valid values (in this case, 1 and 2).
static bool is_completed_operator_node(const node_t* node)
{
	assert(nullptr != node);

	auto result = (nullptr != node->lhs && nullptr != node->rhs);
	return result;
}


// parse string 1 + 2 * 3
int main ()
{
	auto string_to_parse = std::string{"1 + 2"};

	//@FIXME(SJM): just assume single character numbers for now.
	node_t* root_node = nullptr;
	node_t* current_node = nullptr;
	for (auto c: string_to_parse)
	{
		if (c == ' ' || c == '\0')
		{
			skip_this_character;
		}

		if (isdigit(c))
		{
			auto* literal_node = new node_t{};
			literal_node->tag = LITERAL;

			// wow this absolutely sucks and we should fix it.
			char whatever[] = {c, '\0'};
			literal_node->data.literal = std::strtol(whatever, nullptr, 10);; 

			if (nullptr != current_node) // do we have a node already?
			{
				if (current_node->tag == LITERAL)
				{
					assert(current_node->tag != LITERAL && "trying to read a literal after a literal (i.e.) 1 1. This is not allowed.\n");
				}

				if (current_node->tag == OPERATOR)
				{
					assert(current_node->lhs != nullptr); // we should have a lhs at this point.
				}
				current_node->rhs = literal_node;
				go_to_next_character;
			}

			if (nullptr == current_node)
			{
				current_node = literal_node;
			}

		}

		// are we an operator?
		if (isoperator(c))
		{

			// is the current node (lhs) a literal?
			if (current_node->tag == LITERAL)
			{
				// great, we can create a new node. current_node is a literal, attach it to lhs.
				auto* operator_node = new node_t{};
				operator_node->tag = OPERATOR;
				operator_node->data.my_operator = c;
				operator_node->lhs = current_node;
				current_node = operator_node;

				go_to_next_character;
			}

			if (current_node->tag == OPERATOR)
			{
				if (is_completed_operator_node(current_node))
				{
					// great: we can attach the current node to lhs of the new node, and continue.
					auto* operator_node = new node_t{};
					operator_node->tag = OPERATOR;
					operator_node->data.my_operator = c;
					operator_node->lhs = current_node;
					current_node = operator_node;

					go_to_next_character;
				} 
			}
		}
	}
	// never fear, never free.
	print_nodes(current_node);
	std::cout << "evaluate: " << evaluate(current_node) << '\n';

}

/* logically,what should happen:
	read 1: create a new node, insert 1 in it. this is the root node.
	read '+': okay, if our current node is a literal -> this is incorrect: in essence, we can have (1 + 2) + 3, in which case the root node would be '+'. 
	this indicates we need some sort of 'completeness' flag in the node? ah, we can use the fact that both lhs and rhs need not be null.
*/

