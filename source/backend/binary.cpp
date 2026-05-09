#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <elf.h>

#include "compilator.h"
#include "binary.h"
#include "constants.h"


void InsertHeader(char* buffer);
void InsertProg(char* buffer, Compilator* compilator);
void InsertData(char* buffer);
void InsertSections(char* buffer, Compilator* compilator);

static void Disasm(char* buffer);
static void DisasmFile(char* buffer, const char* filename, size_t size);


struct Elf64_Ehdr_my
{
    unsigned char e_ident[16];  // магическая сигнатура и информация о классе, порядке байт, версии ELF
    uint16_t      e_type;       // тип объектного файла (исполняемый, разделяемая библиотека и т.д.)
    uint16_t      e_machine;    // целевая архитектура процессора (x86-64, ARM и т.п.)
    uint32_t      e_version;    // версия ELF-заголовка (обычно 1)
    Elf64_Addr    e_entry;      // виртуальный адрес точки входа в программу
    Elf64_Off     e_phoff;      // смещение в файле до таблицы программных заголовков
    Elf64_Off     e_shoff;      // смещение в файле до таблицы заголовков секций
    uint32_t      e_flags;      // специфичные для процессора флаги
    uint16_t      e_ehsize;     // размер самого ELF-заголовка в байтах
    uint16_t      e_phentsize;  // размер одного элемента таблицы программных заголовков
    uint16_t      e_phnum;      // количество элементов в таблице программных заголовков
    uint16_t      e_shentsize;  // размер одного элемента таблицы заголовков секций
    uint16_t      e_shnum;      // количество элементов в таблице заголовков секций
    uint16_t      e_shstrndx;   // индекс секции, содержащей строки имён секций
};

struct Elf64_Phdr_my 
{
    uint32_t   p_type;    // тип сегмента (загружаемый, динамический, примечания и т.д.)
    uint32_t   p_flags;   // флаги доступа к сегменту (чтение, запись, исполнение)
    Elf64_Off  p_offset;  // смещение сегмента в файле
    Elf64_Addr p_vaddr;   // виртуальный адрес, по которому сегмент загружается в память
    Elf64_Addr p_paddr;   // физический адрес сегмента (используется редко)
    uint64_t   p_filesz;  // размер сегмента в файловом образе (может быть меньше p_memsz)
    uint64_t   p_memsz;   // размер сегмента в памяти (неинициализированные данные могут быть больше)
    uint64_t   p_align;   // требование выравнивания сегмента (степень двойки)
};

struct Elf64_Shdr_my
{
    uint32_t   sh_name;       // индекс в строковой таблице секций, хранящий имя секции
    uint32_t   sh_type;       // тип секции (данные, символы, релокейшены и т.д.)
    uint64_t   sh_flags;      // атрибуты секции (запись, выделение памяти, исполнение)
    Elf64_Addr sh_addr;       // виртуальный адрес секции в памяти (если загружается)
    Elf64_Off  sh_offset;     // смещение содержимого секции в файле
    uint64_t   sh_size;       // размер секции в байтах
    uint32_t   sh_link;       // индекс связанной секции (зависит от типа секции)
    uint32_t   sh_info;       // дополнительная информация, зависящая от типа секции
    uint64_t   sh_addralign;  // требование выравнивания адреса секции (степень двойки)
    uint64_t   sh_entsize;    // размер одного элемента, если секция содержит таблицу фиксированных записей
};

void BinaryCompile(const char* filename, Compilator* compilator)
{
    assert(filename);

    FILE* binary_file = fopen(filename, "w+");
    if(!binary_file) return;

    char* buffer = (char*)calloc(FILESIZE, sizeof(char));

    InsertHeader(buffer);
    InsertProg(buffer, compilator);
    InsertData(buffer);
    InsertSections(buffer, compilator);

    Disasm(buffer);

    fwrite(buffer, FILESIZE, 1, binary_file);
    free(buffer);

    fclose(binary_file);
}

