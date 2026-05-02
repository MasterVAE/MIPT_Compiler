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
    push 0
    jmp .return

.nequal:
    push 1

.return:
    push rcx
    ret