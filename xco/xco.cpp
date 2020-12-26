#include "xco.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _Win32
extern "C" int save_context(CoroutineCtx *from);
extern "C" int restore_context(CoroutineCtx *to);
extern "C" void switch_context(CoroutineCtx *from, CoroutineCtx *to);
#else
extern int switch_context(CoroutineCtx *from, CoroutineCtx *to) __asm__("switch_context");
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
    int stack_size = (1 << 25);                         
    this->stack_base_ptr_ = new uint8_t[stack_size + 0x10];  //Ԥ�� 0x10 �ֽڵĿռ�
    this->stack_base_ptr_ += 0x8;
    this->stack_ = this->stack_base_ptr_ + stack_size;   //ջ�Ǵ������������ģ����ջ��ַҪָ�����

    memset(&self_ctx_, 0, sizeof(self_ctx_));

#ifdef __i386__ || _M_X86
    self_ctx_.esp = reinterpret_cast<uintptr_t>(this->stack_);
    *((long*)self_ctx_.esp) = reinterpret_cast<uintptr_t>(this);

    self_ctx_.esp -= 4;
    self_ctx_.eip = reinterpret_cast<uintptr_t>(wrapper);
#elif _M_X64
    self_ctx_.rsp = reinterpret_cast<uintptr_t>(this->stack_);
    self_ctx_.rcx = reinterpret_cast<uintptr_t>(this);
    self_ctx_.rip = reinterpret_cast<uintptr_t>(wrapper);
#elif __x86_64__
    self_ctx_.rsp = reinterpret_cast<uintptr_t>(this->stack_);
    self_ctx_.rdi = reinterpret_cast<uintptr_t>(this);
    self_ctx_.rip = reinterpret_cast<uintptr_t>(wrapper);
#endif
}

Coroutine::~Coroutine()
{
    stack_base_ptr_ -= 0x8;
    if (stack_base_ptr_ != NULL)
    {
        delete[] stack_base_ptr_;
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
    //do_switch(&thread_local_ctx, &self_ctx_);
    switch_context(&thread_local_ctx, &self_ctx_);
    set_coroutine_state(get_coroutine_state() != FINISHED ? SUSPEND : FINISHED);
    return thread_local_ctx.swap_value;
}

uintptr_t Coroutine::yield(uintptr_t value)
{
    thread_local_ctx.swap_value = value;
    // do_switch(&self_ctx_, &thread_local_ctx);
    switch_context(&self_ctx_, &thread_local_ctx);
    return self_ctx_.swap_value;
}

// void Coroutine::do_switch(CoroutineCtx *from_ctx, CoroutineCtx *to_ctx)
// {
//     int ret = save_context(from_ctx); //����������, �״η��ػ�����eax = 0
//     if (ret == 0)
//     {
//         restore_context(to_ctx);      //����to��������, jmp֮ǰ������eax = 1
//     }
//     else
//     {
//         //restored from other threads, just return and continue
//     }
// }
