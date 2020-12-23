#ifndef __CLIENT_H_
#define __CLIENT_H_

#include "trans.h"
#include <netdb.h>
#include <arpa/inet.h>

class Client : public Trans
{
public:
    Client(int epfd, int sock) : Trans(epfd, sock)
    {
    }

    ~Client() = default;

public:
    void run() override
    {
        uint8_t buf[100] = {0};
        int n = co_read(buf, 100, true); // 0表示一下子读完
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
        n = co_send(buf, 2);
        if (n <= 0)
        {
            printf("send error n is %d\n", n);
            return;
        }

        n = co_read(buf, 100, true);
        if (buf[1] != 1)
        {
            printf("just support connect\n");
            return;
        }

        uint32_t ip = 0;
        switch (buf[3])
        {
        case 1:
        {
            memcpy(&ip, &buf[4], sizeof(ip));
            printf("ip is %s\n", ip);
            break;
        }
        case 3:
        {
            char name[100] = {0};
            memcpy(name, &buf[5], n - 7);
            printf("domain name is %s\n", name);
            struct hostent *ht = gethostbyname(name);
            //memcpy(&ip, &ht->h_addr_list[0], sizeof(ip));
            printf("ht is %p\n", ht);
            char    tmp_ip[50];
            const char *ptr = inet_ntop(ht->h_addrtype, ht->h_addr_list[0], tmp_ip, sizeof(tmp_ip));
            printf("ip is %s\n", tmp_ip);
            break;
        }
        default:
        {
            printf("not support type %d\n", buf[3]);
        }
        }

        int port = ntohs(*(short*)&buf[n - 2]);

        printf("port is %u\n", port);
        uint8_t send_buf[] = {0x05, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        n = co_send(send_buf, sizeof(send_buf));

    }
};

#endif