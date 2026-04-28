#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "op_data.h"

const char* GetOpName(Operation operation)
{
    for(size_t op = 0; op < OP_COUNT; op++)
    {
        if(OP_DATA[op].op == operation)
        {
            return OP_DATA[op].op_name;
        }
    }

    return NULL;
}

const char* GetOpColor(Operation operation)
{
    for(size_t op = 0; op < OP_COUNT; op++)
    {
        if(OP_DATA[op].op == operation)
        {
            return OP_DATA[op].color;
        }
    }

    return NULL;
}

size_t GetOpByName(const char* name)
{
    assert(name);

    size_t i = 0;

    for(; i < OP_COUNT; i++)
    {
        Oper data = OP_DATA[i];
        if(!data.op_name) continue;

        if(!strncmp(name, data.op_name, strlen(data.op_name)))
        {
            return i;
        }
    }

    return i;
}

size_t GetOpByCode(const char* code)
{
    assert(code);

    size_t i = 0;

    for(; i < OP_COUNT; i++)
    {
        Oper data = OP_DATA[i];
        if(!data.op_code) continue;

        if(!strncmp(code, data.op_code, strlen(data.op_code)))
        {
            return i;
        }
    }

    return i;
}

bool IsSystem(char symbol)
{   
    for(size_t i = 0; i < SYSTEM_SYMBOLS_COUNT; i++)
    {
        if(symbol == SYSTEM_SYMBOLS[i])
        {
            return true;
        }
    }

    return false;
}