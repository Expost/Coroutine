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
#include <signal.h>

#include <map>
#include "client.h"

std::map<void *, int> test;
std::map<int, Dispatch *> sock_dis;

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

// curl --socks5 118.126.91.31:5000  https://www.baidu.com
int main()
{
    //signal(SIGPIPE, SIG_IGN);
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    sigprocmask(SIG_BLOCK, &set, NULL);

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
    char *local_addr = "0.0.0.0";
    inet_aton(local_addr, &(serveraddr.sin_addr));
    serveraddr.sin_port = htons(5000);
    bind(listenfd, (sockaddr *)&serveraddr, sizeof(serveraddr));
    listen(listenfd, 1024);

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

                setnonblocking(connfd);
                char *str = inet_ntoa(clientaddr.sin_addr);
                Trans *tmp_co = new Client(epfd, connfd);
                //ev.data.ptr = tmp_co;
                //ev.events = EPOLLIN;
                //epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);
                printf("accept a connnect from %s, %p\n", str, tmp_co);
                tmp_co->resume(0);
            }
            else
            {
                Dispatch *dispatch = (Dispatch *)events[i].data.ptr;
                if (events[i].events & EPOLLIN)
                {
                    if (dispatch == NULL || dispatch->on_in == NULL)
                    {
                        printf("on in is null\n");
                        continue;
                    }

                    //printf("EPOLLIN 1\n");
                    dispatch->on_in->resume(0);
                    //printf("EPOLLIN 2\n");
                }
                else if (events[i].events & EPOLLOUT)
                {
                    if (dispatch == NULL || dispatch->on_out == NULL)
                    {
                        printf("on out is null\n");
                        continue;
                    }
                    //printf("EPOLLOUT 1\n");
                    dispatch->on_out->resume(0);
                    //printf("EPOLLOUT 2\n");
                }
                else if(events[i].events & EPOLLHUP)
                {
                    printf("EPOLLHUP\n");
                    del_trans(epfd, events[i].data.fd);
                }
                else
                {
                    printf("Other event %d\n", events[i].events);
                    exit(-1);
                }

                // if (dispatch && dispatch->on_in && dispatch->on_in->is_end())
                // {
                //     delete dispatch->on_in;
                //     dispatch->on_in = nullptr;
                // }
                // if (dispatch && dispatch->on_out && dispatch->on_out->is_end())
                // {
                //     del_trans(epfd, events[i].data.fd);
                //     delete dispatch->on_out;
                //     dispatch->on_out = nullptr;
                // }

                // if (dispatch && dispatch->on_in == nullptr && dispatch->on_out == nullptr)
                // {
                //     delete dispatch;
                //     sock_dis[events[i].data.fd] = nullptr;
                // }
            }
        }
    }

    return 0;
}