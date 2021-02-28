#ifndef __CLIENT_H_
#define __CLIENT_H_

#include "trans.h"
#include <netdb.h>
#include <arpa/inet.h>

void setnonblocking(int sock);
class Remote : public Trans
{
public:
    Remote(int epfd, int sock, int local_sock) : Trans(epfd),
                                                 remote_sock_(sock),
                                                 local_sock_(local_sock)
    {
        //add_trans(epfd, sock, shared_from_this(), EPOLLHUP);
    }

    ~Remote()
    {
        printf("~Remote\n");
    }

public:
    void run() override
    {
        int n = 0;
        uint8_t buf[4096] = {0};
        int count = 0;
        int sum = 0;
        printf("remote start\n");
        while ((n = co_read(remote_sock_, buf, sizeof(buf), true)) > 0)
        {
            // printf("remote read %p %d and send_ptr %p, %c, %d\n", &buf, n, local_trans_, buf[0], count);
            //printf("[%d] remote read %d, %d, %u, %c\n", remote_sock_, count, n, sum, buf[0]);
            n = co_send(local_sock_, buf, n);
            //printf("[%p] local write %d\n", this, n);
            sum += n;
            count += 1;
        }

        // 如果远程sock读失败，那么就不管了，直接退出。
        // local sock肯定会写失败，由local的协程去处理
        // del_trans(remote_sock_, EPOLLIN);
        // close(remote_sock_);
        // close(local_sock_);
        // printf("remote close socks\n");

        close(remote_sock_);
        del_trans(epfd_, remote_sock_);
    }

private:
    int remote_sock_;
    int local_sock_;
};

class Client : public Trans
{
public:
    Client(int epfd, int sock) : Trans(epfd), client_socks_(sock)
    {
        //add_trans(epfd, sock, shared_from_this(), EPOLLHUP);
    }

    ~Client()
    {
        printf("~Client\n");
    }

private:
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

        std::shared_ptr<Remote> remote_trans = nullptr;
        if (connect(remote_sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        {
            if (errno == EINPROGRESS)
            {
                remote_trans = std::make_shared<Remote>(epfd_, remote_sock, client_socks_);

                add_trans(epfd_, remote_sock, remote_trans, EPOLLHUP);

                modify_mod_add(remote_sock, EPOLLOUT);
                co_ptr_->yield(0);
                modify_mod_del(remote_sock, EPOLLOUT);
                //del_trans(epfd_, remote_sock);

                remote_trans->resume(0);
            }
            else
            {
                printf("unknown error\n");
                return;
            }
        }
        else
        {
            remote_trans = std::make_shared<Remote>(epfd_, remote_sock, client_socks_);
            add_trans(epfd_, remote_sock, remote_trans, EPOLLHUP);

            remote_trans->resume(0);
            printf("connect suc directly\n");
        }

        printf("start read data\n");
        while ((n = co_read(client_socks_, buf, 4096, true)) > 0)
        {
            //printf("client read n is %d\n", n);
            n = co_send(remote_sock, buf, n);
            if (n <= 0)
            {
                break;
            }
            //printf("client write n is %d\n", n);
        }

        printf("socks %d, n is %d, and error is %d\n", client_socks_, n, errno);

        close(client_socks_);
        del_trans(epfd_, client_socks_);

        printf("local close socks\n");
    }
};

#endif