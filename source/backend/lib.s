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
    pop r12

    xor rax, rax
    xor rcx, rcx
    .loop:
    call _read
    movzx rbx, byte [Symbol]

    cmp bl, byte [EndSymbol]
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
    call _write

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
    mov sil, '0'
    add sil, dl     
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
    call _write

    dec r10
    cmp r10, 0
    jne .dec_print_loop

    mov rax, 0x01
    mov rdi, 1
    mov rsi, EndSymbol
    mov rdx, 1
    syscall


    ret



_write:
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


_read:
    push rax     
    push rdi
    push rsi
    push rdx
    push rcx

    mov rax, 0            
    mov rdi, 0            
    mov rsi, Symbol       
    mov rdx, 1          
    syscall   

    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rax

    ret 



L0_DRAW:
    xor rsi, rsi
    xor rdi, rdi

.cycle_y:

    xor rsi, rsi
.cycle_x:

    mov rax, rdi
    add rax, rsi

    lea rbx, [Draw_buffer + rax]
    mov cl, [rbx]
    mov [Symbol], cl
    call _write

    inc rsi
    cmp rsi, 10
    jl .cycle_x

    mov [Symbol], 0x0a
    call _write

    add rdi, 10
    cmp rdi, 100
    jl .cycle_y

    ret;

L0_SET:
    pop rcx
    pop rsi
    pop rdi
    push rcx

    mov rax, rdi
    mov rbx, 10
    imul rbx
    add rax, rsi

    mov [Draw_buffer + rax], '0'
    ret




section         .data

Symbol          db '0'
EndSymbol       db 0x0a
Numbers         db '0123456789'
Draw_buffer times 100 db '_'

section         .bss

Num_buffer          resb 64