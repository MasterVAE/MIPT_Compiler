#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "tree.h"
#include "scope.h"
#include "compilator.h"

enum SearchState
{
    None,
    Use,
    Def
};

static size_t BLOCK_COUNT = 0; 

static void ClearNametablesNode(TreeNode* node);
static void AddVarToNametable(Nametable* nametable, const char* var, const char* scope_name);
static void CheckFuncNametables(TreeNode* node, Nametable* nametable);

static SearchState VariableStateInTree(TreeNode* node, const char* var);

static void SetNametablesReqursive(TreeNode*    node, 
                                   Nametable*   nametable, 
                                   Nametable*   parent_nametable);

Nametable* CreateNametable()
{
    Nametable* nametable = (Nametable*)calloc(1, sizeof(Nametable));
    if(!nametable) return NULL;

    nametable->variable_count = 0;
    nametable->variables = (VariableData*)calloc(1, sizeof(VariableData));
    if(!nametable->variables) return NULL;

    return nametable;
}

void DestroyNametable(Nametable* nametable)
{
    if(!nametable) return;

    for(size_t i = 0; i < nametable->variable_count; i++)
    {
        free(nametable->variables[i].name);
        free(nametable->variables[i].scope_name);
    }

    free(nametable->variables);

    free(nametable);
}

Nametable* CopyNametable(Nametable* nametable)
{
    assert(nametable);
    
    Nametable* copy = CreateNametable();
    if(!copy) return NULL;

    copy->variable_count = nametable->variable_count;

    copy->variables = (VariableData*)realloc(copy->variables,
                                        (copy->variable_count == 0 ? 1 : copy->variable_count) *
                                        sizeof(VariableData));
    
    if(!copy->variables) return NULL;

    for(size_t i = 0; i < copy->variable_count; i++)
    {
        copy->variables[i].name = strdup(nametable->variables[i].name);
        copy->variables[i].scope_name = strdup(nametable->variables[i].scope_name);
    }

    return copy;
}

int FindVarNameInNametable(Nametable* nametable, const char* variable)
{
    assert(nametable);
    assert(variable);

    for(int i = 0; i < (int)nametable->variable_count; i++)
    {
        if(!strcmp(nametable->variables[i].name, variable)) return i;
    }

    return -1;
}

int FindVarScopeInNametable(Nametable* nametable, const char* variable)
{
    assert(nametable);
    assert(variable);

    for(int i = 0; i < (int)nametable->variable_count; i++)
    {
        if(!strcmp(nametable->variables[i].scope_name, variable)) return i;
    }

    return -1;
}

void ClearNametables(Tree* tree)
{
    assert(tree);
    DestroyNametable(tree->root->parent_nametable);
    ClearNametablesNode(tree->root);
}

static void ClearNametablesNode(TreeNode* node)
{
    if(!node) return;

    ClearNametablesNode(node->left);
    ClearNametablesNode(node->right);

    if(!node->nametable) return;
    
    DestroyNametable(node->nametable);
}

void SetNametables(Tree* tree)
{
    assert(tree);

    if(!tree->root) return;

    Nametable* nametable = CreateNametable();
    tree->root->nametable = nametable;
    tree->root->parent_nametable = nametable;
    CheckFuncNametables(tree->root, nametable);
    SetNametablesReqursive(tree->root, tree->root->nametable, tree->root->nametable);
}

static void AddVarToNametable(Nametable* nametable, const char* var, const char* scope_name)
{
    assert(nametable);
    assert(var);
    assert(scope_name);

    nametable->variable_count++;
    VariableData* new_vars = (VariableData*)realloc(nametable->variables, 
                                                  sizeof(VariableData) * nametable->variable_count);
    if(!new_vars) return;

    nametable->variables = new_vars;
    new_vars[nametable->variable_count - 1].name = strdup(var);
    new_vars[nametable->variable_count - 1].scope_name = strdup(scope_name);
}

