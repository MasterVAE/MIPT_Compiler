#include "assert.h"

#include "tree.h"
#include "optimizator.h"

static void OptimizeNode(TreeNode* node);
static bool IsMathOperation(TreeNode* node);

static void ConstantFolding(TreeNode* node);
static void DeadCodeElimination(TreeNode* node);

Tree* OptimizeTree(Tree* tree)
{
    assert(tree);
    OptimizeNode(tree->root);
    return tree;
}

static void OptimizeNode(TreeNode* node)
{
    if(!node) return;
    OptimizeNode(node->left);
    OptimizeNode(node->right);

    ConstantFolding(node);

    DeadCodeElimination(node);
}

static bool IsMathOperation(TreeNode* node)
{
    return CheckType(node, NODE_OPERATION) && (node->value.operation == OP_ADD || 
                                               node->value.operation == OP_SUB ||
                                               node->value.operation == OP_MUL ||
                                               node->value.operation == OP_DIV);
}

static void ConstantFolding(TreeNode* node)
{
    assert(node);

    if(IsMathOperation(node) &&
       CheckType(node->left, NODE_CONSTANT) &&
       CheckType(node->right, NODE_CONSTANT))
    {
        int value = 0;
        int lval = node->left->value.constant;
        int rval = node->right->value.constant;
        if(CheckOperation(node, OP_ADD))
        {
            value =  lval + rval;
        }
        else if(CheckOperation(node, OP_SUB))
        {
            value = lval - rval;
        }
        else if(CheckOperation(node, OP_MUL))
        {
            value = lval * rval;
        }
        else if(CheckOperation(node, OP_DIV))
        {
            value = lval / rval;
        }

        TreeNode* parent = node->parent;

        bool right = parent->right == node;

        NodeDestroy(node);

        if(right)
        {
            parent->right = CreateNode(NODE_CONSTANT, NodeValue {.constant = value});
        }
        else
        {
            parent->left = CreateNode(NODE_CONSTANT, NodeValue {.constant = value});
        }

        return;
    }
}

static void DeadCodeElimination(TreeNode* node)
{
    assert(node);


    // CODE AFTER RETURN
    if(CheckOperation(node, OP_LINE), CheckOperation(node->left, OP_RETURN))
    {
        NodeDestroy(node->right);
        node->right = NULL;
        return;
    }


    // IF(0)
    if(CheckOperation(node, OP_IF) && CheckType(node->left, NODE_CONSTANT) 
                                   && node->left->value.constant == 0)
    {
        bool right = node->parent->right == node;
        if(right)
        {
            node->parent->right = NULL;
        }
        else
        {
            node->parent->left = NULL;
        }

        NodeDestroy(node);
        return;
    }
}