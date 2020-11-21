#include "coroutine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _M_X64
_declspec(naked) int save_context(Coroutine::CoroutineCtx *from)
{
    __asm
    {
        mov eax, [esp + 4];     // 取参
        mov[eax], edi;
        mov[eax + 4], esi;
        mov[eax + 8], ebp;

        //lea ebp, [esp + 4];
        lea esp, [esp + 4];
        mov[eax + 12], esp;     // ebp保存后已经无用了，这里借用ebp存储修正后的值
        lea esp, [esp - 4];

        mov[eax + 16], ebx;
        mov[eax + 20], edx;
        mov[eax + 24], ecx;        

        push [esp];                 //< 压入该函数的返回地址
        pop dword ptr[eax + 28];    //< 将返回地址存放在context中的eip字段中

        xor eax, eax;
        ret;
    }
}

__declspec(naked) int restore_context(Coroutine::CoroutineCtx *to)
{
    __asm
    {
        mov eax, [esp + 4];
        mov edi, [eax];
        mov esi, [eax + 4];
        mov ebp, [eax + 8];
        mov esp, [eax + 12];
        mov ebx, [eax + 16];
        mov edx, [eax + 20];
        mov ecx, [eax + 24];

        mov eax, [eax + 28];
        jmp eax;
    }
}
#else
extern "C" int save_context(Coroutine::CoroutineCtx *from);
extern "C" int restore_context(Coroutine::CoroutineCtx *to);

#endif

Coroutine::Coroutine()
         : id_(0),
           original_ctx_(NULL),
           state_(PRESTART),
           stack_base_ptr_(NULL),
           stack_(NULL)
{
    int stack_size = (1 << 20);                         
    this->stack_base_ptr_ = new uint8_t[stack_size + 0x10];  //预留 0x10 字节的空间
    this->stack_ = this->stack_base_ptr_ + stack_size;   //栈是从上往下增长的，因此栈基址要指向最大处

    memset(&self_ctx_, 0, sizeof(self_ctx_));

#ifndef _M_X64
    self_ctx_.esp = reinterpret_cast<uintptr_t>(this->stack_);
    *((long*)self_ctx_.esp) = reinterpret_cast<uintptr_t>(this);

    self_ctx_.esp -= 4;
    self_ctx_.eip = reinterpret_cast<uintptr_t>(wrapper);  //initialize program counter
#else
    self_ctx_.rsp = reinterpret_cast<uintptr_t>(this->stack_);
    self_ctx_.rcx = reinterpret_cast<uintptr_t>(this);
    //*((uintptr_t*)self_ctx_.rsp) = reinterpret_cast<uintptr_t>(this);
    //self_ctx_.rsp -= 8;
    self_ctx_.rip = reinterpret_cast<uintptr_t>(wrapper);  //initialize program counter
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

    coroutine->run();
    coroutine->set_coroutine_state(FINISHED);

    coroutine->switch_out();
}

void Coroutine::switch_in(CoroutineCtx *original_ctx)
{
    this->original_ctx_ = original_ctx;

    do_switch(original_ctx, &self_ctx_);
    set_coroutine_state(get_coroutine_state() != FINISHED ? SUSPEND : FINISHED);
}

void Coroutine::switch_out()
{
    do_switch(&self_ctx_, original_ctx_);
}

//#pragma optimize( "", off)
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

//#pragma optimize( "", on)
