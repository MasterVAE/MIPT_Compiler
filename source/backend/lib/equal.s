DEFAULT REL
global _start
_start: 
    pop rcx

    pop rbx
    pop rax

    cmp rax, rbx
    je .equal
    jmp .nequal

.equal:
    push 1
    jmp .return

.nequal:
    push 0

.return:
    push rcx
    ret