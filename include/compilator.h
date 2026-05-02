#ifndef COMPILATOR_H
#define COMPILATOR_H

#include <stdio.h>

#include "tree.h"

struct Label
{
    size_t command_offset;
    char* name;
};

struct Jump
{
    size_t command_offset;
    size_t write_offset;
    char* name;
};

struct Compilator
{
    size_t current_label;

    TreeNode** functions;
    size_t function_count;

    size_t current_command;
    char* buffer;

    Label* lables;
    size_t lable_count;

    Jump* jumps;
    size_t jump_count;
};

void CompileTree(Tree* tree, FILE* file);

#endif // COMPILATOR_H