#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "tokenizator.h"
#include "op_data.h"
#include "library.h"
#include "tree.h"

static Program* Read(Program* prog, const char* buffer);
static size_t SkipSpaces(const char** buffer, bool isComment);
static Program* AddToken(Program* prog, size_t line, NodeType type, NodeValue value);

static const char* const COMMENT = "#";

#define SKIP line = SkipSpaces(&buffer, false);

static Program* CreateProgram();

static Program* CreateProgram()
{
    Program* prog = (Program*)calloc(1, sizeof(Program));
    if(!prog) return NULL;

    prog->token_count = 0;
    prog->current_token = 0;
    prog->tokens = (Token*)calloc(1, sizeof(Token));

    if(!prog->tokens) 
    {   
        free(prog);
        return NULL;
    }

    return prog;
}

Program* Tokenize(const char* filename)
{
    assert(filename);

    char* buffer = ReadFile(filename);
    if(!buffer) return NULL;

    Program* prog = CreateProgram();
    if(!prog) 
    {
        free(buffer);
        return NULL;
    }

    Read(prog, buffer);

    free(buffer);
    return prog;
}

static Program* Read(Program* prog, const char* buffer)
{
    assert(prog);
    assert(buffer);

    size_t line = 0;

    while(*buffer != '\0')
    {
        SKIP;
// FIXME 2 cycles
        if(!strncmp(buffer, COMMENT, strlen(COMMENT)))
        {
            SkipSpaces(&buffer, true);
            continue;
        }

        int value = 0;
        int count = 0;
        sscanf(buffer, "%d%n", &value, &count);

        buffer += count;

        if(count > 0)
        {
            prog = AddToken(prog, line, NODE_CONSTANT, NodeValue {.constant = value});
            if(!prog) return NULL;
            
            SKIP;
            continue;
        }


        bool found = false;

        size_t op_ind = GetOpByCode(buffer);
        if(op_ind < OP_COUNT)
        {
            Oper data = OP_DATA[op_ind];

            prog = AddToken(prog, line, NODE_OPERATION, NodeValue {.operation = data.op});
            if(!prog) return NULL;
        
            SKIP;
            found = true;
            buffer += strlen(data.op_code);
        }

        if(found) continue;

        size_t len = 0;
        const char* start = buffer;
        while(!IsSystem(*buffer))
        {
            len += 1;
            buffer++;
        }

        if(len > 0)
        {
            prog = AddToken(prog, line, NODE_IDENTIFICATOR, 
                                        NodeValue {.identificator = strndup(start, len)});
            if(!prog) return NULL;
        }
        else if(*buffer != '\0')
        {
            fprintf(stderr, "Unknown character %s\n", buffer);
        }

        SKIP;
    }

    return prog;
}

static Program* AddToken(Program* prog, size_t line, NodeType type, NodeValue value)
{
    assert(prog);

    prog->token_count++;

    Token* new_tokens = (Token*)realloc(prog->tokens, prog->token_count * sizeof(Token));
    if(!new_tokens) return NULL;

    prog->tokens = new_tokens;

    Token* token = &prog->tokens[prog->token_count - 1];
    token->line = line;
    token->type = type;
    token->value = value;

    return prog;
}

static size_t SkipSpaces(const char** buffer, bool isComment)
{
    assert(buffer);

    static size_t line = 0;

    if(isComment)
    {
        while(**buffer != '\n') (*buffer)++;

        (*buffer)++;
        line++;
        return line;
    }

    while(isspace(**buffer))
    {
        if(**buffer == '\n')
        {
            line++;
        }

        (*buffer)++;
    }

    return line;
}

void DestroyProg(Program* prog)
{
    if(!prog) return;
    if(prog->tokens)
    {
        for(size_t i = 0; i < prog->token_count; i++)
        {
            if(prog->tokens[i].type == NODE_IDENTIFICATOR)
            {
                free(prog->tokens[i].value.identificator);
            }
        }
    }

    free(prog->tokens);
    free(prog);
}