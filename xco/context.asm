IFDEF RAX
.CODE


; rcx from_co
; rdx to_co
; r8, r9, r10, r11
switch_context proc
    pushfq
    pop [rcx]
    lea r8, [rsp + 8]    ; rsp

    mov [rcx + 8], rdi
    mov [rcx + 16], rsi
    mov [rcx + 24], rbp
    mov [rcx + 32], r8
    mov [rcx + 40], rbx
    mov [rcx + 48], rcx
    mov [rcx + 56], r12
    mov [rcx + 64], r13
    mov [rcx + 72], r14
    mov [rcx + 80], r15
    push [rsp]           ; ret addr
    pop qword ptr[rcx + 88]

    ; restore
    push [rdx]
    popfq
    mov rdi, [rdx + 8]
    mov rsi, [rdx + 16]
    mov rbp, [rdx + 24]
    mov rsp, [rdx + 32]
    mov rbx, [rdx + 40]
    mov rcx, [rdx + 48]
    mov r12, [rdx + 56]
    mov r13, [rdx + 64]
    mov r14, [rdx + 72]
    mov r15, [rdx + 80]
    ;mov rax, [rdx + 88]
    jmp qword ptr[rdx + 88]

switch_context endp


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


ELSE

.386
.MODEL FLAT,C
.CODE

save_context proc
    mov eax, [esp + 4];
        
    pushfd;
    pop dword ptr[eax];
         
    mov[eax + 4], edi;
    mov[eax + 8], esi;
    mov[eax + 12], ebp;

    lea esp, [esp + 4];
    mov[eax + 16], esp;
    lea esp, [esp - 4];

    mov[eax + 20], ebx;

    push [esp];
    pop dword ptr[eax + 24];

    xor eax, eax;
    ret;
save_context ENDP


restore_context proc
    mov eax, [esp + 4];
    push dword ptr[eax];
    popfd;

    mov edi, [eax + 4];
    mov esi, [eax + 8];
    mov ebp, [eax + 12];
    mov esp, [eax + 16];
    mov ebx, [eax + 20];
    mov eax, [eax + 24];
    jmp eax;
restore_context ENDP

ENDIF

END