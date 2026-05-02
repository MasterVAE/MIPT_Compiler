DEFAULT REL
global _start
_start: 
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
    mov [0x402000], '-'
    call _write

.decimal_plus:

    mov r10, 8
.dec_clean_loop:                ; очистка буффера чисел
    mov r11, 64 + 0x402010
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

    mov r11, 63 + 0x402010
    sub r11, r10
    mov sil, dl 
    add sil, '0' 
    mov byte [r11], sil   

    inc r10
    cmp r10, 8
    jne .dec_loop

    mov r10, 8
.dec_zero_loop:                 ; выкидыш старших нулей
    mov r11, 64 + 0x402010
    sub r11, r10

    mov dl, [r11]

    cmp dl, '0'
    jne .dec_print_loop

    dec r10
    cmp r10, 1
    jne .dec_zero_loop

.dec_print_loop:                ; печать буффера
    mov r11, 64 + 0x402010
    sub r11, r10

    mov dl, [r11]
    mov [0x402000], dl
    call _write

    dec r10
    cmp r10, 0
    jne .dec_print_loop

    mov rax, 0x01
    mov rdi, 1
    mov rsi, 0x402001
    mov rdx, 1
    syscall


    ret



_write:
    push rax     
    push rdi
    push rsi
    push rdx
    push rcx
    push r10

    mov rax, 0x01           ;syscall печати буффера
    mov rdi, 1
    mov rsi, 0x402000
    xor rdx, rdx
    mov dl, 1
    syscall

    pop r10
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rax

    ret

