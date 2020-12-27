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
        
        int readed = 0;
        int last_read = read_count;
        int n = 0;
        while (true)
        {
            n = read(socks, &buf[readed], last_read);

            if (n == 0)
            {
                // 关闭了连接，这里不处理
                printf("n is zeor\n");
                break;
            }
            else if (n < 0)
            {
                if (n == -1 && errno == EAGAIN)
                {
                    // test[this] += 1;
                    // if(test[this] > 10)
                    // {
                    //     exit(-1);
                    // }
                    modify_mod(socks, this, EPOLLIN);
                    printf("[%p] read error n is %d, %d:%d\n", this, socks, n, errno);
                    co_ptr_->yield(0); // 退出
                    continue;
                }
            
                break;
            }
            else
            {
                readed += n;
                last_read -= n;
            }

            if (over)
            {
                printf("[%p] read count %d, last_read %d\n", this, readed, last_read);
                return readed;
            }

            if (readed < read_count)
            {
                 modify_mod(socks, this, EPOLLIN);
                co_ptr_->yield(0); // 退出
                continue;
            }
            else
            {
                return readed;
            }
        };

        return n;
    }

    int co_send(int socks, void *trans, uint8_t *buf, size_t write_count)
    {
        test[this] = 0;
        int writed = 0;
        int last_write = write_count;
        int n = 0;
        while (1)
        {
            printf("[%p] buf:%p, write_count:%u\n", this, buf, write_count);
            int n = write(socks, buf, write_count);
            if (n == 0)
            {
                // 链接关闭了，返回由上层处理
                printf("write n is zero\n");
                break;
            }
            if (n < 0)
            {
                //EAGAIN;
                if (n == -1 && errno == EAGAIN)
                {
                    modify_mod(socks, trans, EPOLLOUT);
                    printf("[%p] write n is %d:%d\n", this, n, errno);
                    co_ptr_->yield(0); // 退出
                    continue;
                }

                break;
            }
            else
            {
                writed += n;
                last_write -= n;
            }

            if (writed < write_count)
            {
                printf("little %d:%d\n", writed, write_count);
                modify_mod(socks, trans, EPOLLOUT);
                co_ptr_->yield(0); // 退出
                continue;
            }
            else
            {
                //printf("real write %d\n", writed);
                
                return writed;
            }
        };

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