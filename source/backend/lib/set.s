DEFAULT REL
global _start
_start: 
    pop rcx
    pop rsi
    pop rdi
    push rcx

    mov rax, rdi
    mov rbx, 10
    imul rbx
    add rax, rsi

    mov [0x403002 + rax], '0'
    ret
