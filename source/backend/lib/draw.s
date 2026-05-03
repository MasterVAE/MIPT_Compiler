DEFAULT REL
global _start
_start: 
    xor rsi, rsi
    xor rdi, rdi

.cycle_y:

    xor rsi, rsi
.cycle_x:

    mov rax, rdi
    add rax, rsi

    lea rbx, [0x402002 + rax]
    mov cl, [rbx]
    mov [0x402000], cl
    call _write

    inc rsi
    cmp rsi, 10
    jl .cycle_x

    mov [0x402000], 0x0a
    call _write

    add rdi, 10
    cmp rdi, 100
    jl .cycle_y

    ret;

_write:
    push rax     
    push rdi
    push rsi
    push rdx
    push rcx

    mov rax, 0x01           ;syscall печати буффера
    mov rdi, 1
    mov rsi, 0x402000
    xor rdx, rdx
    mov dl, 1
    syscall

    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rax

    ret