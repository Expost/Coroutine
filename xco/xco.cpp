#include "xco.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _M_X64
_declspec(naked) int save_context(CoroutineCtx *from)
{
    __asm
    {
        mov eax, [esp + 4];     // 取参
        
        // 保存状态寄存器
        pushfd;
        pop dword ptr[eax];
         
        mov[eax + 4], edi;
        mov[eax + 8], esi;
        mov[eax + 12], ebp;

        lea esp, [esp + 4];
        mov[eax + 16], esp;
        lea esp, [esp - 4];

        mov[eax + 20], ebx;

        push [esp];                 //< 压入该函数的返回地址
        pop dword ptr[eax + 24];    //< 将返回地址存放在context中的eip字段中

        xor eax, eax;
        ret;
    }
}

__declspec(naked) int restore_context(CoroutineCtx *to)
{
    __asm
    {
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
    }
}
#else
extern "C" int save_context(CoroutineCtx *from);
extern "C" int restore_context(CoroutineCtx *to);
#endif

thread_local CoroutineCtx thread_local_ctx = { 0 };

Coroutine::Coroutine(CoInterface &&co_inf)
         : id_(0),
           original_ctx_(NULL),
           state_(PRESTART),
           stack_base_ptr_(NULL),
           stack_(NULL),
           co_inf_(std::move(co_inf))
{
    int stack_size = (1 << 20);                         
    this->stack_base_ptr_ = new uint8_t[stack_size + 0x10];  //预留 0x10 字节的空间
    this->stack_ = this->stack_base_ptr_ + stack_size;   //栈是从上往下增长的，因此栈基址要指向最大处

    memset(&self_ctx_, 0, sizeof(self_ctx_));

#ifndef _M_X64
    self_ctx_.esp = reinterpret_cast<uintptr_t>(this->stack_);
    *((long*)self_ctx_.esp) = reinterpret_cast<uintptr_t>(this);

    self_ctx_.esp -= 4;
    self_ctx_.eip = reinterpret_cast<uintptr_t>(wrapper);
#else
    self_ctx_.rsp = reinterpret_cast<uintptr_t>(this->stack_);
    self_ctx_.rcx = reinterpret_cast<uintptr_t>(this);
    self_ctx_.rip = reinterpret_cast<uintptr_t>(wrapper);
#endif
}

Coroutine::~Coroutine()
{
    if (stack_base_ptr_ != NULL)
    {
        delete stack_base_ptr_;
        stack_base_ptr_ = NULL;
    }
}

Coroutine::CoroutineState Coroutine::get_coroutine_state() const
{
    return this->state_;
}

void Coroutine::set_coroutine_state(CoroutineState state)
{
    this->state_ = state;
}

void Coroutine::wrapper(void *parm)
{
    Coroutine *coroutine = reinterpret_cast<Coroutine*>(parm);
    coroutine->co_inf_(coroutine);
    coroutine->set_coroutine_state(FINISHED);
    coroutine->yield(0);
}

uintptr_t Coroutine::resume(uintptr_t value)
{
    if (get_coroutine_state() == FINISHED)
    {
        return 0;
    }

    self_ctx_.swap_value = value;
    do_switch(&thread_local_ctx, &self_ctx_);
    set_coroutine_state(get_coroutine_state() != FINISHED ? SUSPEND : FINISHED);
    return thread_local_ctx.swap_value;
}

uintptr_t Coroutine::yield(uintptr_t value)
{
    thread_local_ctx.swap_value = value;
    do_switch(&self_ctx_, &thread_local_ctx);
    return self_ctx_.swap_value;
}

void Coroutine::do_switch(CoroutineCtx *from_ctx, CoroutineCtx *to_ctx)
{
    int ret = save_context(from_ctx); //保存上下文, 首次返回会设置eax = 0
    if (ret == 0)
    {
        restore_context(to_ctx);      //加载to的上下文, jmp之前会设置eax = 1
    }
    else
    {
        //restored from other threads, just return and continue
    }
}
