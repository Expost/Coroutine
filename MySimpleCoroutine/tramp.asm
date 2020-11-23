.CODE
public save_context
public restore_context

save_context proc
    mov rax, rcx
    
    ; ±£´æ×´Ì¬¼Ä´æÆ÷
    pushfq
    pop [rax]

    mov [rax + 8], rdi
    mov [rax + 16], rsi
    mov [rax + 24], rbp

    lea rsp, [rsp + 8]
    mov [rax + 32], rsp
    lea rsp, [rsp - 8]

    mov [rax + 40], rbx
    mov [rax + 48], rcx
    mov [rax + 56], r12
    mov [rax + 64], r13
    mov [rax + 72], r14
    mov [rax + 80], r15
    push [rsp]
    pop qword ptr[rax + 88]
    xor rax, rax
    ret
save_context ENDP

restore_context proc
    mov rax, rcx
    
    ; »Ö¸´×´Ì¬¼Ä´æÆ÷
    push [rax]
    popfq

    mov rdi, [rax + 8]
    mov rsi, [rax + 16]
    mov rbp, [rax + 24]
    mov rsp, [rax + 32]
    mov rbx, [rax + 40]
    mov rcx, [rax + 48]
    mov r12, [rax + 56]
    mov r13, [rax + 64]
    mov r14, [rax + 72]
    mov r15, [rax + 80]
    mov rax, [rax + 88]
    jmp rax
restore_context ENDP


END