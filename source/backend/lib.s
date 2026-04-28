DEFAULT REL
section     .text

global L0_EQUAL
global L0_NEQUAL
global L0_SMALLER
global L0_BIGGER
global L0_IN
global L0_OUT

L0_EQUAL: 
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





L0_NEQUAL: 
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



L0_BIGGER: 
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


L0_SMALLER: 
    pop rcx

    pop rbx
    pop rax

    cmp rax, rbx
    jb .smaller
    jmp .not

.smaller:
    push 1
    jmp .return

.not:
    push 0

.return:
    push rcx
    ret




L0_IN:
    pop rcx

    push 0

    push rcx
    ret



L0_OUT:
    pop rcx

    pop rax

    push rcx

                                        ; проверка минуса
    mov r10d, eax
    shr r10d, 31d
    cmp r10d, 0
    je .decimal_plus
    
    mov r10d, eax
    xor eax, eax
    sub eax, r10d
    mov [Symbol], '-'
    call _print_buffer

.decimal_plus:

    mov r10, 8
.dec_clean_loop:                ; очистка буффера чисел
    mov r11, 64 + Num_buffer
    sub r11, r10
    mov byte [r11], 0     

    dec r10
    cmp r10, 0
    jne .dec_clean_loop


    mov r10, 0
.dec_loop:                ; запись числа в буффер

    mov esi, 10
    xor rdx, rdx
    div esi

    mov r11, 63 + Num_buffer
    sub r11, r10
    mov sil, byte [Numbers + rdx]       
    mov byte [r11], sil   

    inc r10
    cmp r10, 8
    jne .dec_loop

    mov r10, 8
.dec_zero_loop:                 ; выкидыш старших нулей
    mov r11, 64 + Num_buffer
    sub r11, r10

    mov dl, [r11]

    cmp dl, '0'
    jne .dec_print_loop

    dec r10
    cmp r10, 1
    jne .dec_zero_loop

.dec_print_loop:                ; печать буффера
    mov r11, 64 + Num_buffer
    sub r11, r10

    mov dl, [r11]
    mov [Symbol], dl
    call _print_buffer

    dec r10
    cmp r10, 0
    jne .dec_print_loop

    mov rax, 0x01
    mov rdi, 1
    mov rsi, EndSymbol
    mov rdx, 1
    syscall


    ret



_print_buffer:
    push rax     
    push rdi
    push rsi
    push rdx
    push rcx

    mov rax, 0x01           ;syscall печати буффера
    mov rdi, 1
    mov rsi, Symbol
    xor rdx, rdx
    mov dl, 1
    syscall

    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rax

    ret


section         .data

Symbol          db '0'
EndSymbol       db 0x0a
Numbers         db '0123456789ABCDEF'

section         .bss

Num_buffer          resb 64