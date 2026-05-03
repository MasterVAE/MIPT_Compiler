#ifndef TREE_H_
#define TREE_H_

#include <stdlib.h>

enum NodeType
{
    NODE_OPERATION,
    NODE_IDENTIFICATOR,
    NODE_CONSTANT
};

enum Operation
{
    OP_EMPTY,
    OP_VARIABLE,
    OP_FUNCTION,
    OP_ARGUMENT,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_ASSIGN,
    OP_WHILE,
    OP_IF,
    OP_LINE,
    OP_IN,
    OP_OUT,
    OP_EQUAL,
    OP_BIGGER,
    OP_SMALLER,
    OP_NEQUAL,
    OP_RETURN,
    OP_BRACKET_OPEN,
    OP_BRACKET_CLOSE,
    OP_FBRACKET_OPEN,
    OP_FBRACKET_CLOSE,
    OP_COMMA,
    OP_SET,
    OP_DRAW
};

union NodeValue
{
    char* identificator;
    int constant;
    Operation operation;
};

struct VariableData
{
    char* name;
    char* scope_name;
};

struct Nametable
{
    size_t variable_count;
    VariableData* variables;
};

struct TreeNode
{
    NodeType type;
    NodeValue value;

    bool correct;

    Nametable* nametable;
    Nametable* parent_nametable;

    TreeNode* left;
    TreeNode* right;

    TreeNode* parent;
};

struct Tree
{
    TreeNode* root; 
};

Tree* CreateTree();
void TreeDestroy(Tree* tree);
void SaveTree(Tree* tree, const char* filename);
Tree* LoadTree(const char* filename);
void NodeDestroy(TreeNode* node);
TreeNode* CreateNode(NodeType type, NodeValue value, 
                     TreeNode* left = NULL, TreeNode* right = NULL, TreeNode* parent = NULL);
TreeNode* CopyNode(TreeNode* node);
void SetParents(Tree* tree);

bool CheckOperation(TreeNode* node, Operation op);
bool CheckType(TreeNode* node, NodeType type);

static const size_t BUFFER_SIZE = 128;

#endif // TREE_H_