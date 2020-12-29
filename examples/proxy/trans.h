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

void print(...)
{
}

class Trans;
struct CallBack
{
    Trans* in = nullptr;
    Trans* out = nullptr;
};

class Trans
{
public:
    Trans(int epfd) : epfd_(epfd),
                      co_(std::bind(&Trans::warp, this, std::placeholders::_1))
    {
    }

    ~Trans() = default;

public:
    int resume(int value)
    {
        co_.resume(value);
    }

    int add_trans(int socks, Trans *trans, int oper)
    {
        epoll_event ev;
        ev.data.ptr = trans;
        ev.events = oper;

        return epoll_ctl(epfd_, EPOLL_CTL_ADD, socks, &ev);
    }

    int modify_mod(int socks, void *trans, int oper)
    {
        epoll_event ev;
        ev.data.ptr = trans;
        ev.events = oper;
        return epoll_ctl(epfd_, EPOLL_CTL_MOD, socks, &ev);
    }

    int del_trans(int socks, int oper)
    {
        epoll_event ev;
        ev.data.ptr = this;
        ev.events = oper;

        return epoll_ctl(epfd_, EPOLL_CTL_DEL, socks, &ev);
    }

    int co_read(int socks, uint8_t *buf, size_t read_count, bool over = false)
    {
        printf("[%p] %d start\n", this, socks);
        //modify_mod(socks, this, EPOLLIN);
        co_ptr_->yield(0); // 退出
        printf("[%p] %d end\n", this, socks);

        int n = read(socks, buf, read_count);
        return n;
    }

    int co_send(int socks, uint8_t *buf, size_t write_count)
    //int co_send(int socks, void *trans, uint8_t *buf, size_t write_count)
    {
        //modify_mod(socks, this, EPOLLOUT);
        
        co_ptr_->yield(0); // 退出
        int n = write(socks, buf, write_count);

        return n;
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