#ifndef COMPILATOR_H
#define COMPILATOR_H

#include <stdio.h>

#include "tree.h"

struct Compilator
{
    size_t current_label;

    TreeNode** functions;
    size_t function_count;
};

void CompileTree(Tree* tree, FILE* file);

#endif // COMPILATOR_H