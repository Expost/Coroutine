.CODE
public save_context
public restore_context

save_context proc
    mov rax, rcx
    mov [rax], rdi
    mov [rax + 8], rsi
    mov [rax + 16], rbp
    mov [rax + 24], rsp
    mov [rax + 32], rbx
    mov [rax + 40], rcx
    mov [rax + 48], rdx
    mov [rax + 56], r8
    mov [rax + 64], r9
    mov [rax + 72], r10
    mov [rax + 80], r11
    mov [rax + 88], r12
    mov [rax + 96], r13
    mov [rax + 104], r14
    mov [rax + 112], r15
    push [rsp]
    pop qword ptr[rax + 120]
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
    mov rdx, [rax + 48]
    mov r8, [rax + 56]
    mov r9, [rax + 64]
    mov r10, [rax + 72]
    mov r11, [rax + 80]
    mov r12, [rax + 88]
    mov r13, [rax + 96]
    mov r14, [rax + 104]
    mov r15, [rax + 112]
    mov rax, [rax + 120]
    jmp rax
restore_context ENDP


END