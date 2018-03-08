

#include "coroutine_mgr.h"
#include <Windows.h>

CoroutineMgr::thread_t *g_main_t;

__declspec(naked) int save_context(CoroutineMgr::ctx_buf_t *from)
{
    __asm
    {
        mov eax, [esp + 4];
        mov [eax], edi;
        mov [eax + 4], esi;
        mov [eax + 8], ebp;
        mov [eax + 12], esp;
        mov [eax + 16], ebx;
        mov [eax + 20], edx;
        mov [eax + 24], ecx;
        push [esp];
        pop dword ptr [eax + 28];

        xor eax,eax;
        ret;
    }
}

__declspec(naked) int restore_context(CoroutineMgr::ctx_buf_t *to)
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

void CoroutineMgr::do_switch(thread_t *from, thread_t *to)
{
    int ret = save_context(&(from->ctx)); //保存上下文, 首次返回会设置eax = 0
    if (ret == 0) 
    {
        restore_context(&to->ctx);     //加载to的上下文, jmp之前会设置eax = 1
    }
    else
    {
        //restored from other threads, just return and continue
    }
}


CoroutineMgr::CoroutineMgr():
            current_id(0),
            main_id(0),
            is_running(true)
{
    main_t = create_main_coroutine();
    g_main_t = main_t;
}

CoroutineMgr::thread_t *CoroutineMgr::create_coroutine(unsigned id, thread_handler_t thread_handle, void *parm)
{
    thread_t *t = (thread_t*)malloc(sizeof(thread_t));

    int stack_size = (1 << 20);  //fixed 1M for demo
    void *stack_top = malloc(stack_size);  //or mmap with stack type sepcified

    t->id = id;
    t->finished = false;
    t->main_t = main_t;
    t->stack = (long)stack_top + stack_size; //栈是从上往下增长的，因此栈基址要指向最大处
    t->handler = thread_handle;
    t->parm = t;
    memset(&t->ctx, 0, sizeof(ctx_buf_t));
    t->ctx.esp = (long)t->stack;  //initialize stack pointer
    *((int*)t->ctx.esp) = (int)t;
    t->ctx.esp -= 4;
    t->ctx.eip = (int)wrapper;  //initialize program counter
    return t;
}

CoroutineMgr::thread_t *CoroutineMgr::create_main_coroutine()
{
    thread_t *t = (thread_t*)malloc(sizeof(thread_t));

    int stack_size = (1 << 20);  //fixed 1M for demo
    void *stack_top = malloc(stack_size);  //or mmap with stack type sepcified

    t->id = 0;
    t->finished = false;
    t->stack = (long)stack_top + stack_size; //栈是从上往下增长的，因此栈基址要指向最大处
    t->handler = main_coroutine;
    t->parm = this;
    memset(&t->ctx, 0, sizeof(ctx_buf_t));
    t->ctx.esp = (long)t->stack;  //initialize stack pointer
    *((int*)t->ctx.esp) = (int)this;
    t->ctx.esp -= 4;
    t->ctx.eip = (int)main_coroutine;  //initialize program counter
    return t;
}

bool CoroutineMgr::coroutine_start(thread_t *t)
{
    restore_context(&t->ctx);

    //never returns
    return false;
}

void CoroutineMgr::run_all()
{
    coroutine_start(main_t);
}

void CoroutineMgr::wrapper(void *parm)
{
    thread_t *current_t = (thread_t *)parm;
    
    thread_handler_t func = current_t->handler;
    func(current_t->parm);

    current_t->finished = true;

    do_switch(current_t, current_t->main_t);
}

void CoroutineMgr::erase_thread_t(thread_t *t)
{
    auto iter = thread_t_s_.begin();
    for (; iter != thread_t_s_.end(); iter++)
    {
        if (*iter == t)
        {
            thread_t_s_.erase(iter);
            break;
        }
    }
}

void CoroutineMgr::main_coroutine(void *parm)
{
    CoroutineMgr* mgr = (CoroutineMgr*)(parm);
    thread_t *main_t = mgr->main_t;

    while (mgr->is_running)
    {
        size_t i = 0;
        while (i < mgr->thread_t_s_.size())
        {
            mgr->current_id = i;
            mgr->do_switch(main_t, mgr->thread_t_s_[i]);

            if (mgr->thread_t_s_[i]->finished == true)
            {
                mgr->erase_thread_t(mgr->thread_t_s_[i]);
            }
            else
            {
                i++;
            }
        }

        Sleep(10);
    }
}

void CoroutineMgr::add_co(thread_t *t)
{
    thread_t_s_.push_back(t);
}