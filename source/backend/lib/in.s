DEFAULT REL
global _start
_start: 
    pop r12

    xor rax, rax
    xor rcx, rcx
    .loop:
    call _read
    movzx rbx, byte [0x403000]

    cmp bl, 0x0a
    je .end

    cmp bl, '-'
    jne .continue

.minus:
    inc rcx
    jmp .loop

.continue:
    
    xor rdx, rdx
    mov rsi, 10
    mul rsi

    sub bl, '0'
    add rax, rbx

    jmp .loop

.end:
    test rcx, rcx
    jne .end_minus

    push rax

    push r12
    ret

.end_minus:

    xor rbx, rbx
    sub rbx, rax
    push rbx

    push r12
    ret

_read:
    push rax     
    push rdi
    push rsi
    push rdx
    push rcx

    mov rax, 0            
    mov rdi, 0            
    mov rsi, 0x403000     
    mov rdx, 1          
    syscall   

    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rax

    ret 