static void CheckFuncNametables(TreeNode* node, Nametable* nametable)
{
    if(!node) return;

    if( CheckOperation(node, OP_FUNCTION) &&
        CheckOperation(node->right, OP_FUNCTION) &&
        CheckType(node->left, NODE_IDENTIFICATOR) &&
        !CheckType(node->right->left, NODE_IDENTIFICATOR))
    {
        Nametable* new_nametable = CreateNametable();
        node->nametable = new_nametable;
        node->parent_nametable = new_nametable;

        CheckFuncNametables(node->left, new_nametable);
        CheckFuncNametables(node->right, new_nametable);

        return;
    }

    node->parent_nametable = nametable;

    if(CheckOperation(node, OP_VARIABLE))
    {
        const char* name = node->left->value.identificator;

        if(FindVarNameInNametable(nametable, name) != -1) return;
        AddVarToNametable(nametable, name, name);
        return;
    }

    CheckFuncNametables(node->left, nametable);
    CheckFuncNametables(node->right, nametable);
}

static void SetNametablesReqursive(TreeNode*    node, 
                                   Nametable*   nametable, 
                                   Nametable*   parent_nametable)
{
    assert(nametable);
    assert(parent_nametable);

    if(!node) return;

    if( CheckOperation(node, OP_FUNCTION) &&
        CheckOperation(node->right, OP_FUNCTION) &&
        CheckType(node->left, NODE_IDENTIFICATOR) &&
        !CheckType(node->right->left, NODE_IDENTIFICATOR))
    {

        SetNametablesReqursive(node->left, node->nametable, node->nametable);
        SetNametablesReqursive(node->right, node->nametable, node->nametable);
        return;
    }

    node->nametable = CopyNametable(nametable);

    if(CheckOperation(node, OP_IF) || CheckOperation(node, OP_WHILE))
    {
        BLOCK_COUNT++;
        SetNametablesReqursive(node->left, nametable, parent_nametable);

        Nametable* new_nametable = CopyNametable(nametable);
        for(size_t i = 0; i < node->nametable->variable_count; i++)
        {
            const char* name = node->nametable->variables[i].name;
            SearchState state = VariableStateInTree(node->right, name);

            if(state == Def)
            {
                char buffer[BUFFER_SIZE] = {0};
                char scope_name[BUFFER_SIZE] = {0};

                sscanf(name, "%s", buffer);
                sprintf(scope_name, "%s_%lu", buffer, BLOCK_COUNT);

                free(new_nametable->variables[i].scope_name);
                new_nametable->variables[i].scope_name = strdup(scope_name);
                AddVarToNametable(parent_nametable, name, scope_name);
            }
        }

        SetNametablesReqursive(node->right, new_nametable, parent_nametable);
        DestroyNametable(new_nametable);
        return;
    }

    SetNametablesReqursive(node->left, nametable, parent_nametable);
    SetNametablesReqursive(node->right, nametable, parent_nametable);
}

static SearchState VariableStateInTree(TreeNode* node, const char* var)
{
    assert(var);

    if(!node) return None;

    if(CheckOperation(node, OP_VARIABLE))
    {
        if(!strcmp(var, node->left->value.identificator))
        {
            return Use;
        }

        return None;
    }
    if(CheckOperation(node, OP_ASSIGN))
    {
        if(!strcmp(var, node->left->left->value.identificator))
        {
            SearchState right = VariableStateInTree(node->right, var);
            if(right == Use) return Use;

            return Def;
        }

        return VariableStateInTree(node->right, var);
    }

    SearchState left = VariableStateInTree(node->left, var);
    SearchState right = VariableStateInTree(node->right, var);

    return left == None ? right : left;
}


size_t FrameSize(TreeNode* node, Compilator* compilator)
{
    assert(node);

    const char* name = node->left->value.identificator;

    for(size_t i = 0; i < compilator->function_count; i++)
    {
        if(!strcmp(compilator->functions[i]->left->value.identificator, name))
        {
            return compilator->functions[i]->parent_nametable->variable_count;
        }
    }
    return 0;
}

int VariableOffcet(TreeNode* node)
{
    assert(CheckOperation(node, OP_VARIABLE));

    int i = FindVarNameInNametable(node->nametable, node->left->value.identificator);
    if(i == -1) return -1;

    const char* scope_name = node->nametable->variables[i].scope_name;

    return FindVarScopeInNametable(node->parent_nametable, scope_name);
}