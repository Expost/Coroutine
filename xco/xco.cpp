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

Coroutine::Coroutine(CoInterface &&co_inf)
         : id_(0),
           state_(PRESTART),
           stack_base_ptr_(NULL),
           stack_(NULL),
           co_inf_(std::move(co_inf))
{
    int stack_size = (1 << 25);                         
    this->stack_base_ptr_ = new uint8_t[stack_size + 0x10];  //预留 0x10 字节的空间
    this->stack_ = this->stack_base_ptr_ + stack_size;   //栈是从上往下增长的，因此栈基址要指向最大处

    memset(&self_ctx_, 0, sizeof(self_ctx_));

#ifdef __i386__ || _M_X86
    self_ctx_.esp = reinterpret_cast<uintptr_t>(this->stack_);
    *((long*)self_ctx_.esp) = reinterpret_cast<uintptr_t>(this);

    self_ctx_.esp -= 4;
    self_ctx_.eip = reinterpret_cast<uintptr_t>(wrapper);
#elif _M_X64
    self_ctx_.rsp = reinterpret_cast<uintptr_t>(this->stack_);
    self_ctx_.rsp -= 0x8;

    self_ctx_.rcx = reinterpret_cast<uintptr_t>(this);
    self_ctx_.rip = reinterpret_cast<uintptr_t>(wrapper);
#elif __x86_64__
    self_ctx_.rsp = reinterpret_cast<uintptr_t>(this->stack_);
     // 这里的目的是64位下，调用函数时的栈是16字节对齐的，这里默认就是16字节对齐的，但当进到一个函数中时，由于压入了返回值
     // 所以函数内是8字节对齐的，所以这里也要改为8字节对齐
    self_ctx_.rsp -= 0x8;      

    self_ctx_.rdi = reinterpret_cast<uintptr_t>(this);
    self_ctx_.rip = reinterpret_cast<uintptr_t>(wrapper);
#endif
}

Coroutine::~Coroutine()
{
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
    switch_context(&original_ctx_, &self_ctx_);
    set_coroutine_state(get_coroutine_state() != FINISHED ? SUSPEND : FINISHED);
    return original_ctx_.swap_value;
}

uintptr_t Coroutine::yield(uintptr_t value)
{
    original_ctx_.swap_value = value;
    // do_switch(&self_ctx_, &thread_local_ctx);
    switch_context(&self_ctx_, &original_ctx_);
    return self_ctx_.swap_value;
}

// void Coroutine::do_switch(CoroutineCtx *from_ctx, CoroutineCtx *to_ctx)
// {
//     int ret = save_context(from_ctx); //保存上下文, 首次返回会设置eax = 0
//     if (ret == 0)
//     {
//         restore_context(to_ctx);      //加载to的上下文, jmp之前会设置eax = 1
//     }
//     else
//     {
//         //restored from other threads, just return and continue
//     }
// }
