
#ifndef _COROUTINE_H_
#define _COROUTINE_H_

#include "coroutine_mgr.h"

class Coroutine
{
public:
    typedef struct
    {
        //unsigned int eflag;
        unsigned int edi; // 0
        unsigned int esi; // 4
        unsigned int ebp; // 8
        unsigned int esp; // 12
        unsigned int ebx; // 16
        unsigned int edx; // 20
        unsigned int ecx; // 24
        unsigned int eip; // 28
    } CoroutineCtx;

    typedef enum
    {
        PRESTART = 0,    //< 在运行之前，只是存放在队列中，从未执行过
        RUNNING,         //< 正在某个线程中执行
        SUSPEND,         //< 挂起，还未执行完，只是通过switch_out函数跳出
        FINISHED         //< 执行完成
    }CoroutineState;

public:
    Coroutine();
    virtual ~Coroutine();

public:
    CoroutineState get_coroutine_state() const;

protected:
    void switch_out();

private:
    virtual void run() = 0;

private:
    void set_coroutine_state(CoroutineState state);
    void switch_in(CoroutineCtx *original_ctx);
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

private:
    /// 设置CoroutineMgr为Coroutine的友元，使其可访问协程的私有方法
    friend class CoroutineMgr;
};

#endif