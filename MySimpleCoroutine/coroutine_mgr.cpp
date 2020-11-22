

#include "coroutine_mgr.h"
#include "coroutine.h"
#include <Windows.h>

CoroutineMgr::CoroutineMgr()
{
    memset(&lock_, 0, sizeof(lock_));
    InitializeCriticalSection(&lock_);
}

CoroutineMgr &CoroutineMgr::get_instance()
{
    static CoroutineMgr cor_mgr;
    return cor_mgr;
}

void CoroutineMgr::add_coroutine(Coroutine *coroutine)
{
    EnterCriticalSection(&lock_);
    coroutine_vec_.push_back(coroutine);
    LeaveCriticalSection(&lock_);
}

Coroutine* CoroutineMgr::get_coroutine()
{
    EnterCriticalSection(&lock_);
    Coroutine *coroutine = NULL;
    auto itr = coroutine_vec_.begin();
    if (itr != coroutine_vec_.end() &&
        ((*itr)->get_coroutine_state() == Coroutine::PRESTART ||
        (*itr)->get_coroutine_state() == Coroutine::SUSPEND))
    {
        coroutine = *itr;
        coroutine_vec_.erase(itr);
        coroutine_vec_.push_back(coroutine);
    }
    
    //for (auto co : coroutine_vec_)
    //{
    //    if (co->get_coroutine_state() == Coroutine::PRESTART || 
    //        co->get_coroutine_state() == Coroutine::SUSPEND)
    //    {
    //        /// 在这里就设为RUNING，避免多线程情况下，该协程被重复选中
    //        co->set_coroutine_state(Coroutine::RUNNING);
    //        coroutine = co;
    //        break;
    //    }
    //}

    LeaveCriticalSection(&lock_);
    return coroutine;
}

void CoroutineMgr::erase_coroutine(Coroutine *coroutine)
{
    EnterCriticalSection(&lock_);
    auto it = coroutine_vec_.begin();
    for (; it != coroutine_vec_.end(); it++)
    {
        if (*it == coroutine)
        {
            coroutine_vec_.erase(it);
            break;
        }
    }

    LeaveCriticalSection(&lock_);

    delete coroutine;
}

bool CoroutineMgr::run()
{
    Coroutine *t = get_coroutine();
    if (t == NULL)
    {
        return false;
    }

    CoroutineCtx ctx;
    t->resume();

    if (t->get_coroutine_state() == Coroutine::FINISHED)
    {
        erase_coroutine(t);
    }

    return true;
}