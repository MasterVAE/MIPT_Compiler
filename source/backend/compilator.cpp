#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "tree.h"
#include "compilator.h"
#include "scope.h"
#include "nasm.h"

#include "nasm.h"

#define PRINT(...) fprintf(file, __VA_ARGS__);

#define SET(buffer, size)                                                               \
    memcpy(compilator->buffer + compilator->current_command, buffer, size);             \
    compilator->current_command += size;

#define SET_BYTE(byte)                                                                  \
    compilator->buffer[compilator->current_command++] = byte;

#define SET_VALUE(constant)                                                             \
    {                                                                                   \
        int value = constant;                                                           \ 
        memcpy(compilator->buffer + compilator->current_command, &value, 4);            \
        compilator->current_command += 4;                                               \
    }   


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
static void LinkLibrary(FILE* file);

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

    LinkLibrary(file);

    NasmCompile("files/prog.bin", compilator);

    DestroyCompilator(compilator);

    ClearNametables(tree);
}

static void CompileSystemCodeBefore(FILE* file, TreeNode* root)
{
    assert(file);
    assert(root);

    PRINT(  "DEFAULT REL\n");
    PRINT(  "section     .text\n\n");
    PRINT(  "global main\n\n");
    PRINT(  "main:\n");
    PRINT(  "   sub rsp, 1024         ; создание базового фрейма\n");
    PRINT(  "   mov rbp, rsp\n");
    PRINT(  "   add rbp, 8\n");
    PRINT(  "   mov rax, rbp\n");
    PRINT(  "   add rax, 8\n");
    PRINT(  "   mov rbx, rbp\n");
    PRINT(  "   add rbx, %lu\n", (root->nametable->variable_count + 3) * 8);
    PRINT(  "   mov [rax], rbx\n");
}


static void CompileSystemCodeAfter(FILE* file)
{
    assert(file);

    PRINT(  "_end:               ; syscall выхода из программы\n");
    PRINT(  "   mov rax, 0x3C\n");
    PRINT(  "   xor rdi, rdi\n");
    PRINT(  "   syscall\n");

}

