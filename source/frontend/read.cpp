#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "read.h"
#include "op_data.h"
#include "tokenizator.h"

// #define DEBUG_

static TreeNode* Programm   (Program* prog);
static TreeNode* Block      (Program* prog);
static TreeNode* Line       (Program* prog);
static TreeNode* Assigment  (Program* prog);
static TreeNode* Expression (Program* prog);
static TreeNode* Term       (Program* prog);
static TreeNode* Primar     (Program* prog);
static TreeNode* Element    (Program* prog);
static TreeNode* Brackets   (Program* prog);
static TreeNode* Constant   (Program* prog);
static TreeNode* FuncVar    (Program* prog);
static TreeNode* Identif    (Program* prog);
static TreeNode* System     (Program* prog);
static TreeNode* Argument   (Program* prog);

static Token* CurrentToken(Program* prog);

static bool CheckOperation(Token* token, Operation op);

#define CURRENT CurrentToken(prog)

#define SYNTAX(node)                                                                            \
{                                                                                               \
    Token* token = CURRENT;                                                                     \
    if(!token)                                                                                  \
    {                                                                                           \
        fprintf(stderr, " SYNTAX error %s:%d\n", __FILE__, __LINE__);                           \
    }                                                                                           \
    else                                                                                        \
    {                                                                                           \
        fprintf(stderr, " SYNTAX error %s:%d line: %lu\n", __FILE__, __LINE__, token->line);    \
    }                                                                                           \
    if(node)                                                                                    \
    {                                                                                           \
        NodeDestroy(node);                                                                      \
    }                                                                                           \
    return NULL;                                                                                \
}

Tree* ReadProgramm(Program* prog)
{
    assert(prog);

    Tree* tree = CreateTree();

    tree->root = Programm(prog);

    return tree;
}

static TreeNode* Programm(Program* prog)
{
    assert(prog);

    TreeNode* node = Line(prog);

    if(!node) return CreateNode(NODE_OPERATION, NodeValue {.operation = OP_EMPTY});

    while(prog->current_token < prog->token_count)
    {
        TreeNode* new_node = Line(prog);
        if(!new_node) break;

        node = CreateNode(NODE_OPERATION, NodeValue {.operation = OP_LINE}, 
                                                    node, new_node);
    }

    if(prog->current_token != prog->token_count) SYNTAX(node);
    
    return node;
}

static TreeNode* Block(Program* prog)
{
    assert(prog);

    if(!CheckOperation(CURRENT, OP_FBRACKET_OPEN)) SYNTAX(NULL);

    prog->current_token++;

    TreeNode* node = Line(prog);

    if(CheckOperation(CURRENT, OP_FBRACKET_CLOSE))
    {
        if(node) 
        {
            node = CreateNode(NODE_OPERATION, NodeValue {.operation = OP_LINE}, node);
        }
        if(!node)
        {
            node = CreateNode(NODE_OPERATION, NodeValue {.operation = OP_EMPTY});
        } 
    }

    while(!CheckOperation(CURRENT, OP_FBRACKET_CLOSE))
    {
        TreeNode* new_line = Line(prog);
        if(!new_line) SYNTAX(node);

        node = CreateNode(NODE_OPERATION, NodeValue {.operation = OP_LINE}, 
                                                     node, new_line);
    }

    prog->current_token++;

    return node;
}

static TreeNode* Line(Program* prog)
{
    assert(prog);

    TreeNode* node = Assigment(prog);
    if(!node) return NULL;

    if(!CheckOperation(CURRENT, OP_LINE)) SYNTAX(node);

    prog->current_token++;

    if(node) return node;

    return CreateNode(NODE_OPERATION, NodeValue {.operation = OP_EMPTY});
}

static TreeNode* Assigment(Program* prog)
{
    assert(prog);

    TreeNode* node = Expression(prog);
    if(!node) return NULL;

    while(CheckOperation(CURRENT, OP_ASSIGN))
    {
        prog->current_token++;

        TreeNode* new_node = Assigment(prog);
        if(!new_node) SYNTAX(node);

        node = CreateNode(NODE_OPERATION, NodeValue {.operation = OP_ASSIGN}, node, new_node);
    }

    return node;
}

static TreeNode* Expression(Program* prog)
{
    assert(prog);

    TreeNode* node = Term(prog);
    if(!node) return NULL;

    while(1)
    {
        if(CheckOperation(CURRENT, OP_EQUAL) 
        || CheckOperation(CURRENT, OP_NEQUAL)
        || CheckOperation(CURRENT, OP_SMALLER)
        || CheckOperation(CURRENT, OP_BIGGER))
        {
            Operation current_operation = CURRENT->value.operation;
            prog->current_token++;

            TreeNode* new_node = Term(prog);
            if(!new_node) SYNTAX(node);

            node = CreateNode(
                        NODE_OPERATION, 
                        NodeValue {.operation = current_operation}, 
                        node, 
                        new_node
                    );
            
            
        }
        else break;
    }

    return node;
}

static TreeNode* Term(Program* prog)
{
    assert(prog);

    TreeNode* node = Primar(prog);
    if(!node) return NULL;

    while(1)
    {
        if(CheckOperation(CURRENT, OP_ADD))
        {
            prog->current_token++;

            TreeNode* new_node = Primar(prog);
            if(!new_node) SYNTAX(node);

            node = CreateNode(NODE_OPERATION, NodeValue {.operation = OP_ADD}, node, new_node);
        }
        else if(CheckOperation(CURRENT, OP_SUB))
        {
            prog->current_token++;

            TreeNode* new_node = Primar(prog);
            if(!new_node) SYNTAX(node);

            node = CreateNode(NODE_OPERATION, NodeValue {.operation = OP_SUB}, node, new_node);
        }
        else break;
    }

    return node;
}

