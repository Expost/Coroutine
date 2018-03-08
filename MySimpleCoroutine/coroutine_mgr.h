

#include <vector>



class CoroutineMgr
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
    } ctx_buf_t;

    typedef void (*thread_handler_t)(void *parm);
    typedef struct stThread
    {
        unsigned id;
        bool finished;
        long stack;
        stThread *main_t;
        thread_handler_t handler;
        void *parm;
        ctx_buf_t ctx; 
    } thread_t;

public:
    CoroutineMgr();

public:
    thread_t *create_coroutine(unsigned id, thread_handler_t thread_handle, void *parm);
    thread_t *create_main_coroutine();

    void add_co(thread_t *t);

    void erase_thread_t(thread_t *t);
    bool coroutine_start(thread_t *t);
    void run_all();

    static void do_switch(thread_t *from, thread_t *to);

private:
    static void wrapper(void *parm);
    static void main_coroutine(void *parm);

private:
    
    void destroy_coroutine();

private:
    thread_t *main_t;

private:
    size_t main_id;
    size_t current_id;
    std::vector<thread_t *> thread_t_s_;
    bool is_running;
};

extern CoroutineMgr::thread_t *g_main_t;

#define YEID(parm) (CoroutineMgr::do_switch((CoroutineMgr::thread_t*)parm, g_main_t))