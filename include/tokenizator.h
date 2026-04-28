#ifndef TOKENIZATOR_H
#define TOKENIZATOR_H

#include <stdlib.h>

#include "tree.h"

struct Token
{
    size_t line;
    NodeType type;
    NodeValue value;
};

struct Program
{
    Token* tokens;
    size_t token_count; 
    size_t current_token;
};

Program* Tokenize(const char* filename);

void DestroyProg(Program* prog);

#endif // TOKENIZATOR_H