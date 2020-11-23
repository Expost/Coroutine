#include <stdio.h>
#include "coroutine.h"
//
//class MyCoroutine1 :public Coroutine
//{
//private:
//    void run()
//    {
//        int sum = 0;
//        for (int i = 0; i < 3; i++)
//        {
//            printf("[Coroutine 1] sum is:%d\n", sum += i);
//            yield();
//        }
//    }
//};
//
//class MyCoroutine2 :public Coroutine
//{
//private:
//    void run()
//    {
//        func1(1);
//        func2();
//    }
//
//    void func1(int count)
//    {
//        for (int i = 0; i < count; i++)
//        {
//            printf("[Coroutine 2] i is:%d\n", i);
//            yield();
//        }
//    }
//
//    void func2()
//    {
//        printf("[Coroutine 2] func2 start\n");
//        yield();
//        func3();
//    }
//
//    void func3()
//    {
//        printf("[Coroutine 2] current tid is:%d\n", GetCurrentThreadId());
//        yield();
//        printf("[Coroutine 2] func3 end.\n");
//    }
//};
//
//DWORD WINAPI my_thread1(LPVOID parm)
//{
//    int count = 0;
//    while (CoroutineMgr::get_instance().run())
//    {
//        printf("[my_thread1] count:%d\n\n", count++);
//        Sleep(2000);
//    }
//
//    return 0;
//}
//
//DWORD WINAPI my_thread2(LPVOID parm)
//{
//    int count = 0;
//    while (CoroutineMgr::get_instance().run())
//    {
//        printf("[my_thread2] count:%d\n\n", count++);
//        Sleep(1000);
//    }
//
//    return 0;
//}

int main()
{
    //CoroutineMgr::get_instance().add_coroutine(new MyCoroutine1);
    //CoroutineMgr::get_instance().add_coroutine(new MyCoroutine2);

    //// µ¥Ïß³Ì
    //int count = 0;
    //while(CoroutineMgr::get_instance().run())
    //{
    //    printf("[main func] count:%d\n\n", count++);
    //    Sleep(1000);
    //}


    //CoroutineCtx ctx = { 0 };
    Coroutine co1([](Coroutine* this_){
        for (int i = 0; i < 3; i++) {
            //printf("co1 value %d\n", i);
            
            printf("yield value %llu\n", this_->yield((i + 1) * 10));
        }

        });

    Coroutine co2([](Coroutine* this_) {
        for (int i = 0; i < 3; i++) {
            printf("co2 value %d\n", i);
            //this_->yield();
        }

        });

    printf("resume value %p\n", co1.resume(1));
    printf("resume value %p\n", co1.resume(2));
    printf("resume value %p\n", co1.resume(3));
    printf("resume value %p\n", co1.resume(4));
    //printf("resume value %u\n", co1.resume(5));


    //co2.resume();
    //co2.resume();
    //co2.resume();


        
    getchar();
    return 0;
}