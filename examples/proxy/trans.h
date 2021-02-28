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
#include <memory>
#include "xco.h"

class Trans;
struct Dispatch
{
    int socket = 0;
    int oper = 0;
    std::shared_ptr<Trans> on_in = nullptr;
    std::shared_ptr<Trans> on_out = nullptr;
    std::shared_ptr<Trans> on_hup = nullptr;

    ~Dispatch()
    {
        oper = 0;
        socket = 0;
        on_in = nullptr;
        on_out = nullptr;
        on_hup = nullptr;
    }
};

extern std::map<int, Dispatch *> sock_dis;

void mod(Dispatch *dispatch, int oper, std::shared_ptr<Trans> trans)
{
    if(dispatch == NULL)
    {
        printf("dispatch is null\n");
        return ;
    }

    if (oper & EPOLLIN)
    {
        dispatch->on_in = trans;
    }
    if (oper & EPOLLOUT)
    {
        dispatch->on_out = trans;
    }
    if(oper & EPOLLHUP)
    {
        dispatch->on_hup = trans;
    }
}

int add_trans(int epfd, int socks, std::shared_ptr<Trans> trans, int oper)
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

    auto itr = sock_dis.find(socks);
    if(itr != sock_dis.end())
    {
        delete itr->second;
        sock_dis.erase(itr);
    }
    
    return epoll_ctl(epfd, EPOLL_CTL_DEL, socks, &ev);
}

class Trans: public std::enable_shared_from_this<Trans>
{
public:
    Trans(int epfd) : epfd_(epfd),
                      co_(std::bind(&Trans::warp, this, std::placeholders::_1))
    {
    }

    virtual ~Trans()
    {
        printf("~Trans\n");
    }

public:
    bool is_end()
    {
        return co_.get_coroutine_state() == Coroutine::FINISHED;
    }

    int resume(int value)
    {
        co_.resume(value);
    }

    int modify_mod_add(int socks, int oper)
    {
        auto dispatch = sock_dis[socks];
        if(dispatch == nullptr)
        {
            printf("modify_mod_add dispatch is null\n");
            return -1;
        }

        epoll_event ev;
        //ev.data.ptr = trans;
        ev.data.ptr = dispatch;
        mod((Dispatch *)ev.data.ptr, oper, shared_from_this());
        ev.events = dispatch->oper | oper;
        dispatch->oper = ev.events;
        //printf("sock %d add %d and now oper is %d\n", socks, oper, ev.events);
        return epoll_ctl(epfd_, EPOLL_CTL_MOD, socks, &ev);
    }

    int modify_mod_del(int socks, int oper)
    {
        auto dispatch = sock_dis[socks];
        if(dispatch == nullptr)
        {
            printf("modify_mod_del dispatch is null\n");
            return -1;
        }

        epoll_event ev;

        ev.data.ptr = dispatch;
        //mod((Dispatch*)ev.data.ptr, oper, (Trans*)trans);
        dispatch->oper &= (~oper);
        ev.events = dispatch->oper;
        //printf("sock %d del %d and new oper is %d\n", socks, oper, ev.events);
        return epoll_ctl(epfd_, EPOLL_CTL_MOD, socks, &ev);
    }

    int co_read(int socks, uint8_t *buf, size_t read_count, bool over = false)
    {
        //printf("sock %d, read 1\n", socks);
        modify_mod_add(socks, EPOLLIN);
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

        modify_mod_del(socks, EPOLLIN);
        return n;
    }

    int co_send(int socks, uint8_t *buf, size_t write_count)
    {
        modify_mod_add(socks, EPOLLOUT);
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

        modify_mod_del(socks, EPOLLOUT);
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