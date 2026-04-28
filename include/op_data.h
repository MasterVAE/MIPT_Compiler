#ifndef OP_DATA_H
#define OP_DATA_H

#include "tree.h"
#include "stdlib.h"

struct Oper
{
    Operation op;
    const char* op_name;
    const char* op_code;
    const char* color;
};

static Oper OP_DATA[] = 
{
    {OP_ADD,            "+",        "+",        "goldenrod3"},
    {OP_SUB,            "-",        "-",        "goldenrod3"},
    {OP_MUL,            "*",        "*",        "goldenrod3"},
    {OP_DIV,            "/",        "/",        "goldenrod3"},
    {OP_BIGGER,         "BIGGER",   ">",        "bisque4"},
    {OP_SMALLER,        "SMALLER",  "<",        "bisque4"},
    {OP_EQUAL,          "EQUAL",    "==",       "bisque4"},
    {OP_NEQUAL,         "NOT_EQUAL","!=",       "bisque4"},
    {OP_ASSIGN,         "ASSIGN",   "=",        "darkolivegreen3"},
    {OP_WHILE,          "WHILE",    "while",    "darkolivegreen3"},
    {OP_IF,             "IF",       "if",       "darkolivegreen3"},
    {OP_IN,             "IN",       "in",       "darkolivegreen3"},
    {OP_OUT,            "OUT",      "out",      "darkolivegreen3"},
    {OP_RETURN,         "RETURN",   "return",   "darkolivegreen3"},
    {OP_BRACKET_OPEN,   NULL,       "("},
    {OP_BRACKET_CLOSE,  NULL,       ")"},
    {OP_FBRACKET_OPEN,  NULL,       "{"},
    {OP_FBRACKET_CLOSE, NULL,       "}"},
    {OP_COMMA,          NULL,       ","},
    {OP_LINE,           "LINE",     ";",        "aliceblue"},
    {OP_EMPTY,          "EMPTY",    NULL,       "aliceblue"},
    {OP_VARIABLE,       "VARIABLE", NULL,       "aliceblue"},
    {OP_FUNCTION,       "FUNCTION", NULL,       "aliceblue"},
    {OP_ARGUMENT,       "ARGUMENT", NULL,       "aliceblue"}
};

const size_t OP_COUNT = sizeof(OP_DATA)/sizeof(OP_DATA[0]);

const char SYSTEM_SYMBOLS[] = {'(', ')', '{', '}', ';', '<', '>', '!', '=', '-', '+', '*', '/', 
                               ',', ' ', '\0'};

const size_t SYSTEM_SYMBOLS_COUNT = sizeof(SYSTEM_SYMBOLS)/sizeof(SYSTEM_SYMBOLS[0]);

const char* GetOpName(Operation op);
const char* GetOpColor(Operation operation);

size_t GetOpByName(const char* name);
size_t GetOpByCode(const char* code);

bool IsSystem(char symbol);

#endif // OP_DATA_H