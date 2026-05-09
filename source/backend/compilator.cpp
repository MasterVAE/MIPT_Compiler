#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "tree.h"
#include "compilator.h"
#include "scope.h"
#include "binary.h"
#include "constants.h"

//#define NASM

#ifdef NASM
#define PRINT(...) fprintf(file, __VA_ARGS__); 
#define SET_BYTE(byte)
#define SET_VALUE(constant)
#define JUMP(...)
#define LABEL(...)
#define SET_DATA(array)
#else
#define PRINT(...)

#define SET_BYTE(byte)                                                                  \
    compilator->buffer[compilator->current_command++] = (char)byte;

#define SET_VALUE(constant)                                                             \
    {                                                                                   \
        int value = constant;                                                           \
        memcpy(compilator->buffer + compilator->current_command, &value, 4);            \
        compilator->current_command += 4;                                               \
    }

#define JUMP(...)                                                                       \
    {                                                                                   \
        char buffer[100];                                                               \
        sprintf(buffer, __VA_ARGS__);                                                   \
        AddJump(compilator, buffer);                                                    \
    }

#define LABEL(...)                                                                      \
    {                                                                                   \
        char buffer[100];                                                               \
        sprintf(buffer, __VA_ARGS__);                                                   \
        AddLabel(compilator, buffer);                                                   \
    }
#define SET_DATA(...)                                                                   \
    {                                                                                   \
        char arr[] = __VA_ARGS__;                                                       \
        for(size_t i = 0; i < sizeof(arr); i++)                                         \
        {                                                                               \
            SET_BYTE(arr[i]);                                                           \
        }                                                                               \
    }
#endif

#define ERROR   {                                                                       \
                    fprintf(stderr, "%s:%d Compilator error\n", __FILE__, __LINE__);    \
                    return;                                                             \
                }

static Compilator* CreateCompilator();
static void DestroyCompilator(Compilator* compilator);

static void CompileNode(TreeNode* node, FILE* file, Compilator* compilator);

static void CompileSystemCodeBefore(FILE* file, TreeNode* root, Compilator* compilator);
static void CompileSystemCodeAfter(FILE* file, Compilator* compilator);
static void CompileFunctions(FILE* file, Compilator* compilator);
static void CompileArguments(TreeNode* node, FILE* file, Compilator* compilator);
static void LinkLibrary(FILE* file);

static void AddLabel(Compilator* comp, char* name);
static void AddJump(Compilator* comp, char* name);
static void StaticJump(Compilator* comp, size_t adress);
static void CompileJumps(Compilator* comp);

static size_t SearchFuncInNametable(Compilator* compilator, const char* identificator);


static const char* const NASM_LIB_FILENAME = "source/backend/lib.s";
static const size_t REG_SIZE = 8;
static const size_t VAR_OFFSET = 3;

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

    CompileSystemCodeBefore(file, tree->root, compilator);

    CompileNode(tree->root, file, compilator);

    CompileSystemCodeAfter(file, compilator);

    CompileFunctions(file, compilator);

#ifdef NASM
    LinkLibrary(file);
#else
    CompileJumps(compilator);
    BinaryCompile("files/prog.bin", compilator);
#endif

    DestroyCompilator(compilator);

    ClearNametables(tree);
}

static void CompileSystemCodeBefore(FILE* file, TreeNode* root, Compilator* compilator)
{
    assert(file);
    assert(root);
    assert(compilator);

    PRINT("DEFAULT REL\n");
    PRINT("section     .text\n\n");
    PRINT("global main\n\n");
    PRINT("main:\n");

    PRINT("   sub rsp, 1024         ; создание базового фрейма\n");
    SET_DATA({REX_W, 0x81, 0xEC});
    SET_VALUE(1024);

    PRINT("   mov rbp, rsp\n");
    SET_DATA({REX_W, 0x89, 0xE5});

    PRINT("   add rbp, 8\n");
    SET_DATA({REX_W, 0x83, 0xC5, 0x08});

    PRINT("   mov rax, rbp\n");
    SET_DATA({REX_W, 0x89, 0xE8});

    PRINT("   add rax, 8\n");
    SET_DATA({REX_W, 0x83, 0xC0, 0x08});

    PRINT("   mov rbx, rbp\n");
    SET_DATA({REX_W, 0x89, 0xEB});

    size_t offset = (root->nametable->variable_count + VAR_OFFSET) * REG_SIZE;
    PRINT("   add rbx, %lu\n", offset);
    SET_DATA({REX_W, 0x81, 0xC3});
    SET_VALUE((int)offset);

    PRINT("   mov [rax], rbx\n");
    SET_DATA({REX_W, 0x89, 0x18});
}

