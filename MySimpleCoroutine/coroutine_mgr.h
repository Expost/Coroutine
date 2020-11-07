
#include <stdint.h>
#include <map>
#include <vector>
#include <windows.h>

#ifndef __COROUTINE_MGR_H_
#define __COROUTINE_MGR_H_

class Coroutine;

class CoroutineMgr
{
public:
    static CoroutineMgr &get_instance();

public:
    void add_coroutine(Coroutine *coroutine);
    void erase_coroutine(Coroutine *coroutine);
    bool run();

private:
    Coroutine* get_coroutine();

private:
    std::vector<Coroutine*> coroutine_vec_;
    CRITICAL_SECTION  lock_;

private:
    CoroutineMgr();
};


#endif