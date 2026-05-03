#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <elf.h>

#include "compilator.h"
#include "nasm.h"


void InsertHeader(char* buffer, Compilator* compilator);
void InsertProg(char* buffer, Compilator* compilator);
void InsertData(char* buffer);
void InsertSections(char* buffer, Compilator* compilator);

static void Disasm(char* buffer);
static void DisasmFile(char* buffer, const char* filename, size_t size);


struct  Elf64_Ehdr_my
{
    unsigned char e_ident[16];
    uint16_t      e_type;
    uint16_t      e_machine;
    uint32_t      e_version;
    Elf64_Addr    e_entry;
    Elf64_Off     e_phoff;
    Elf64_Off     e_shoff;
    uint32_t      e_flags;
    uint16_t      e_ehsize;
    uint16_t      e_phentsize;
    uint16_t      e_phnum;
    uint16_t      e_shentsize;
    uint16_t      e_shnum;
    uint16_t      e_shstrndx;
};

struct Elf64_Phdr_my 
{
    uint32_t   p_type;
    uint32_t   p_flags;
    Elf64_Off  p_offset;
    Elf64_Addr p_vaddr;
    Elf64_Addr p_paddr;
    uint64_t   p_filesz;
    uint64_t   p_memsz;
    uint64_t   p_align;
};


struct Elf64_Shdr_my
{
    uint32_t   sh_name;
    uint32_t   sh_type;
    uint64_t   sh_flags;
    Elf64_Addr sh_addr;
    Elf64_Off  sh_offset;
    uint64_t   sh_size;
    uint32_t   sh_link;
    uint32_t   sh_info;
    uint64_t   sh_addralign;
    uint64_t   sh_entsize;
};

void NasmCompile(const char* filename, Compilator* compilator)
{
    assert(filename);

    FILE* binary_file = fopen(filename, "w+");
    if(!binary_file) return;

    char* buffer = (char*)calloc(0x4000, sizeof(char));

    InsertHeader(buffer, compilator);
    InsertProg(buffer, compilator);
    InsertData(buffer);
    InsertSections(buffer, compilator);

    Disasm(buffer);

    fwrite(buffer, 0x4000, 1, binary_file);
    free(buffer);

    fclose(binary_file);
}

void InsertHeader(char* buffer, Compilator* compilator)
{
    Elf64_Ehdr_my elf_header = 
    {
        .e_ident = {0x7f, 'E', 'L', 'F', 2, 1, 1, 0},
        .e_type = 2,
        .e_machine = 62,
        .e_version = 1,
        .e_entry = 0x401000,
        .e_phoff = 64,
        .e_shoff = 0x2100,
        .e_flags = 0,
        .e_ehsize = 64,
        .e_phentsize = 56,
        .e_phnum = 2,
        .e_shentsize = 64,
        .e_shnum = 5,
        .e_shstrndx = 4
    };

    Elf64_Phdr_my text_header = 
    {
        .p_type = 1,
        .p_flags = 5,
        .p_offset = 0,
        .p_vaddr = 0x400000,
        .p_paddr = 0,
        .p_filesz = 0x2000,
        .p_memsz = 0x2000,
        .p_align = 0x1000
    };

    Elf64_Phdr_my data_header = 
    {
        .p_type = 1,
        .p_flags = 6,
        .p_offset = 0x2000,
        .p_vaddr = 0x402000,
        .p_paddr = 0,
        .p_filesz = 102,
        .p_memsz = 166,
        .p_align = 0x1000
    };

    memcpy(buffer, &elf_header, sizeof(elf_header));
    memcpy(buffer + 0x40, &text_header, sizeof(text_header));
    memcpy(buffer + 0x78, &data_header, sizeof(data_header));
}

void InsertProg(char* buffer, Compilator* compilator)
{
    assert(buffer);
    assert(compilator);

    printf("Code %lu/%d\n", compilator->current_command, 0x1000);
    memcpy(buffer + 0x1000, compilator->buffer, 0x1000);
}

