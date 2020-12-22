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
    Trans(int epfd, int sock) : epfd_(epfd),
                                client_sock_(sock),
                                co_(std::bind(&Trans::warp, this, std::placeholders::_1))
    {
    }

    ~Trans() = default;

public:
    int resume(int value)
    {
        co_.resume(value);
    }

    int co_read(uint8_t *buf, size_t read_count, bool over = false)
    {
        int readed = 0;
        int last_read = read_count;
        int n = 0;
        while (true)
        {
            n = read(client_sock_, &buf[readed], last_read);

            if (n == 0)
            {
                // 关闭了连接，这里不处理
                printf("n is zeor\n");
                break;
            }
            else if (n < 0)
            {
                // 暂不处理
                if (n == 1 && errno == EAGAIN)
                {
                    epoll_event ev;
                    ev.data.ptr = this;
                    ev.events = EPOLLIN; // 这里继续读
                    epoll_ctl(epfd_, EPOLL_CTL_MOD, client_sock_, &ev);
                    co_ptr_->yield(0); // 退出
                    printf("continue 1\n");
                    continue;
                }

                printf("error n is %d\n", n);
                break;
            }
            else
            {
                readed += n;
                last_read -= n;
            }

            if(over)
            {
                return readed;
            }

            if (readed < read_count)
            {
                epoll_event ev;
                ev.data.ptr = this;
                
                ev.events = EPOLLIN; // 这里继续读
                epoll_ctl(epfd_, EPOLL_CTL_MOD, client_sock_, &ev);
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

    int co_send(uint8_t *buf, size_t write_count)
    {
        int writed = 0;
        int last_write = write_count;
        int n = 0;
        while (1)
        {
            int n = write(client_sock_, buf, write_count);
            if (n == 0)
            {
                // 链接关闭了，返回由上层处理
                break;
            }
            if (n < 0)
            {
                if (n == -1 && errno == EAGAIN)
                {
                    epoll_event ev;
                    ev.data.ptr = this;
                    ev.events = EPOLLOUT; // 这里继续读
                    epoll_ctl(epfd_, EPOLL_CTL_MOD, client_sock_, &ev);
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
                epoll_event ev;
                ev.data.ptr = this;
                ev.events = EPOLLOUT; // 这里继续读
                epoll_ctl(epfd_, EPOLL_CTL_MOD, client_sock_, &ev);
                co_ptr_->yield(0); // 退出
                continue;
            }
            else
            {
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

public:
    Coroutine co_;
    Coroutine *co_ptr_;
    int client_sock_;
    int epfd_;
};

#endif