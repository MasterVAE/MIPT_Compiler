DEFAULT REL
global _start
_start: 
    pop rcx

    pop rbx
    pop rax

    cmp rax, rbx
    ja .bigger
    jmp .not

.bigger:
    push 1
    jmp .return

.not:
    push 0

.return:
    push rcx
    ret