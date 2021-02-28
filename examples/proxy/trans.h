#ifndef __TRANS_H_
#define __TRANS_H_

#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <functional>
#include "xco.h"

class Trans;
struct Dispatch
{
    Trans *on_in = nullptr;
    Trans *on_out = nullptr;
    Trans *on_hup = nullptr;

    int oper = 0;
    int socket = 0;
};

extern std::map<int, Dispatch *> sock_dis;

void mod(Dispatch *dispatch, int oper, Trans *trans)
{
    if (oper & EPOLLIN)
    {
        dispatch->on_in = (Trans *)trans;
    }
    if (oper & EPOLLOUT)
    {
        dispatch->on_out = (Trans *)trans;
    }
    if (oper & EPOLLHUP)
    {
        dispatch->on_hup = (Trans *)trans;
    }
}

int add_trans(int epfd, int socks, Trans *trans, int oper)
{
    epoll_event ev;
    sock_dis[socks] = new Dispatch();
    ev.data.ptr = sock_dis[socks];
    ev.events = oper;
    sock_dis[socks]->oper = oper;
    sock_dis[socks]->socket = socks;
    mod((Dispatch *)ev.data.ptr, oper, trans);
    return epoll_ctl(epfd, EPOLL_CTL_ADD, socks, &ev);
}

int del_trans(int epfd, int socks)
{
    epoll_event ev;
    ev.data.ptr = nullptr;
    ev.events = 0;

    return epoll_ctl(epfd, EPOLL_CTL_DEL, socks, &ev);
}
class Trans
{
public:
    Trans(int epfd) : epfd_(epfd),
                      co_(std::bind(&Trans::warp, this, std::placeholders::_1))
    {
    }

    ~Trans() = default;

public:
    bool is_end()
    {
        return co_.get_coroutine_state() == Coroutine::FINISHED;
    }

    int resume(int value)
    {
        co_.resume(value);
    }

    int modify_mod_add(int socks, void *trans, int oper)
    {
        epoll_event ev;
        //ev.data.ptr = trans;

        ev.data.ptr = sock_dis[socks];
        mod((Dispatch *)ev.data.ptr, oper, (Trans *)trans);
        ev.events = sock_dis[socks]->oper | oper;
        sock_dis[socks]->oper = ev.events;
        //printf("sock %d add %d and now oper is %d\n", socks, oper, ev.events);
        return epoll_ctl(epfd_, EPOLL_CTL_MOD, socks, &ev);
    }

    int modify_mod_del(int socks, void *trans, int oper)
    {
        epoll_event ev;
        //ev.data.ptr = trans;

        ev.data.ptr = sock_dis[socks];
        //mod((Dispatch*)ev.data.ptr, oper, (Trans*)trans);
        sock_dis[socks]->oper &= (~oper);
        ev.events = sock_dis[socks]->oper;
        //printf("sock %d del %d and new oper is %d\n", socks, oper, ev.events);
        return epoll_ctl(epfd_, EPOLL_CTL_MOD, socks, &ev);
    }

    int co_read(int socks, uint8_t *buf, size_t read_count, bool over = false)
    {
        //printf("sock %d, read 1\n", socks);
        modify_mod_add(socks, this, EPOLLIN);
        co_ptr_->yield(0);
        //printf("sock %d, read 2\n", socks);
        int n = 0;
        while (true)
        {
            n = read(socks, buf, read_count);

            if (n == -1 && errno == EAGAIN)
            {
                co_ptr_->yield(0);
                continue;
            }

            break;
        }

        modify_mod_del(socks, this, EPOLLIN);
        return n;
        // int readed = 0;
        // int last_read = read_count;
        // int n = 0;
        // while (true)
        // {

        //     if (n == 0)
        //     {
        //         // 关闭了连接，这里不处理
        //         printf("n is zeor\n");
        //         break;
        //     }
        //     else if (n < 0)
        //     {
        //         if (n == -1 && errno == EAGAIN)
        //         {
        //             // test[this] += 1;
        //             // if(test[this] > 10)
        //             // {
        //             //     exit(-1);
        //             // }

        //             printf("[%p] read error n is %d, %d:%d\n", this, socks, n, errno);
        //             co_ptr_->yield(0); // 退出
        //             continue;
        //         }

        //         break;
        //     }
        //     else
        //     {
        //         readed += n;
        //         last_read -= n;
        //     }

        //     if (over)
        //     {
        //         printf("[%p] read count %d, last_read %d\n", this, readed, last_read);
        //         return readed;
        //     }

        //     if (readed < read_count)
        //     {
        //         modify_mod(socks, this, EPOLLIN);
        //         co_ptr_->yield(0); // 退出
        //         continue;
        //     }
        //     else
        //     {
        //         return readed;
        //     }
        // };

        // return n;
    }

    int co_send(int socks, void *trans, uint8_t *buf, size_t write_count)
    {
        modify_mod_add(socks, this, EPOLLOUT);
        co_ptr_->yield(0);
        int n = 0;
        while (true)
        {
            n = write(socks, buf, write_count);
            if (n == -1 && errno == EAGAIN)
            {
                co_ptr_->yield(0);
                continue;
            }

            break;
        }

        modify_mod_del(socks, this, EPOLLOUT);
        return n;

        // test[this] = 0;
        // int writed = 0;
        // int last_write = write_count;
        // int n = 0;
        // while (1)
        // {
        //     printf("[%p] buf:%p, write_count:%u\n", this, buf, write_count);
        //     int n = write(socks, buf, write_count);
        //     if (n == 0)
        //     {
        //         // 链接关闭了，返回由上层处理
        //         printf("write n is zero\n");
        //         break;
        //     }
        //     if (n < 0)
        //     {
        //         //EAGAIN;
        //         if (n == -1 && errno == EAGAIN)
        //         {
        //             modify_mod(socks, trans, EPOLLOUT);
        //             printf("[%p] write n is %d:%d\n", this, n, errno);
        //             co_ptr_->yield(0); // 退出
        //             continue;
        //         }

        //         break;
        //     }
        //     else
        //     {
        //         writed += n;
        //         last_write -= n;
        //     }

        //     if (writed < write_count)
        //     {
        //         printf("little %d:%d\n", writed, write_count);
        //         modify_mod(socks, trans, EPOLLOUT);
        //         co_ptr_->yield(0); // 退出
        //         continue;
        //     }
        //     else
        //     {
        //         //printf("real write %d\n", writed);

        //         return writed;
        //     }
        // };

        // return n;
    }

    void warp(Coroutine *this_)
    {
        co_ptr_ = this_;
        run();
    }

    virtual void run() = 0;

protected:
    Coroutine co_;
    Coroutine *co_ptr_;
    int epfd_;
};

#endif