#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "tree.h"
#include "compilator.h"
#include "scope.h"

#define PRINT(...) fprintf(file, __VA_ARGS__);

#define ERROR   {                                                                       \
                    fprintf(stderr, "%s:%d Compilator error\n", __FILE__, __LINE__);    \
                    return;                                                             \
                }

static Compilator* CreateCompilator();

static void DestroyCompilator(Compilator* compilator);

static void CompileNode(TreeNode* node, FILE* file, Compilator* compilator);

static void CompileSystemCodeBefore(FILE* file, TreeNode* root);
static void CompileSystemCodeAfter(FILE* file);
static void CompileFunctions(FILE* file, Compilator* compilator);
static void CompileArguments(TreeNode* node, FILE* file, Compilator* compilator);

static size_t SearchFuncInNametable(Compilator* compilator, const char* identificator);

struct VarCounter
{
    char** idents;
    size_t idents_count;
};

void CompileTree(Tree* tree, FILE* file)
{
    assert(tree);
    assert(file);

    Compilator* compilator = CreateCompilator();
    if(!compilator) return;

    SetNametables(tree);

    CompileSystemCodeBefore(file, tree->root);

    CompileNode(tree->root, file, compilator);

    CompileSystemCodeAfter(file);

    CompileFunctions(file, compilator);

    DestroyCompilator(compilator);

    ClearNametables(tree);
}

static void CompileSystemCodeBefore(FILE* file, TreeNode* root)
{
    assert(file);
    assert(root);

    PRINT(  "DEFAULT REL\n"
            "section     .text\n"
            "extern L0_EQUAL\n"
            "extern L0_NEQUAL\n"
            "extern L0_SMALLER\n"
            "extern L0_BIGGER\n"
            "extern L0_IN\n"
            "extern L0_OUT\n"
            "global main\n"
            "main:\n"
            "lea rbp, [rsp + 8]\n"
            "lea rax, [rbp + 8]\n"
            "lea rbx, [rbp + %lu]\n"
            "mov [rax], rbx\n", (root->nametable->variable_count + 2)*8);
}


static void CompileSystemCodeAfter(FILE* file)
{
    assert(file);

    PRINT(
        "_end:\n"
        "mov rax, 0x3C\n"
        "xor rdi, rdi\n"
        "syscall\n"
    )
}

static void CompileFunctions(FILE* file, Compilator* compilator)
{
    assert(file);
    assert(compilator);

    for(size_t i = 0; i < compilator->function_count; i++)
    {
        TreeNode* func = compilator->functions[i];
        PRINT("L_%s:\n", func->left->value.identificator);
        PRINT("pop rcx\n");
        CompileArguments(func->right->left, file, compilator);
        PRINT("push rcx\n");
        CompileNode(func->right->right, file, compilator);

        PRINT(  "mov rbp, [rbp]\n"
                "ret\n");
    }
}

static void CompileArguments(TreeNode* node, FILE* file, Compilator* compilator)
{
    assert(file);
    assert(compilator);

    if(!node) return;

    if(CheckOperation(node, OP_ARGUMENT))
    {
        CompileArguments(node->left, file, compilator);
        CompileArguments(node->right, file, compilator);
    }
    else if(CheckOperation(node, OP_VARIABLE))
    {
        int i = VariableOffcet(node);
        if(i == -1) ERROR;

        PRINT(  "pop rax\n"
                "mov [rbp + %d], rax\n", (i + 2) * 8);
    }
    else
    {
        ERROR;
        return;
    }

}

