
#ifndef _COROUTINE_H_
#define _COROUTINE_H_

//#include "coroutine_mgr.h"
#include <functional>


class Coroutine;
using CoInterface = std::function<void(Coroutine* this_)>;

#ifndef _M_X64
struct CoroutineCtx
{
    //unsigned int eflag;
    uint32_t edi; // 0
    uint32_t esi; // 4
    uint32_t ebp; // 8
    uint32_t esp; // 12
    uint32_t ebx; // 16
    
    //uint32_t edx; // 20
    //uint32_t ecx; // 24
    
    uint32_t eip; // 28
    uint32_t swap_value;
};
#else
struct CoroutineCtx
{
    //unsigned int eflag;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t rsp;
    uint64_t rbx;

    uint64_t rcx;
    //uint64_t rdx;
    //uint64_t r8;
    //uint64_t r9;
    //uint64_t r10;
    //uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rip;
    uintptr_t swap_value;
};

#endif

class Coroutine
{
public:
    enum CoroutineState
    {
        PRESTART = 0,    //< 在运行之前，只是存放在队列中，从未执行过
        RUNNING,         //< 正在某个线程中执行
        SUSPEND,         //< 挂起，还未执行完，只是通过switch_out函数跳出
        FINISHED         //< 执行完成
    };

public:
    Coroutine(CoInterface &&co_inf);
    virtual ~Coroutine();

public:
    CoroutineState get_coroutine_state() const;

public:
    uintptr_t yield(uintptr_t value);
    uintptr_t resume(uintptr_t value);

private:
    //virtual void run() = 0;

private:
    void set_coroutine_state(CoroutineState state);
    void do_switch(CoroutineCtx *from_ctx, CoroutineCtx *to_ctx);

private:
    static void wrapper(void *parm);

private:
    unsigned id_;
    CoroutineState state_;
    uint8_t *stack_base_ptr_;
    uint8_t *stack_;
    CoroutineCtx *original_ctx_;
    CoroutineCtx self_ctx_;
    CoInterface co_inf_;
};

#endif