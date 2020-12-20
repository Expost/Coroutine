#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <functional>

#include "xco.h"

void setnonblocking(int sock)
{
    int opts = fcntl(sock, F_GETFL);
    if (opts < 0)
    {
        perror("fcntl(sock, GETFL)");
        exit(1);
    }

    opts = opts | O_NONBLOCK;
    if (fcntl(sock, F_SETFL, opts) < 0)
    {
        perror("fcntl(sock, SETFL, opts)");
        exit(1);
    }
}

class Trans
{
public:
    Trans(int epfd, int sock) : epfd_(epfd),
                                client_sock_(sock),
                                co_(std::bind(&Trans::warp, this, std::placeholders::_1))
    {
        // co_.resume(NULL);
    }

    ~Trans() = default;

public:
    int resume(int value)
    {
        co_.resume(value);
    }

    int co_read(uint8_t *buf, size_t read_count)
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
                    co_ptr_->yield(NULL); // 退出
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

            if (readed < read_count)
            {
                epoll_event ev;
                ev.data.ptr = this;
                
                ev.events = EPOLLIN; // 这里继续读
                epoll_ctl(epfd_, EPOLL_CTL_MOD, client_sock_, &ev);
                co_ptr_->yield(NULL); // 退出
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
                    co_ptr_->yield(NULL); // 退出
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
                co_ptr_->yield(NULL); // 退出
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

    void run()
    {
        uint8_t buf[100] = {0};
        for (int i = 0; i < 3; i++)
        {
            int n = co_read(buf, 9);
            if (n <= 0)
            {
                printf("error n is %d\n", n);
                return;
            }

            printf("read buf is %s\n", buf);

            n = co_send(buf, 10);
            printf("echo send %s\n", buf);
        }
    }

public:
    Coroutine co_;
    Coroutine *co_ptr_;
    int client_sock_;
    int epfd_;
};

int main()
{
    int epfd = epoll_create(256);
    sockaddr_in clientaddr;
    sockaddr_in serveraddr;

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);

    setnonblocking(listenfd);
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt));

    epoll_event ev, events[20];
    ev.data.fd = listenfd;
    ev.events = EPOLLIN;

    epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    char *local_addr = "127.0.0.1";
    inet_aton(local_addr, &(serveraddr.sin_addr));
    serveraddr.sin_port = htons(5000);
    bind(listenfd, (sockaddr *)&serveraddr, sizeof(serveraddr));
    listen(listenfd, 20);

    const int MAXLINE = 4096;
    char line[MAXLINE];
    int n = 0;

    while (true)
    {
        int nfds = epoll_wait(epfd, events, 20, 500);
        for (int i = 0; i < nfds; i++)
        {
            if (events[i].data.fd == listenfd)
            {
                socklen_t client_len = 0;
                int connfd = accept(listenfd, (sockaddr *)&clientaddr, &client_len);
                if (connfd < 0)
                {
                    perror("connfd < 0");
                    exit(1);
                }

                char *str = inet_ntoa(clientaddr.sin_addr);
                printf("accept a connnect from %s\n", str);
                Trans *tmp_co = new Trans(epfd, connfd);
                ev.data.ptr = tmp_co;
                ev.events = EPOLLIN;

                epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);
            }
            else if (events[i].events & EPOLLIN)
            {
                Trans *trans = (Trans *)events[i].data.ptr;
                trans->resume(0);
                // printf("EPOLLIN\n");
                // int sockfd = events[i].data.fd;
                // if (sockfd < 0)
                // {
                //     continue;
                // }

                // n = read(sockfd, line, MAXLINE);
                // if (n < 0)
                // {
                //     if (errno == ECONNRESET)
                //     {
                //         printf("reset\n");
                //         close(sockfd);
                //         events[i].data.fd = -1;
                //     }
                //     else
                //     {
                //         printf("readline error\n");
                //     }
                // }
                // else if (n == 0)
                // {
                //     printf("n is zero\n");
                //     close(sockfd);
                //     events[i].data.fd = -1;
                //     continue;
                // }
                // if (n < MAXLINE - 2)
                // {
                //     line[n] = 0;
                // }

                // printf("data is %s\n", line);
                // ev.data.fd = sockfd;
                // ev.events = EPOLLOUT | EPOLLET;
                // epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev);
            }
            else if (events[i].events & EPOLLOUT)
            {
                Trans *trans = (Trans *)events[i].data.ptr;
                trans->resume(0);
                // printf("EPOLL OUT and line is:%s, len is:%d\n", line, n);
                // int sockfd = events[i].data.fd;
                // write(sockfd, line, n);
                // ev.data.fd = sockfd;
                // ev.events = EPOLLIN | EPOLLET;
                // epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev);
            }
        }
    }

    return 0;
}