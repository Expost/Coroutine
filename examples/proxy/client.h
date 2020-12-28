#ifndef __CLIENT_H_
#define __CLIENT_H_

#include "trans.h"
#include <netdb.h>
#include <arpa/inet.h>

//extern void print(...);

void setnonblocking(int sock);
class Remote : public Trans
{
public:
    Remote(int epfd, int sock, int local_sock, void *local_trans) : Trans(epfd),
                                                                    remote_sock_(sock),
                                                                    local_sock_(local_sock),
                                                                    local_trans_(local_trans)
    {
    }

    ~Remote() = default;

public:
    void run() override
    {
        int n = 0;
        uint8_t buf[4096] = {0};
        int count = 0;
        int sum = 0;
        while ((n = co_read(remote_sock_, buf, sizeof(buf), true)) > 0)
        {
            // printf("remote read %p %d and send_ptr %p, %c, %d\n", &buf, n, local_trans_, buf[0], count);
            //printf("[%d] remote read %d, %d, %u, %c\n", remote_sock_, count, n, sum, buf[0]);
            n = co_send(local_sock_, buf, n);
            //printf("[%p] local write %d\n", this, n);
            sum += n;
            count += 1;
        }

        del_trans(remote_sock_, EPOLLIN);
        close(remote_sock_);
        close(local_sock_);
        printf("remote close socks\n");
    }

private:
    int remote_sock_;
    int local_sock_;
    void *local_trans_;
};

class Client : public Trans
{
public:
    Client(int epfd, int sock) : Trans(epfd), client_socks_(sock)
    {
    }

    ~Client() = default;

private:
    Trans *remote_trans_;
    int client_socks_;

public:
    void run() override
    {
        uint8_t buf[4096] = {0};
        int n = co_read(client_socks_, buf, 100, true); // 0表示一下子读完
        if (n <= 0)
        {
            printf("error n is %d\n", n);
            return;
        }

        if (buf[0] != 5)
        {
            printf("socks ver is not 5, it's %d", buf[0]);
            return;
        }

        buf[0] = 5;
        buf[1] = 0;
        n = co_send(client_socks_, buf, 2);
        if (n <= 0)
        {
            printf("send error n is %d\n", n);
            return;
        }

        n = co_read(client_socks_, buf, 100, true);
        if (buf[1] != 1)
        {
            printf("just support connect\n");
            return;
        }

        uint32_t ip = 0;
        char tmp_ip[50];
        switch (buf[3])
        {
        case 1:
        {
            memcpy(&ip, &buf[4], sizeof(ip));
            printf("ip is %u\n", ip);
            break;
        }
        case 3:
        {
            char name[100] = {0};
            memcpy(name, &buf[5], n - 7);
            printf("domain name is %s\n", name);
            struct hostent *ht = gethostbyname(name);
            if (!ht)
            {
                return;
            }

            memcpy(&ip, &ht->h_addr_list[0], sizeof(ip));

            const char *ptr = inet_ntop(ht->h_addrtype, ht->h_addr_list[0], tmp_ip, sizeof(tmp_ip));
            printf("ip is %s\n", tmp_ip);
            break;
        }
        default:
        {
            printf("not support type %d\n", buf[3]);
        }
        }

        short port = ntohs(*(short *)&buf[n - 2]);

        printf("port is %u\n", port);
        uint8_t send_buf[] = {0x05, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        co_send(client_socks_, send_buf, sizeof(send_buf));

        int remote_sock = socket(AF_INET, SOCK_STREAM, 0);

        struct sockaddr_in servaddr;
        servaddr.sin_port = htons((short)port);
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = inet_addr(tmp_ip); //此处更改epoll服务器地址

        setnonblocking(remote_sock);

        if (connect(remote_sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        {
            if (errno == EINPROGRESS)
            {
                printf("connect EINPROGRESS\n");
                del_trans(client_socks_, EPOLLOUT | EPOLLIN);
                add_trans(remote_sock, this, EPOLLOUT | EPOLLIN);
                int event = co_ptr_->yield(0);

                if (event == EPOLLERR)
                {
                    printf("connect failed\n");
                    return;
                }
                else if (event == EPOLLOUT)
                {
                    printf("connect successful\n");
                }
                else
                {
                    printf("event is %d\n", event);
                }

                del_trans(remote_sock, EPOLLOUT);
                remote_trans_ = new Remote(epfd_, remote_sock, client_socks_, this);
                add_trans(remote_sock, remote_trans_, EPOLLIN);
                add_trans(client_socks_, this, EPOLLIN);
            }
            else
            {
                printf("connect %s:%lu failed\n", tmp_ip, port);
                return;
            }
        }

        printf("start read data\n");
        while ((n = co_read(client_socks_, buf, 4096, true)) > 0)
        {
            //printf("read n is %d\n", n);
            n = co_send(remote_sock, buf, n);
            //printf("write n is %d\n", n);
        }

        del_trans(client_socks_, EPOLLIN);
        close(client_socks_);
        close(remote_sock);
        printf("local close socks\n");
    }
};

#endif