static void CompileSystemCodeAfter(FILE* file, Compilator* compilator)
{
    assert(file);
    assert(compilator);

    PRINT("_end:               ; syscall выхода из программы\n");

    PRINT("   mov rax, 0x3C\n");
    SET_DATA({REX_W, 0xC7, 0xC0});
    SET_VALUE(0x3C);

    PRINT("   xor rdi, rdi\n");
    SET_DATA({REX_W, 0x31, 0xFF});

    PRINT("   syscall\n");
    SET_DATA({SYSCALL_1, SYSCALL_2});
}

static void CompileFunctions(FILE* file, Compilator* compilator)
{
    assert(file);
    assert(compilator);

    for(size_t i = 0; i < compilator->function_count; i++)
    {
        TreeNode* func = compilator->functions[i];
        PRINT("L_%s:                ; функция %s\n", func->left->value.identificator, func->left->value.identificator);
        LABEL("L_%s", func->left->value.identificator);

        PRINT(  "   pop rcx\n");
        SET_BYTE(POP + RCX);

        PRINT(  "   mov rax, rbp\n");
        SET_DATA({REX_W, 0x89, 0xE8});

        PRINT(  "   add rax, 16\n");
        SET_DATA({REX_W, 0x83, 0xC0, 0x10});

        PRINT(  "   mov [rax], rcx\n");
        SET_DATA({REX_W, 0x89, 0x08});

        CompileArguments(func->right->left, file, compilator);
        CompileNode(func->right->right, file, compilator);

        PRINT("\n   mov rax, rbp\n");
        SET_DATA({REX_W, 0x89, 0xE8});

        PRINT(  "   add rax, 16\n");
        SET_DATA({REX_W, 0x83, 0xC0, 0x10});

        PRINT(  "   mov rax, [rax]\n");
        SET_DATA({REX_W, 0x8B, 0x00});

        PRINT(  "   push rax\n");
        SET_BYTE(PUSH + RAX);

        PRINT(  "   mov rbp, [rbp]\n");
        SET_DATA({REX_W, 0x8B, 0x6D, 0x00});

        PRINT(  "   ret\n");
        SET_BYTE(RET);
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

        PRINT("   pop rax                             ; переменная %s\n", name);
        SET_BYTE(POP + RAX);

        PRINT("   mov rbx, rbp\n");
        SET_DATA({REX_W, 0x89, 0xEB});

        PRINT("   add rbx, %d\n", (i + VAR_OFFSET) * REG_SIZE);
        SET_DATA({REX_W, 0x81, 0xC3});
        SET_VALUE((i + VAR_OFFSET) * REG_SIZE);

        PRINT("   mov [rbp + %d], rax\n", (i + VAR_OFFSET) * REG_SIZE);
        SET_DATA({REX_W, 0x89, 0x85});
        SET_VALUE((i + VAR_OFFSET) * REG_SIZE);
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
        PRINT("   mov rax, %d\n", node->value.constant);
        SET_DATA({REX_W, 0xC7, 0xC0});
        SET_VALUE(node->value.constant);

        PRINT("   push rax\n");
        SET_BYTE(PUSH + RAX);
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
            SET_DATA({REX_W, 0x89, 0xEB});

            PRINT(  "   add rbx, %d   \n", (i + VAR_OFFSET) * REG_SIZE);
            SET_DATA({REX_W, 0x81, 0xC3});
            SET_VALUE((i + VAR_OFFSET) * REG_SIZE);

            PRINT(  "   mov rax, [rbx]\n");
            SET_DATA({REX_W, 0x8B, 0x03});

            PRINT(  "   push rax\n");
            SET_BYTE(PUSH + RAX);
            return;
        }
        case OP_ADD:
        {
            CompileNode(node->left, file, compilator);
            CompileNode(node->right, file, compilator);

            PRINT("   pop rax     ; сложение\n");
            SET_BYTE(POP + RAX);

            PRINT("   pop rbx\n");
            SET_BYTE(POP + RBX);

            PRINT("   add rax, rbx\n");
            SET_DATA({REX_W, 0x01, 0xD8});

            PRINT("   push rax\n");
            SET_BYTE(PUSH + RAX);
            return;
        }
        case OP_SUB:
        {
            CompileNode(node->right, file, compilator);
            CompileNode(node->left, file, compilator);

            PRINT("   pop rax         ; вычитание\n");
            SET_BYTE(POP + RAX);

            PRINT("   pop rbx\n");
            SET_BYTE(POP + RBX);

            PRINT("   sub rax, rbx\n");
            SET_DATA({REX_W, 0x29, 0xD8});

            PRINT("   push rax\n");
            SET_BYTE(PUSH + RAX);
            return;
        }
        case OP_MUL:
        {
            CompileNode(node->right, file, compilator);
            CompileNode(node->left, file, compilator);

            PRINT("   pop rax             ; умножение\n");
            SET_BYTE(POP + RAX);

            PRINT("   pop rbx\n");
            SET_BYTE(POP + RBX);

            PRINT("   mov rdx, 0\n");
            SET_DATA({REX_W, 0xC7, 0xC2});
            SET_VALUE(0);

            PRINT("   imul rbx\n");
            SET_DATA({REX_W, 0x0F, 0xAF, 0xC3});

            PRINT("   push rax\n");
            SET_BYTE(PUSH + RAX);
            return;
        }
        case OP_DIV:
        {
            CompileNode(node->right, file, compilator);
            CompileNode(node->left, file, compilator);

            PRINT("   pop rax             ; деление\n");
            SET_BYTE(POP + RAX);

            PRINT("   pop rbx\n");
            SET_BYTE(POP + RBX);

            PRINT("   mov rdx, 0\n");
            SET_DATA({REX_W, 0xC7, 0xC2});
            SET_VALUE(0);

            PRINT("   idiv rbx\n");
            SET_DATA({REX_W, 0xF7, 0xFB});

            PRINT("   push rax\n");
            SET_BYTE(PUSH + RAX);
            return;
        }
        case OP_ASSIGN:
        {
            if(!node->left || !node->left->left) ERROR;

            const char* name = node->left->left->value.identificator;

            int i = VariableOffcet(node->left);
            if(i == -1) ERROR;

            CompileNode(node->right, file, compilator);

            PRINT("   pop rax             ; присваивание %s\n", name);
            SET_BYTE(POP + RAX);

            PRINT("   mov rbx, rbp\n");
            SET_DATA({REX_W, 0x89, 0xEB});

            PRINT("   add rbx, %d\n", (i + VAR_OFFSET) * REG_SIZE);
            SET_DATA({REX_W, 0x81, 0xC3});
            SET_VALUE((i + VAR_OFFSET) * REG_SIZE);

            PRINT("   mov [rbx], rax\n");
            SET_DATA({REX_W, 0x89, 0x03});

            return;
        }
        case OP_OUT:
        {
            CompileNode(node->left, file, compilator);

            PRINT("   call L0_OUT         ; печать в stdout\n");
            SET_BYTE(CALL); SET_VALUE(PLACEHOLDER);
            StaticJump(compilator, OUT_ENTRY);


            return;
        }
        case OP_IN:
        {
            PRINT("   call L0_IN          ; чтение из stdin-a\n");
            SET_BYTE(CALL); SET_VALUE(PLACEHOLDER);
            StaticJump(compilator, IN_ENTRY);


            return;
        }
        case OP_IF:
        {
            size_t lable = compilator->current_label++;

            CompileNode(node->left, file, compilator);

            PRINT("   pop rax\n");
            SET_BYTE(POP + RAX);

            PRINT("   mov rbx, 0\n");
            SET_DATA({REX_W, 0xC7, 0xC3});
            SET_VALUE(0);

            PRINT("   cmp rax, rbx\n");
            SET_DATA({REX_W, 0x39, 0xD8});

            PRINT("   je .L%lu\n", lable);
            SET_DATA({0x0F, 0x84});
            SET_VALUE(0);
            JUMP(".L%lu", lable);

            CompileNode(node->right, file, compilator);

            PRINT(".L%lu:\n", lable);
            LABEL(".L%lu", lable);
            return;
        }
        case OP_WHILE:
        {
            size_t label1 = compilator->current_label++;
            size_t label2 = compilator->current_label++;

            PRINT(".L%lu:\n", label1);
            LABEL(".L%lu", label1);

            CompileNode(node->left, file, compilator);

            PRINT("   pop rax\n");
            SET_BYTE(POP + RAX);

            PRINT("   mov rbx, 0\n");
            SET_DATA({REX_W, 0xC7, 0xC3});
            SET_VALUE(0);

            PRINT("   cmp rax, rbx\n");
            SET_DATA({REX_W, 0x39, 0xD8});

            PRINT("   je .L%lu\n", label2);
            SET_DATA({0x0F, 0x84});
            SET_VALUE(0);
            JUMP(".L%lu", label2);

            CompileNode(node->right, file, compilator);

            PRINT("   jmp .L%lu\n", label1);
            SET_BYTE(JMP); SET_VALUE(PLACEHOLDER);
            JUMP(".L%lu", label1);

            PRINT(".L%lu:\n", label2);
            LABEL(".L%lu", label2);
            return;
        }
        case OP_EQUAL:
        {
            CompileNode(node->left, file, compilator);
            CompileNode(node->right, file, compilator);

            PRINT("   call L0_EQUAL\n");
            SET_BYTE(CALL); SET_VALUE(PLACEHOLDER);
            StaticJump(compilator, EQUAL_ENTRY);

            return;
        }
        case OP_NEQUAL:
        {
            CompileNode(node->left, file, compilator);
            CompileNode(node->right, file, compilator);

            PRINT("   call L0_NEQUAL\n");
            SET_BYTE(CALL); SET_VALUE(PLACEHOLDER);
            StaticJump(compilator, NEQUAL_ENTRY);
            return;
        }
        case OP_SMALLER:
        {
            CompileNode(node->left, file, compilator);
            CompileNode(node->right, file, compilator);

            PRINT("   call L0_SMALLER\n");
            SET_BYTE(CALL); SET_VALUE(PLACEHOLDER);
            StaticJump(compilator, SMALLER_ENTRY);
            return;
        }
        case OP_BIGGER:
        {
            CompileNode(node->left, file, compilator);
            CompileNode(node->right, file, compilator);

            PRINT("   call L0_BIGGER\n");
            SET_BYTE(CALL); SET_VALUE(PLACEHOLDER);
            StaticJump(compilator, BIGGER_ENTRY);
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
            SET_DATA({REX_W, 0x89, 0xE8});

            PRINT(  "   add rax, 8\n");
            SET_DATA({REX_W, 0x83, 0xC0, 0x08});

            PRINT(  "   mov rax, [rax]\n");
            SET_DATA({REX_W, 0x8B, 0x00});
            PRINT(  "   mov [rax], rbp\n");
            SET_DATA({REX_W, 0x89, 0x28});

            PRINT(  "   mov rbp, rax\n");
            SET_DATA({REX_W, 0x89, 0xC5});

            PRINT(  "   mov rax, rbp\n");
            SET_DATA({REX_W, 0x89, 0xE8});

            PRINT(  "   add rax, 8\n");
            SET_DATA({REX_W, 0x83, 0xC0, 0x08});

            PRINT(  "   mov rbx, rbp\n");
            SET_DATA({REX_W, 0x89, 0xEB});

            PRINT(  "   add rbx, %lu\n", (3 + callee_vars) * REG_SIZE);
            SET_DATA({REX_W, 0x81, 0xC3});
            SET_VALUE((int)((3 + callee_vars) * REG_SIZE));

            PRINT(  "   mov [rax], rbx\n");
            SET_DATA({REX_W, 0x89, 0x18});

            PRINT(  "   call L_%s\n", node->left->value.identificator);
            SET_BYTE(CALL); SET_VALUE(PLACEHOLDER);
            JUMP("L_%s", node->left->value.identificator);

            return;
        }
        case OP_RETURN:
        {
            if(node->left)  CompileNode(node->left, file, compilator);

            PRINT("\n   mov rax, rbp             ;return\n");
            SET_DATA({REX_W, 0x89, 0xE8});

            PRINT(  "   add rax, 16\n");
            SET_DATA({REX_W, 0x83, 0xC0, 0x10});

            PRINT(  "   mov rax, [rax]\n");
            SET_DATA({REX_W, 0x8B, 0x00});

            PRINT(  "   push rax\n");
            SET_BYTE(PUSH + RAX);

            PRINT(  "   mov rbp, [rbp]\n");
            SET_DATA({REX_W, 0x8B, 0x6D, 0x00});

            PRINT(  "   ret\n");
            SET_BYTE(RET);

            return;
        }
        case OP_ARGUMENT:
        {
            CompileNode(node->right, file, compilator);
            CompileNode(node->left, file, compilator);
            return;
        }
        case OP_SET:
        {
            CompileNode(node->left, file, compilator);
            PRINT("   call L0_SET          ; запись в буффер \n");
            SET_BYTE(CALL); SET_VALUE(PLACEHOLDER);
            StaticJump(compilator, SET_ENTRY);
            return;
        }
        case OP_DRAW:
        {
            PRINT("   call L0_DRAW         ; печать буффера \n");
            SET_BYTE(CALL); SET_VALUE(PLACEHOLDER);
            StaticJump(compilator, DRAW_ENTRY);
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

    comp->buffer = (char*)calloc(0x2000, sizeof(char));
    if(!comp->buffer) return NULL;
    comp->current_command = 0;

    comp->lables = (Label*)calloc(1, sizeof(Label));
    comp->lable_count = 0;

    comp->jump_count = 0;
    comp->jumps = (Jump*)calloc(1, sizeof(Jump));

    return comp;
}

static void DestroyCompilator(Compilator* compilator)
{
    if(!compilator) return;

    for(size_t i = 0; i < compilator->jump_count; i++)
    {
        free(compilator->jumps[i].name);
    }
    free(compilator->jumps);

    for(size_t i = 0; i < compilator->lable_count; i++)
    {
        free(compilator->lables[i].name);
    }
    free(compilator->lables);

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

    FILE* file_lib = fopen(NASM_LIB_FILENAME, "rb");
    if (!file_lib) return;

    fseek(file_lib, 0, SEEK_END);
    size_t fsize = (size_t)ftell(file_lib);

    fseek(file_lib, 0, SEEK_SET);

    char* buffer = (char*)calloc(fsize, sizeof(char));
    if (!buffer) 
    {
        fclose(file_lib);
        return;
    }

    size_t bytes_read = fread(buffer, 1, fsize, file_lib);
    fwrite(buffer, 1, bytes_read, file);

    free(buffer);
    fclose(file_lib);
}


static void AddLabel(Compilator* comp, char* name)
{
    assert(comp);
    assert(name);

    comp->lable_count++;
    size_t i = comp->lable_count - 1;

    comp->lables = (Label*)realloc(comp->lables, sizeof(Label) * comp->lable_count);

    comp->lables[i].command_offset = comp->current_command;
    comp->lables[i].name = strdup(name);
}

static void AddJump(Compilator* comp, char* name)
{
    assert(comp);
    assert(name);

    comp->jump_count++;
    size_t i = comp->jump_count - 1;

    comp->jumps = (Jump*)realloc(comp->jumps, sizeof(Jump) * comp->jump_count);

    size_t offset = comp->current_command;

    comp->jumps[i].name = strdup(name);
    comp->jumps[i].command_offset = offset;
    comp->jumps[i].write_offset = offset - 4;
}

#ifdef NASM
static void StaticJump(Compilator* comp, size_t adress) {}
#else
static void StaticJump(Compilator* comp, size_t adress)
{
    assert(comp);
    size_t offset = adress - comp->current_command - 0x1000;
    memcpy(comp->buffer + comp->current_command - 4, &offset, 4);
}
#endif

static void CompileJumps(Compilator* comp)
{
    assert(comp);
    for(size_t i = 0; i < comp->jump_count; i++)
    {
        int label = -1;
        for(size_t j = 0; j < comp->lable_count; j++)
        {
            if(!strcmp(comp->lables[j].name, comp->jumps[i].name))
            {
                label = (int)j;
                break;
            }
        }

        if(label == -1) ERROR;

        int jump_offset = (int)comp->lables[label].command_offset - (int)comp->jumps[i].command_offset;
        memcpy(comp->buffer + comp->jumps[i].write_offset, &jump_offset, 4);
    }
}