void InsertData(char* buffer)
{
    assert(buffer);

    unsigned char data_bytes[102];
    data_bytes[0] = 0x30;
    data_bytes[1] = 0x0A;
    for (int i = 2; i < 102; ++i)
        data_bytes[i] = '_';

    memcpy(buffer + 0x2000, data_bytes, sizeof(data_bytes));

    const char* shstrtab = "\0.shstrtab\0.text\0.data\0.bss\0";
    size_t shstrtab_size = 28;
    memcpy(buffer + 0x2066, shstrtab, shstrtab_size);
}

void InsertSections(char* buffer, Compilator* compilator)
{
    assert(buffer);
    assert(compilator);

    Elf64_Shdr_my shdr_null = {0};

    Elf64_Shdr_my shdr_text = {
        .sh_name      = 11,
        .sh_type      = 1,
        .sh_flags     = 0x6,
        .sh_addr      = 0x401000,
        .sh_offset    = 0x1000,
        .sh_size      = 0x1000,
        .sh_link      = 0,
        .sh_info      = 0,
        .sh_addralign = 16,
        .sh_entsize   = 0
    };

    Elf64_Shdr_my shdr_data = {
        .sh_name      = 17,
        .sh_type      = 1,
        .sh_flags     = 0x3,
        .sh_addr      = 0x402000,
        .sh_offset    = 0x2000,
        .sh_size      = 102,
        .sh_link      = 0,
        .sh_info      = 0,
        .sh_addralign = 16,
        .sh_entsize   = 0
    };

    Elf64_Shdr_my shdr_bss = {
        .sh_name      = 23,
        .sh_type      = 8,
        .sh_flags     = 0x3,
        .sh_addr      = 0x402010,
        .sh_offset    = 0,
        .sh_size      = 64,
        .sh_link      = 0,
        .sh_info      = 0,
        .sh_addralign = 16,
        .sh_entsize   = 0
    };

    Elf64_Shdr_my shdr_shstrtab = {
        .sh_name      = 1,
        .sh_type      = 3,
        .sh_flags     = 0,
        .sh_addr      = 0,
        .sh_offset    = 0x2066,
        .sh_size      = 28,
        .sh_link      = 0,
        .sh_info      = 0,
        .sh_addralign = 1,
        .sh_entsize   = 0
    };

    int shoff = 0x2100;
    memcpy(buffer + shoff, &shdr_null, 64);
    memcpy(buffer + shoff + 64, &shdr_text, 64);
    memcpy(buffer + shoff + 128, &shdr_data, 64);
    memcpy(buffer + shoff + 192, &shdr_bss, 64);
    memcpy(buffer + shoff + 256, &shdr_shstrtab, 64);
}

static void Disasm(char* buffer)
{
    assert(buffer);

    DisasmFile(buffer + 0x1200, "source/backend/lib/bigger", 0x100);
    DisasmFile(buffer + 0x1300, "source/backend/lib/smaller", 0x100);
    DisasmFile(buffer + 0x1400, "source/backend/lib/equal", 0x100);
    DisasmFile(buffer + 0x1500, "source/backend/lib/nequal", 0x100);
    DisasmFile(buffer + 0x1600, "source/backend/lib/set", 0x200);
    DisasmFile(buffer + 0x1800, "source/backend/lib/draw", 0x200);
    DisasmFile(buffer + 0x1A00, "source/backend/lib/out", 0x300);
    DisasmFile(buffer + 0x1D00, "source/backend/lib/in", 0x300);
}

static void DisasmFile(char* buffer, const char* filename, size_t size)
{
    assert(filename);
    assert(buffer);

    FILE* file = fopen(filename, "rb+");
    if(!file) return;

    fseek(file, 0x1000, SEEK_SET);
    fread(buffer, size, 1, file);

    fclose(file);
}