.text
.globl switch_context
.intel_syntax noprefix
switch_context:

#ifdef __i386__
    mov eax, [esp + 4]
    lea edx, [esp + 4]

    pushfd
    pop dword ptr[eax]
    mov [eax + 4], edi
    mov [eax + 8], esi
    mov [eax + 12], ebp
    mov [eax + 16], edx
    mov [eax + 20], ebx;
    push [esp];
    pop dword ptr[eax + 24]

    //restore
    mov eax, [esp + 8]
    push dword ptr[eax];
    popfd;

    mov edi, [eax + 4]
    mov esi, [eax + 8]
    mov ebp, [eax + 12]
    mov esp, [eax + 16]
    mov ebx, [eax + 20]
    mov eax, [eax + 24]
    jmp eax

#elif __x86_64__
    // arg1, from_co   rdi
    // arg2, to_co     rsi
    // rdi，rsi，rdx，rcx，r8，r9

    pushfq
    pop [rdi]
    lea r8, [rsp + 8]    // rsp

    mov [rdi + 8], r10
    mov [rdi + 16], r11
    mov [rdi + 24], rbp
    mov [rdi + 32], r8
    mov [rdi + 40], rbx
    mov [rdi + 48], rdi
    mov [rdi + 56], r12
    mov [rdi + 64], r13
    mov [rdi + 72], r14
    mov [rdi + 80], r15
    push [rsp]           // ret addr
    pop qword ptr[rdi + 88]

    // restore
    push [rsi]
    popfq
    mov r10, [rsi + 8]
    mov r11, [rsi + 16]
    mov rbp, [rsi + 24]
    mov rsp, [rsi + 32]
    mov rbx, [rsi + 40]
    mov rdi, [rsi + 48]
    mov r12, [rsi + 56]
    mov r13, [rsi + 64]
    mov r14, [rsi + 72]
    mov r15, [rsi + 80]
    jmp qword ptr[rsi + 88]

#else
    #error "platform not support"
#endif
