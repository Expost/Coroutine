.CODE
public save_context
public restore_context

save_context proc
    mov rax, rcx
    mov [rax], rdi
    mov [rax + 8], rsi
    mov [rax + 16], rbp

    lea rsp, [rsp + 8]
    mov [rax + 24], rsp
    lea rsp, [rsp - 8]

    mov [rax + 32], rbx


    mov [rax + 40], rcx
    ;mov [rax + 48], rdx
    ;mov [rax + 56], r8
    ;mov [rax + 64], r9
    ;mov [rax + 72], r10
    ;mov [rax + 80], r11


    mov [rax + 48], r12
    mov [rax + 56], r13
    mov [rax + 64], r14
    mov [rax + 72], r15
    push [rsp]
    pop qword ptr[rax + 80]
    xor rax, rax
    ret
save_context ENDP

restore_context proc
    mov rax, rcx
    
    mov rdi, [rax]
    mov rsi, [rax + 8]
    mov rbp, [rax + 16]
    mov rsp, [rax + 24]
    mov rbx, [rax + 32]


    mov rcx, [rax + 40]
    ;mov rdx, [rax + 48] 
    ;mov r8, [rax + 56]
    ;mov r9, [rax + 64]
    ;mov r10, [rax + 72]
    ;mov r11, [rax + 80]


    mov r12, [rax + 48]
    mov r13, [rax + 56]
    mov r14, [rax + 64]
    mov r15, [rax + 72]
    mov rax, [rax + 80]
    jmp rax
    
    ;push [rax + 120]
    ;mov rax, rdx
    ;ret
restore_context ENDP


END