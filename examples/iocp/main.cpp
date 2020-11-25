#include <winsock2.h>
#include <stdio.h>
#pragma comment(lib,"ws2_32.lib")

#define PORT 5000
#define MSGSIZE 1024

//该结构体包含重叠IO结构，主要用来进行重叠操作
typedef struct {

    OVERLAPPED overlapped;
    WSABUF WsaBuf;
    char buffer[MSGSIZE];
    DWORD RecvBytes;
}MyOverData, *LPMyOverData;

//该结构体包含套接字信息，用来作为完成键
typedef struct {
    SOCKET s;
    SOCKADDR_IN sockaddr;
}MySockData, *LPMySockData;

//服务线程
DWORD WINAPI ServerThread(LPVOID lpParam);

int main() {

    //定义变量
    WSADATA wsaData;
    SOCKET sListen, sAccept;
    SOCKADDR_IN ServerAddr, ClientAddr;
    HANDLE CompletionPort;
    SYSTEM_INFO SystemInfo;
    LPMyOverData lpmyoverdata;
    LPMySockData lpmysockdata;
    DWORD Flags = 0;

    //初始化套接字
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    //创建完成端口
    CompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

    //获取系统信息（cpu的核心数），原因是为了尽可能利用系统资源
    GetSystemInfo(&SystemInfo);

    //创建线程池，线程的个数为cpu核心数2倍
    for (UINT i = 0; i < SystemInfo.dwNumberOfProcessors * 2; i++) {
        CreateThread(NULL, 0, ServerThread, (LPVOID)CompletionPort, NULL, NULL);
    }

    //开始创建监听套接字
    sListen = socket(AF_INET, SOCK_STREAM, 0);
    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = htons(PORT);
    ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(sListen, (SOCKADDR*)&ServerAddr, sizeof(SOCKADDR));
    listen(sListen, 5);

    while (1) {
        int addrlen = sizeof(SOCKADDR);

        //获取客户端套接字
        sAccept = accept(sListen, (SOCKADDR*)&ClientAddr, &addrlen);

        //为自定义的结构体指针分配空间
        lpmyoverdata = (LPMyOverData)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(MyOverData));
        lpmysockdata = (LPMySockData)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(MySockData));

        //给结构体赋值，方便在子线程中使用
        lpmysockdata->s = sAccept;
        lpmysockdata->sockaddr = ClientAddr;
        lpmyoverdata->WsaBuf.len = MSGSIZE;
        lpmyoverdata->WsaBuf.buf = lpmyoverdata->buffer;

        //这里是该函数的第二个用途，将套接字和完成端口进行绑定，并将自定义的结构体作为完成键参数
        CreateIoCompletionPort((HANDLE)sAccept, CompletionPort, (DWORD)lpmysockdata, 0);

        //进行重叠IO操作，这里使用自定义的重叠IO结构作为重叠结构体指针
        WSARecv(lpmysockdata->s, &lpmyoverdata->WsaBuf, 1, &lpmyoverdata->RecvBytes,
            &Flags, &lpmyoverdata->overlapped, NULL);
    }
    return 0;
}



DWORD WINAPI ServerThread(LPVOID lpParam) {
    //从线程参数中获取完成端口句柄
    HANDLE CompletionPort = (HANDLE)lpParam;

    //创建变量指针
    LPMyOverData lpmyoverdata;
    LPMySockData lpmysockdata;
    DWORD Flags = 0;
    DWORD BytesTransferred;
    while (1) {

        //等待函数，当IO完成后该函数将返回
        GetQueuedCompletionStatus(CompletionPort,
            &BytesTransferred,
            (LPDWORD)&lpmysockdata,        //这个参数由CreateIoCompletionPort()函数的完成键参数确定
            (LPOVERLAPPED*)&lpmyoverdata,  //这个参数由WSARecv()函数的lpOverlapped参数确定
            INFINITE);

        //链接关闭，扫尾
        if (BytesTransferred == 0) {

            printf("closesocket %d\n", lpmysockdata->s);

            if (closesocket(lpmysockdata->s) == SOCKET_ERROR) {
                printf("close socket error %d\n", WSAGetLastError());
                return 0;
            }
            HeapFree(GetProcessHeap(), 0, lpmysockdata);
            HeapFree(GetProcessHeap(), 0, lpmyoverdata);
            continue;
        }

        //收到消息，进行消息处理
        else {
            printf("%s\n", lpmyoverdata->buffer);
            char msg[] = "收到消息!";
            send(lpmysockdata->s, msg, sizeof(msg), 0);
        }

        //重置变量
        Flags = 0;
        memset(&lpmyoverdata->overlapped, 0, sizeof(OVERLAPPED));
        lpmyoverdata->WsaBuf.len = MSGSIZE;
        lpmyoverdata->WsaBuf.buf = lpmyoverdata->buffer;

        //继续对套接字进行重叠IO操作
        WSARecv(lpmysockdata->s, &lpmyoverdata->WsaBuf, 1, &lpmyoverdata->RecvBytes,
            &Flags, &lpmyoverdata->overlapped, NULL);
    }
}