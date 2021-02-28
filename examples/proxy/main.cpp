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

    time_t last_time = time(NULL);
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
                auto tmp_co = std::make_shared<Client>(epfd, connfd);
                add_trans(epfd, connfd, tmp_co, EPOLLHUP);
                //ev.data.ptr = tmp_co;
                //ev.events = EPOLLIN;
                //epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);
                printf("accept a connnect %d from %s, %p\n", connfd, str, tmp_co);
                tmp_co->resume(0);
            }
            else
            {
                Dispatch *dispatch = (Dispatch *)events[i].data.ptr;
                if (dispatch == NULL)
                {
                    continue;
                }

                if (events[i].events & EPOLLIN)
                {
                    auto in_routine = dispatch->on_in;
                    if (in_routine == NULL)
                    {
                        printf("on in is null\n");
                        continue;
                    }
                    in_routine->resume(0);
                }
                else if (events[i].events & EPOLLOUT)
                {
                    auto out_routine = dispatch->on_out;
                    if (out_routine == NULL)
                    {
                        printf("on out is null\n");
                        continue;
                    }

                    out_routine->resume(0);
                }
                else if (events[i].events & EPOLLHUP)
                {
                    int error_sock = dispatch->socket;
                    printf("socks %d EPOLLHUP\n", error_sock);
                    del_trans(epfd, error_sock);
                    close(error_sock);
                    //exit(-1);
                }
                else
                {
                    printf("Other event %d\n", events[i].events);
                    exit(-1);
                }
            }
        }

        time_t now = time(NULL);
        if (now - last_time >= 5)
        {
            last_time = now;
            printf("inter\n");
            for (auto &itr : sock_dis)
            {
                printf("sock:%d, ptr:%p\n", itr.first, itr.second);
            }
        }
    }

    return 0;
}