static TreeNode* Primar(Program* prog)
{
    assert(prog);

    TreeNode* node = Element(prog);
    if(!node) return NULL;

    while(1)
    {
        if(CheckOperation(CURRENT, OP_MUL)
        || CheckOperation(CURRENT, OP_DIV))
        {
            Operation current_operation = CURRENT->value.operation;
            prog->current_token++;

            TreeNode* new_node = Element(prog);
            if(!new_node) SYNTAX(node);

            node = CreateNode(
                        NODE_OPERATION, 
                        NodeValue {.operation = current_operation}, 
                        node, 
                        new_node
                    );
        }
        else break;
    }

    return node;
}


static TreeNode* Element(Program* prog)
{
    assert(prog);

    TreeNode* node = NULL;

    node = Brackets(prog);
    if(!node) node = Constant(prog);
    if(!node) node = System(prog);
    if(!node) node = FuncVar(prog);

    return node;
}

static TreeNode* Brackets(Program* prog)
{
    assert(prog);

    TreeNode* node = NULL;

    if(!CheckOperation(CURRENT, OP_BRACKET_OPEN)) return NULL;

    prog->current_token++;

    node = Assigment(prog);

    if(!CheckOperation(CURRENT, OP_BRACKET_CLOSE)) SYNTAX(node);

    prog->current_token++;

    return node;
}

static TreeNode* Constant(Program* prog)
{
    assert(prog);
    
    if(CURRENT->type != NODE_CONSTANT)  return NULL;

    TreeNode* node = CreateNode(NODE_CONSTANT, CURRENT->value);

    prog->current_token++;

    return node;
}

static TreeNode* System(Program* prog)
{
    assert(prog);

    TreeNode* node = NULL;

    if(CheckOperation(CURRENT, OP_IN))
    {
        prog->current_token++;

        node = CreateNode(NODE_OPERATION, NodeValue {.operation = OP_IN});
    }
    else if(CheckOperation(CURRENT, OP_OUT))
    {
        prog->current_token++;

        node = CreateNode(NODE_OPERATION, NodeValue {.operation = OP_OUT}, Argument(prog));
    }
    else if(CheckOperation(CURRENT, OP_IF))
    {
        prog->current_token++;

        TreeNode* node1 = Argument(prog);
        TreeNode* node2 = Block(prog);

        node = CreateNode(NODE_OPERATION, NodeValue {.operation = OP_IF}, node1, node2);

    }
    else if(CheckOperation(CURRENT, OP_WHILE))
    {
        prog->current_token++;
        
        TreeNode* node1 = Argument(prog);
        TreeNode* node2 = Block(prog);

        node = CreateNode(NODE_OPERATION, NodeValue {.operation = OP_WHILE}, node1, node2);
    }
    else if(CheckOperation(CURRENT, OP_RETURN))
    {
        prog->current_token++;
        
        TreeNode* node1 = Assigment(prog);

        node = CreateNode(NODE_OPERATION, NodeValue {.operation = OP_RETURN}, node1);
    }

    return node;
}

static TreeNode* FuncVar(Program* prog)
{
    assert(prog);

    TreeNode* ident = Identif(prog);
    if(!ident) return NULL;

    if(CheckOperation(CURRENT, OP_BRACKET_OPEN))
    {
        TreeNode* arg = Argument(prog);
        if(!arg) SYNTAX(ident);

        if(CheckOperation(CURRENT, OP_FBRACKET_OPEN))
        {
            TreeNode* block = Block(prog);
            if(!block) 
            {
                NodeDestroy(arg);
                SYNTAX(ident);
            }

            return CreateNode(  
                NODE_OPERATION, 
                NodeValue {.operation = OP_FUNCTION}, ident, 
                CreateNode(NODE_OPERATION, NodeValue {.operation = OP_FUNCTION}, arg, block)
            );

        }
        return CreateNode(NODE_OPERATION, NodeValue {.operation = OP_FUNCTION}, ident, arg);
    }

    return CreateNode(NODE_OPERATION, NodeValue {.operation = OP_VARIABLE}, ident);

}

static TreeNode* Identif(Program* prog)
{
    assert(prog);

    if(CURRENT->type != NODE_IDENTIFICATOR) return NULL;

    Token* current = CURRENT;

    prog->current_token++;

    return CreateNode(NODE_IDENTIFICATOR, 
                      NodeValue {.identificator = strdup(current->value.identificator)});
}

static TreeNode* Argument(Program* prog)
{
    assert(prog);

    TreeNode* node = NULL;

    if(!CheckOperation(CURRENT, OP_BRACKET_OPEN)) SYNTAX(NULL);
    prog->current_token++;

    node = Assigment(prog);

    while(CheckOperation(CURRENT, OP_COMMA))
    {
        prog->current_token++;

        TreeNode* node1 = Assigment(prog);
        if(!node1) SYNTAX(node);

        node = CreateNode(NODE_OPERATION, NodeValue {.operation = OP_ARGUMENT}, node, node1);
    }

    if(!CheckOperation(CURRENT, OP_BRACKET_CLOSE)) SYNTAX(node);

    prog->current_token++;

    if(!node) node = CreateNode(NODE_OPERATION, NodeValue {.operation = OP_EMPTY});

    return node;
}


static Token* CurrentToken(Program* prog)
{
    assert(prog);

    if(prog->current_token >= prog->token_count)
    {
        return NULL;
    }

    return &prog->tokens[prog->current_token];
}

static bool CheckOperation(Token* token, Operation op)
{
    assert(token);

    return token->type == NODE_OPERATION && token->value.operation == op;
}