void InsertHeader(char* buffer)
{
    Elf64_Ehdr_my elf_header = 
    {
        .e_ident = {0x7f, 'E', 'L', 'F', 2, 1, 1, 0},
        .e_type = 2,
        .e_machine = 62,
        .e_version = 1,
        .e_entry = VADRESS + ENTRY_POINT,
        .e_phoff = 64,
        .e_shoff = DATA_POINT + 0x100,
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
        .p_vaddr = VADRESS,
        .p_paddr = 0,
        .p_filesz = DATA_POINT,
        .p_memsz = DATA_POINT,
        .p_align = ALIGNMENT
    };

    Elf64_Phdr_my data_header = 
    {
        .p_type = 1,
        .p_flags = 6,
        .p_offset = DATA_POINT,
        .p_vaddr = VADRESS + DATA_POINT,
        .p_paddr = 0,
        .p_filesz = 102,
        .p_memsz = 166,
        .p_align = ALIGNMENT
    };

    memcpy(buffer, &elf_header, sizeof(elf_header));
    memcpy(buffer + sizeof(elf_header), &text_header, sizeof(text_header));
    memcpy(buffer + sizeof(elf_header) + sizeof(text_header), &data_header, sizeof(data_header));
}

void InsertProg(char* buffer, Compilator* compilator)
{
    assert(buffer);
    assert(compilator);

    printf("Code %lu/%d\n", compilator->current_command, DATA_POINT - ENTRY_POINT);
    memcpy(buffer + ENTRY_POINT, compilator->buffer, DATA_POINT - ENTRY_POINT);
}

void InsertData(char* buffer)
{
    assert(buffer);

    unsigned char data_bytes[102];
    data_bytes[0] = 0x30;
    data_bytes[1] = 0x0A;
    for (int i = 2; i < 102; ++i)
        data_bytes[i] = '_';

    memcpy(buffer + DATA_POINT, data_bytes, sizeof(data_bytes));

    const char* shstrtab = "\0.shstrtab\0.text\0.data\0.bss\0";
    size_t shstrtab_size = 28;
    memcpy(buffer + DATA_POINT + 0x66, shstrtab, shstrtab_size);
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
        .sh_addr      = VADRESS + ENTRY_POINT,
        .sh_offset    = ENTRY_POINT,
        .sh_size      = DATA_POINT - ENTRY_POINT,
        .sh_link      = 0,
        .sh_info      = 0,
        .sh_addralign = 16,
        .sh_entsize   = 0
    };

    Elf64_Shdr_my shdr_data = {
        .sh_name      = 17,
        .sh_type      = 1,
        .sh_flags     = 0x3,
        .sh_addr      = VADRESS + DATA_POINT,
        .sh_offset    = DATA_POINT,
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
        .sh_addr      = VADRESS + DATA_POINT + 0x10,
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
        .sh_offset    = DATA_POINT + 0x66,
        .sh_size      = 28,
        .sh_link      = 0,
        .sh_info      = 0,
        .sh_addralign = 1,
        .sh_entsize   = 0
    };

    int shoff = 0x3100;
    memcpy(buffer + shoff, &shdr_null, 64);
    memcpy(buffer + shoff + 64, &shdr_text, 64);
    memcpy(buffer + shoff + 128, &shdr_data, 64);
    memcpy(buffer + shoff + 192, &shdr_bss, 64);
    memcpy(buffer + shoff + 256, &shdr_shstrtab, 64);
}

static void Disasm(char* buffer)
{
    assert(buffer);

    DisasmFile(buffer + BIGGER_ENTRY,   "source/backend/lib/bigger", 0x100);
    DisasmFile(buffer + SMALLER_ENTRY,  "source/backend/lib/smaller", 0x100);
    DisasmFile(buffer + EQUAL_ENTRY,    "source/backend/lib/equal", 0x100);
    DisasmFile(buffer + NEQUAL_ENTRY,   "source/backend/lib/nequal", 0x100);
    DisasmFile(buffer + SET_ENTRY,      "source/backend/lib/set", 0x200);
    DisasmFile(buffer + DRAW_ENTRY,     "source/backend/lib/draw", 0x200);
    DisasmFile(buffer + OUT_ENTRY,      "source/backend/lib/out", 0x300);
    DisasmFile(buffer + IN_ENTRY,       "source/backend/lib/in", 0x300);
}

static void DisasmFile(char* buffer, const char* filename, size_t size)
{
    assert(filename);
    assert(buffer);

    FILE* file = fopen(filename, "rb+");
    if(!file) return;

    fseek(file, ENTRY_POINT, SEEK_SET);
    fread(buffer, size, 1, file);

    fclose(file);
}