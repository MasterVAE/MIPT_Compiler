#ifndef CONSTANTS_H
#define CONSTANTS_H

#define REX_W 0x48

#define RAX 0
#define RBX 3
#define RCX 1
#define RDX 2

#define PUSH 0x50
#define POP 0x58
#define RET 0xC3
#define CALL 0xE8
#define JMP 0xE9

#define SYSCALL_1 0x0F
#define SYSCALL_2 0x05

#define ENTRY_POINT 0x1000
#define DATA_POINT 0x3000
#define VADRESS 0x400000
#define ALIGNMENT 0x1000
#define FILESIZE 0x4000

#define BIGGER_ENTRY 0x2200
#define SMALLER_ENTRY 0x2300
#define EQUAL_ENTRY 0x2400
#define NEQUAL_ENTRY 0x2500
#define SET_ENTRY 0x2600
#define DRAW_ENTRY 0x2800
#define OUT_ENTRY 0x2A00
#define IN_ENTRY 0x2D00

#define PLACEHOLDER 0

#endif // CONSTANTS_H