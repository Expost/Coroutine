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

//std::map<void*, int> test;

#include "client.h"

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
                ev.data.ptr = tmp_co;
                ev.events = EPOLLIN;
                printf("accept a connnect from %s, %p\n", str, tmp_co);
                epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);
            }
            else if (events[i].events & EPOLLIN)
            {
                Trans *trans = (Trans *)events[i].data.ptr;
                //test[trans] = 0;
                trans->resume(EPOLLIN);
            }
            else if (events[i].events & EPOLLOUT)
            {
                Trans *trans = (Trans *)events[i].data.ptr;
                printf("[%p] out\n", trans);
                // if(test[trans] > 10)
                // {
                //     exit(-1);
                // }
                //test[trans] += 1;
                trans->resume(EPOLLOUT);
            }
            else if(events[i].events & EPOLLERR)
            {
                Trans *trans = (Trans *)events[i].data.ptr;
                printf("EPOLLERR with trans %p", trans);
                trans->resume(EPOLLERR);
            }
            else
            {
                Trans *trans = (Trans *)events[i].data.ptr;
                printf("unknown event %d with trans %p\n", events[i].events, trans);
            }
            
        }
    }

    return 0;
}