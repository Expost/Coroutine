

#include <stdio.h>
#include "coroutine_mgr.h"



void func_a(void *parm)
{
    printf("this is func_a start\n");

    YEID(parm);

    printf("this is func_a end\n");
}

void func_b(void *parm)
{
    printf("this is func_b start\n");

    YEID(parm);

    printf("this is func_b end\n");
}


int main()
{
    CoroutineMgr co;
    CoroutineMgr::thread_t *t1 = co.create_coroutine(0, func_a, NULL);
    CoroutineMgr::thread_t *t2 = co.create_coroutine(1, func_b, NULL);

    co.add_co(t1);
    co.add_co(t2);

    co.run_all();

    getchar();
    return 0;
}