static void CompileFunctions(FILE* file, Compilator* compilator)
{
    assert(file);
    assert(compilator);

    for(size_t i = 0; i < compilator->function_count; i++)
    {
        TreeNode* func = compilator->functions[i];
        PRINT("L_%s:                ; функция %s\n", func->left->value.identificator, func->left->value.identificator);
        PRINT(  "   pop rcx\n");
        PRINT(  "   mov rax, rbp\n");
        PRINT(  "   add rax, 16\n");
        PRINT(  "   mov [rax], rcx\n");
        CompileArguments(func->right->left, file, compilator);
        CompileNode(func->right->right, file, compilator);

        PRINT("\n   mov rax, rbp\n");
        PRINT(  "   add rax, 16\n");
        PRINT(  "   mov rax, [rax]\n");
        PRINT(  "   push rax\n");
        PRINT(  "   mov rbp, [rbp]\n");
        PRINT(  "   ret\n");
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
        const char* name = node->left->value.identificator;

        int i = VariableOffcet(node);
        if(i == -1) ERROR;

        PRINT(  "   pop rax                             ; переменная %s\n", name);
        PRINT(  "   mov rbx, rbp\n");
        PRINT(  "   add rbx, %d\n", (i + 3) * 8);
        PRINT(  "   mov [rbp + %d], rax\n", (i + 3) * 8);
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
        PRINT(  "   mov rax, %d\n", node->value.constant);
        SET_BYTE(0x48);
        SET_BYTE(0xC7);
        SET_BYTE(0b11000000);
        SET_VALUE(node->value.constant);

        PRINT(  "   push rax\n");
        SET_BYTE(0x50);
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
            const char* name = node->left->value.identificator;

            int i = VariableOffcet(node);
            if(i == -1) ERROR;

            PRINT("\n   mov rbx, rbp           ; переменная %s\n", name);
            PRINT("     add rbx, %d   \n", (i + 3) * 8)
            PRINT("\n   mov rax, [rbx]\n");

            PRINT(  "   push rax\n");
            SET_BYTE(0x50);
                    
            return;
        }
        case OP_ADD:
        {
            CompileNode(node->left, file, compilator);
            CompileNode(node->right, file, compilator);

            PRINT(  "   pop rax     ; сложение\n");
            SET_BYTE(0x58);

            PRINT(  "   pop rbx\n");
            SET_BYTE(0x5B);

            PRINT(  "   add rax, rbx\n");

            PRINT(  "   push rax\n");
            SET_BYTE(0x50);
            return;
        }
        case OP_SUB:
        {
            CompileNode(node->right, file, compilator);
            CompileNode(node->left, file, compilator);

            PRINT(  "   pop rax         ; вычитание\n");
            SET_BYTE(0x58);

            PRINT(  "   pop rbx\n");
            SET_BYTE(0x5B);

            PRINT(  "   sub rax, rbx\n");

            PRINT(  "   push rax\n");
            SET_BYTE(0x50);
            return;
        }
        case OP_MUL:
        {
            CompileNode(node->right, file, compilator);
            CompileNode(node->left, file, compilator);

            PRINT(  "   pop rax             ; умножение\n");
            SET_BYTE(0x58);

            PRINT(  "   pop rbx\n");
            SET_BYTE(0x5B);

            PRINT(  "   mov rdx, 0\n");
            PRINT(  "   imul rbx\n");

            PRINT(  "   push rax\n");
            SET_BYTE(0x50);

            return;
        }
        case OP_DIV:
        {
            CompileNode(node->right, file, compilator);
            CompileNode(node->left, file, compilator);

            PRINT(  "   pop rax             ; деление\n");
            SET_BYTE(0x58);

            PRINT(  "   pop rbx\n");
            SET_BYTE(0x5B);

            PRINT(  "   mov rdx, 0\n");
            PRINT(  "   idiv rbx\n");

            PRINT(  "   push rax\n");
            SET_BYTE(0x50);

            return;
        }
        case OP_ASSIGN:
        {
            if(!node->left || !node->left->left) ERROR;

            const char* name = node->left->value.identificator;

            int i = VariableOffcet(node->left);
            if(i == -1) ERROR;

            CompileNode(node->right, file, compilator);

            PRINT(  "   pop rax             ; присваивание %s\n", name);
            SET_BYTE(0x58);

            PRINT(  "   mov rbx, rbp\n");
            PRINT(  "   add rbx, %d\n", (i + 3) * 8)
            PRINT(  "   mov [rbx], rax\n");

            return;
        }
        case OP_OUT:
        {
            CompileNode(node->left, file, compilator);
           
            PRINT(  "   call L0_OUT         ; печать в stdout\n");

            return;
        }
        case OP_IN:
        {
            PRINT(  "   call L0_IN          ; чтение из stdinma\n");

            return;
        }
        case OP_IF:
        {
            size_t lable = compilator->current_label++;

            CompileNode(node->left, file, compilator);
            PRINT(  "   pop rax\n");
            SET_BYTE(0x58);

            PRINT(  "   mov rbx, 0\n");
            PRINT(  "   cmp rax, rbx\n");

            PRINT(  "   je .L%lu\n", lable);

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

            PRINT(  "   pop rax\n");
            SET_BYTE(0x58);

            PRINT(  "   mov rbx, 0\n");
            PRINT(  "   cmp rax, rbx\n");

            PRINT(  "   je .L%lu\n", label2);

            CompileNode(node->right, file, compilator);

            PRINT(  "   jmp .L%lu\n", label1);

            PRINT(".L%lu:\n", label2);

            return;
        }
        case OP_EQUAL:
        {
            CompileNode(node->left, file, compilator);
            CompileNode(node->right, file, compilator);

            PRINT(  "   call L0_EQUAL\n");

            return;
        }
        case OP_NEQUAL:
        {
            CompileNode(node->left, file, compilator);
            CompileNode(node->right, file, compilator);

            PRINT(  "   call L0_NEQUAL\n");

            return;
        }
        case OP_SMALLER:
        {

            CompileNode(node->left, file, compilator);
            CompileNode(node->right, file, compilator);

            PRINT(  "   call L0_SMALLER\n");

            return;
        }
        case OP_BIGGER:
        {

            CompileNode(node->left, file, compilator);
            CompileNode(node->right, file, compilator);

            PRINT(  "   call L0_BIGGER\n");

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

            size_t idx = SearchFuncInNametable(compilator, name);
            if (idx >= compilator->function_count) ERROR;

            TreeNode* def = compilator->functions[idx];
            size_t callee_vars = def->parent_nametable->variable_count;

            CompileNode(node->right, file, compilator);
            
            PRINT("\n   mov rax, rbp          ; создание стекового фрейма для вызываемой функции\n");
            PRINT(  "   add rax, 8\n");
            PRINT(  "   mov rax, [rax]\n");
            PRINT(  "   mov [rax], rbp\n");

            PRINT(  "   mov rbp, rax\n");

            PRINT(  "   mov rax, rbp\n");
            PRINT(  "   add rax, 8\n");
            PRINT(  "   mov rbx, rbp\n");
            PRINT(  "   add rbx, %lu\n", (3 + callee_vars) * 8);
            PRINT(  "   mov [rax], rbx\n");
            PRINT(  "   call L_%s\n", node->left->value.identificator);

            return;
        }
        case OP_RETURN:
        {
            if(node->left)  CompileNode(node->left, file, compilator);

            PRINT("\n   mov rax, rbp             ;return\n");
            PRINT(  "   add rax, 16\n");
            PRINT(  "   mov rax, [rax]\n");
            PRINT(  "   push rax\n");
            PRINT(  "   mov rbp, [rbp]\n");

            PRINT(  "   ret\n");
            SET_BYTE(0xC3);

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

    comp->buffer = (char*)calloc(2048, sizeof(char));
    if(!comp->buffer) return NULL;
    comp->current_command = 0;

    return comp;
}

static void DestroyCompilator(Compilator* compilator)
{
    if(!compilator) return;

    free(compilator->functions);
    free(compilator->buffer);
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

static void LinkLibrary(FILE* file)
{
    assert(file);

    FILE* file_lib = fopen("source/backend/lib.s", "r+");
    if(!file_lib) return;

    char buffer = 0;

    while((buffer = fgetc(file_lib)) != EOF)
    {
        fputc(buffer, file);
    }
}