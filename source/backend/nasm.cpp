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
    unsigned char e_ident[16]; // Магическое число и информация о файле
    uint16_t      e_type;      // Тип файла (исполняемый, объектный и т.д.)
    uint16_t      e_machine;   // Архитектура (например, x86-64)
    uint32_t      e_version;   // Версия формата
    Elf64_Addr    e_entry;     // Адрес точки входа
    Elf64_Off     e_phoff;     // Смещение до Program Header Table
    Elf64_Off     e_shoff;     // Смещение до Section Header Table
    uint32_t      e_flags;     // Флаги, зависящие от архитектуры
    uint16_t      e_ehsize;    // Размер самого ELF-заголовка
    uint16_t      e_phentsize; // Размер одной записи в Program Header Table
    uint16_t      e_phnum;     // Количество записей в Program Header Table
    uint16_t      e_shentsize; // Размер одной записи в Section Header Table
    uint16_t      e_shnum;     // Количество записей в Section Header Table
    uint16_t      e_shstrndx;  // Индекс секции, содержащей строковую таблицу имен секций
};

struct Elf64_Phdr_my 
{
    uint32_t   p_type;   // Тип сегмента (например, PT_LOAD)
    uint32_t   p_flags;  // Права доступа (чтение, запись, исполнение)
    Elf64_Off  p_offset; // Смещение в файле
    Elf64_Addr p_vaddr;  // Виртуальный адрес при загрузке
    Elf64_Addr p_paddr;  // Физический адрес (обычно игнорируется)
    uint64_t   p_filesz; // Размер сегмента в файле
    uint64_t   p_memsz;  // Размер сегмента в памяти
    uint64_t   p_align;  // Выравнивание
};


struct Elf64_Shdr_my
{
    uint32_t   sh_name;      // Индекс имени секции в строковой таблице .shstrtab
    uint32_t   sh_type;      // Тип секции (SHT_PROGBITS, SHT_SYMTAB и т.д.)
    uint64_t   sh_flags;     // Атрибуты секции (write, alloc, exec)
    Elf64_Addr sh_addr;      // Виртуальный адрес секции в памяти (если применимо)
    Elf64_Off  sh_offset;    // Смещение начала данных секции от начала файла
    uint64_t   sh_size;      // Размер секции в байтах
    uint32_t   sh_link;      // Ссылка на другую секцию (зависит от типа)
    uint32_t   sh_info;      // Дополнительная информация (зависит от типа)
    uint64_t   sh_addralign; // Требования к выравниванию
    uint64_t   sh_entsize;   // Размер одной записи, если секция содержит таблицу (например, в .symtab)
};

void NasmCompile(const char* filename, Compilator* compilator)
{
    assert(filename);

    FILE* binary_file = fopen(filename, "w+");
    if(!binary_file) return;

    char* buffer = (char*)calloc(0x2168, sizeof(char));


    InsertHeader(buffer, compilator);
    InsertProg(buffer, compilator);
    InsertData(buffer);
    InsertSections(buffer, compilator);

    Disasm(buffer);

    fwrite(buffer, 0x2168, 1, binary_file);
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
        .e_shoff = 0x2028,
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
        .p_filesz = 12,
        .p_memsz = 0x50,
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
    // Данные .data
    unsigned char data_bytes[] = {
        0x30,                         // '0'
        0x0A,                         // '\n'
        '0','1','2','3','4','5','6','7','8','9'  // "0123456789"
    };
    memcpy(buffer + 0x2000, data_bytes, sizeof(data_bytes));

    const char* shstrtab = "\0.shstrtab\0.text\0.data\0.bss\0";
    size_t shstrtab_size = 28;
    memcpy(buffer + 0x200C, shstrtab, shstrtab_size);   
}

void InsertSections(char* buffer, Compilator* compilator)
{
    assert(buffer);
    assert(compilator);

    Elf64_Shdr_my shdr_null = {0}; // нулевая секция

    Elf64_Shdr_my shdr_text = {
        .sh_name      = 11,   // индекс ".text" в .shstrtab
        .sh_type      = 1,    // SHT_PROGBITS
        .sh_flags     = 0x6,  // SHF_ALLOC | SHF_EXECINSTR (2+4=6)
        .sh_addr      = 0x401000,
        .sh_offset    = 0x1000,
        .sh_size      = 0x1000,    // code_size was 9
        .sh_link      = 0,
        .sh_info      = 0,
        .sh_addralign = 16,
        .sh_entsize   = 0
    };

    Elf64_Shdr_my shdr_data = {
        .sh_name      = 17,   // ".data" начинается с 17-го байта
        .sh_type      = 1,
        .sh_flags     = 0x3,  // SHF_ALLOC | SHF_WRITE
        .sh_addr      = 0x402000,
        .sh_offset    = 0x2000,
        .sh_size      = 12,
        .sh_link      = 0,
        .sh_info      = 0,
        .sh_addralign = 16,
        .sh_entsize   = 0
    };

    Elf64_Shdr_my shdr_bss = {
        .sh_name      = 23,   // ".bss"
        .sh_type      = 8,    // SHT_NOBITS
        .sh_flags     = 0x3,  // SHF_ALLOC | SHF_WRITE
        .sh_addr      = 0x402010, // сразу после .data
        .sh_offset    = 0x200C,   // условное смещение (в файле не занимает места)
        .sh_size      = 64,
        .sh_link      = 0,
        .sh_info      = 0,
        .sh_addralign = 16,
        .sh_entsize   = 0
    };

    Elf64_Shdr_my shdr_shstrtab = {
        .sh_name      = 1,    // ".shstrtab" начинается с 1-го байта
        .sh_type      = 3,    // SHT_STRTAB
        .sh_flags     = 0,
        .sh_addr      = 0,
        .sh_offset    = 0x200C,
        .sh_size      = 28,   // shstrtab_size
        .sh_link      = 0,
        .sh_info      = 0,
        .sh_addralign = 1,
        .sh_entsize   = 0
    };


    int shoff = 0x2028;
    memcpy(buffer + shoff, &shdr_null, 64);
    memcpy(buffer + shoff + 64, &shdr_text, 64);
    memcpy(buffer + shoff + 128, &shdr_data, 64);
    memcpy(buffer + shoff + 192, &shdr_bss, 64);
    memcpy(buffer + shoff + 256, &shdr_shstrtab, 64);
}

static void Disasm(char* buffer)
{
    assert(buffer);

    DisasmFile(buffer + 0x1200, "source/backend/lib/bigger", 0x200);
    DisasmFile(buffer + 0x1400, "source/backend/lib/smaller", 0x200);
    DisasmFile(buffer + 0x1600, "source/backend/lib/equal", 0x200);
    DisasmFile(buffer + 0x1800, "source/backend/lib/nequal", 0x200);
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