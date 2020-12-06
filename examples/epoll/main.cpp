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
    ev.events = EPOLLIN | EPOLLET;

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
                ev.data.fd = connfd;
                ev.events = EPOLLIN | EPOLLET;

                epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);
            }
            else if (events[i].events & EPOLLIN)
            {
                printf("EPOLLIN\n");
                int sockfd = events[i].data.fd;
                if (sockfd < 0)
                {
                    continue;
                }

                n = read(sockfd, line, MAXLINE);
                if (n < 0)
                {
                    if (errno == ECONNRESET)
                    {
                        printf("reset\n");
                        close(sockfd);
                        events[i].data.fd = -1;
                    }
                    else
                    {
                        printf("readline error\n");
                    }
                }
                else if (n == 0)
                {
                    printf("n is zero\n");
                    close(sockfd);
                    events[i].data.fd = -1;
                    continue;
                }
                if (n < MAXLINE - 2)
                {
                    line[n] = 0;
                }

                printf("data is %s\n", line);
                ev.data.fd = sockfd;
                ev.events = EPOLLOUT | EPOLLET;
                epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev);
            }
            else if (events[i].events & EPOLLOUT)
            {
                printf("EPOLL OUT and line is:%s, len is:%d\n", line, n);
                int sockfd = events[i].data.fd;
                write(sockfd, line, n);
                ev.data.fd = sockfd;
                ev.events = EPOLLIN | EPOLLET;
                epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev);
            }
        }
    }

    return 0;
}