static void CompileNode(TreeNode* node, FILE* file, Compilator* compilator)
{
    assert(node);
    assert(file);
    assert(compilator);

    assert(node->type != NODE_IDENTIFICATOR);

    if(node->type == NODE_CONSTANT)
    {
        PRINT("mov rax, %d\n", node->value.constant);
        PRINT("push rax\n");
        return;
    }

    switch (node->value.operation)
    {
        case OP_LINE:
        {
            if(node->left) CompileNode(node->left, file, compilator);
            if(node->right) CompileNode(node->right, file, compilator);
            return;
        }
        case OP_VARIABLE:
        {
            int i = VariableOffcet(node);
            if(i == -1) ERROR;

            PRINT("mov rax, [rbp + %d]\n", (i + 2) * 8);
            PRINT("push rax\n")
                    
            return;
        }
        case OP_ADD:
        {
            CompileNode(node->left, file, compilator);
            CompileNode(node->right, file, compilator);

            PRINT(  "pop rax\n"
                    "pop rbx\n"
                    "add rax, rbx\n"
                    "push rax\n");

            return;
        }
        case OP_SUB:
        {
            CompileNode(node->right, file, compilator);
            CompileNode(node->left, file, compilator);

            PRINT(  "pop rax\n"
                    "pop rbx\n"
                    "sub rax, rbx\n"
                    "push rax\n");
            return;
        }
        case OP_MUL:
        {
            CompileNode(node->right, file, compilator);
            CompileNode(node->left, file, compilator);

            PRINT(  "pop rax\n"
                    "pop rbx\n"
                    "mov rdx, 0\n"
                    "imul rbx\n"
                    "push rax\n");


            return;
        }
        case OP_DIV:
        {
            CompileNode(node->right, file, compilator);
            CompileNode(node->left, file, compilator);

            PRINT(  "pop rax\n"
                    "pop rbx\n"
                    "mov rdx, 0\n"
                    "idiv rbx\n"
                    "push rax\n");

            return;
        }
        case OP_ASSIGN:
        {
            if(!node->left || !node->left->left) ERROR;

            int i = VariableOffcet(node->left);
            if(i == -1) ERROR;

            CompileNode(node->right, file, compilator);

            PRINT("pop rax\n");
            PRINT("mov [rbp + %d], rax\n", (i + 2) * 8);

            return;
        }
        case OP_OUT:
        {
            CompileNode(node->left, file, compilator);
           
            PRINT("call L0_OUT\n");

            return;
        }
        case OP_IN:
        {
            PRINT("call L0_IN\n");

            return;
        }
        case OP_IF:
        {
            size_t lable = compilator->current_label++;

            CompileNode(node->left, file, compilator);
            PRINT("pop rax\n");
            PRINT("mov rbx, 0\n");
            PRINT("cmp rax, rbx\n");

            PRINT("je .L%lu\n", lable);

            CompileNode(node->right, file, compilator);

            PRINT(".L%lu:\n", lable);

            return;
        }
        case OP_WHILE:
        {
            size_t label1 = compilator->current_label++;
            size_t label2 = compilator->current_label++;

            PRINT(".L%lu:\n", label1);

            CompileNode(node->left, file, compilator);

            PRINT("pop rax\n");
            PRINT("mov rbx, 0\n");
            PRINT("cmp rax, rbx\n");

            PRINT("je .L%lu\n", label2);

            CompileNode(node->right, file, compilator);

            PRINT("jmp .L%lu\n", label1);

            PRINT(".L%lu:\n", label2);

            return;
        }
        case OP_EQUAL:
        {
            CompileNode(node->left, file, compilator);
            CompileNode(node->right, file, compilator);

            PRINT("call L0_EQUAL\n");

            return;
        }
        case OP_NEQUAL:
        {
            CompileNode(node->left, file, compilator);
            CompileNode(node->right, file, compilator);

            PRINT("call L0_NEQUAL\n");

            return;
        }
        case OP_SMALLER:
        {

            CompileNode(node->left, file, compilator);
            CompileNode(node->right, file, compilator);

            PRINT("call L0_SMALLER\n");

            return;
        }
        case OP_BIGGER:
        {

            CompileNode(node->left, file, compilator);
            CompileNode(node->right, file, compilator);

            PRINT("call L0_BIGGER\n");

            return;
        }
        case OP_FUNCTION:
        {
            char* name = node->left->value.identificator;

            if(CheckOperation(node->right, OP_FUNCTION)
            && (!node->right->left || node->right->left->type != NODE_IDENTIFICATOR))
            {
                size_t i = SearchFuncInNametable(compilator, name);
                if(i < compilator->function_count) ERROR;

                compilator->function_count++;

                compilator->functions 
                = (TreeNode**)realloc(compilator->functions, 
                                      compilator->function_count * sizeof(TreeNode*));

                compilator->functions[compilator->function_count - 1] = node;

                return;
            }

            CompileNode(node->right, file, compilator);

            PRINT("lea rax, [rbp + 8]\n");
            PRINT("mov rax, [rax]\n");
            PRINT("mov [rax], rbp\n");

            PRINT("mov rbp, rax\n");

            PRINT("lea rax, [rbp + 8]\n");
            PRINT("lea rbx, [rbp + %lu]\n", (2 + node->nametable->variable_count) * 8);
            PRINT("mov [rax], rbx\n");
            PRINT("call L_%s\n", node->left->value.identificator);

            return;
        }
        case OP_RETURN:
        {
            if(node->left)  CompileNode(node->left, file, compilator);

            PRINT(  "mov rbp, [rbp]\n"
                    "ret\n");

            return;
        }
        case OP_ARGUMENT:
        {
            CompileNode(node->right, file, compilator);
            CompileNode(node->left, file, compilator);

            return;
        }
        case OP_EMPTY:
        case OP_COMMA:
        case OP_BRACKET_OPEN:
        case OP_BRACKET_CLOSE:
        case OP_FBRACKET_OPEN:
        case OP_FBRACKET_CLOSE:
        default:
        {
            ERROR;
            return;
        }
    }
}


static Compilator* CreateCompilator()
{
    Compilator* comp = (Compilator*)calloc(1, sizeof(Compilator));
    if(!comp) return NULL;

    comp->current_label = 0;

    comp->function_count = 0;
    comp->functions = (TreeNode**)calloc(1, sizeof(TreeNode*));
    if(!comp->functions)
    {
        free(comp);
        return NULL;
    }

    return comp;
}

static void DestroyCompilator(Compilator* compilator)
{
    if(!compilator) return;

    free(compilator->functions);
    free(compilator);
}

static size_t SearchFuncInNametable(Compilator* compilator, const char* identificator)
{
    assert(compilator);
    assert(identificator);

    size_t i = 0;
    for(i = 0; i < compilator->function_count; i++)
    {
        if(compilator->functions[i] 
        && !strcmp(identificator, compilator->functions[i]->left->value.identificator))
        {
            return i;
        }
    }

    return i;
}