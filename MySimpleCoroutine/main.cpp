

#include <stdio.h>
#include "coroutine_mgr.h"
#include "coroutine.h"

class MyCoroutine1 :public Coroutine
{
private:
    void run() override
    {
        int sum = 0;
        for (int i = 0; i < 3; i++)
        {
            printf("[Coroutine 1] sum is:%d\n", sum += i);
            switch_out();
        }
    }
};

class MyCoroutine2 :public Coroutine
{
private:
    void run() override
    {
        func1(1);
        func2();
    }

    void func1(int count)
    {
        for (int i = 0; i < count; i++)
        {
            printf("[Coroutine 2] i is:%d\n", i);
            switch_out();
        }
    }

    void func2()
    {
        printf("[Coroutine 2] func2 start\n");
        switch_out();
        func3();
    }

    void func3()
    {
        printf("[Coroutine 2] current tid is:%d\n", GetCurrentThreadId());
        switch_out();
        printf("[Coroutine 2] func3 end.\n");
    }
};

DWORD WINAPI my_thread1(LPVOID parm)
{
    int count = 0;
    while (CoroutineMgr::get_instance().run())
    {
        printf("[my_thread1] count:%d\n\n", count++);
        Sleep(2000);
    }

    return 0;
}

DWORD WINAPI my_thread2(LPVOID parm)
{
    int count = 0;
    while (CoroutineMgr::get_instance().run())
    {
        printf("[my_thread2] count:%d\n\n", count++);
        Sleep(1000);
    }

    return 0;
}

int main()
{
    CoroutineMgr::get_instance().add_coroutine(new MyCoroutine1);
    CoroutineMgr::get_instance().add_coroutine(new MyCoroutine2);

    // 单线程
    int count = 0;
    while(CoroutineMgr::get_instance().run())
    {
        printf("[main func] count:%d\n\n", count++);
        Sleep(1000);
    }

    // 多线程
    //CreateThread(NULL, 0, my_thread1, NULL, NULL, 0);
    //CreateThread(NULL, 0, my_thread2, NULL, NULL, 0);
        
    getchar();
    return 0;
}