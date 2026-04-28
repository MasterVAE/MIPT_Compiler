#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>

#include "library.h"

static size_t FileLength(FILE* file);

char* ReadFile(const char* filename)
{
    assert(filename);

    FILE* file = fopen(filename, "r+");
    if(!file) return NULL;

    size_t lenght = FileLength(file);
    char* buffer = (char*)calloc(lenght + 1, sizeof(char));
    if(!buffer) return NULL;

    fread(buffer, lenght, 1, file);
    buffer[lenght] = '\0';

    return buffer;
}

static size_t FileLength(FILE* file)
{
    assert(file);

    fseek(file, 0, SEEK_END);
    size_t filesize = (size_t)ftell(file);
    fseek(file, 0, SEEK_SET);

    return filesize;
}

char* SkipSpaces(char* buffer)
{
    while (isspace(*buffer)) buffer++;
    
    return buffer;
}