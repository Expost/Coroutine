#ifndef _COROUTINE_H_
#define _COROUTINE_H_

#include <functional>
#include <stdint.h>

class Coroutine;
using CoInterface = std::function<void(Coroutine* this_)>;

#ifdef __i386__ || _M_X86
struct CoroutineCtx
{
    uint32_t eflag;
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t eip;
    uint32_t swap_value;
};
#elif _M_X64
struct CoroutineCtx
{
    uint64_t eflag;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t rsp;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rip;
    uintptr_t swap_value;
};
#elif __x86_64__
struct CoroutineCtx
{
    uint64_t eflag;
    uint64_t r10;
    uint64_t r11;
    uint64_t rbp;
    uint64_t rsp;
    uint64_t rbx;
    uint64_t rdi;
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
        PRESTART = 0,    //< ������֮ǰ��ֻ�Ǵ���ڶ����У���δִ�й�
        RUNNING,         //< ����ĳ���߳���ִ��
        SUSPEND,         //< ���𣬻�δִ���ֻ꣬��ͨ��switch_out��������
        FINISHED         //< ִ�����
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