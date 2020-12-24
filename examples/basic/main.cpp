#include <stdio.h>
#include "xco/xco.h"
#include <netdb.h>
#include <arpa/inet.h>

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
    Coroutine co1([](Coroutine *this_) {
        //printf("co1 value %d\n", i);
        struct hostent *ht = gethostbyname("www.baidu.com");

        const char *ptr = NULL;
        char ip[50];
        ptr = inet_ntop(ht->h_addrtype, ht->h_addr_list[0], ip, sizeof(ip));
        //??inet_ntop???IP???ptr?
        printf("%s\n", ptr);
    });

    printf("resume value %p\n", co1.resume(1));


    getchar();
    return 0;
}