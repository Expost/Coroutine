#include <winsock2.h>
#include <stdio.h>
#pragma comment(lib,"ws2_32.lib")

#define PORT 5000
#define MSGSIZE 1024

//�ýṹ������ص�IO�ṹ����Ҫ���������ص�����
typedef struct {

    OVERLAPPED overlapped;
    WSABUF WsaBuf;
    char buffer[MSGSIZE];
    DWORD RecvBytes;
}MyOverData, *LPMyOverData;

//�ýṹ������׽�����Ϣ��������Ϊ��ɼ�
typedef struct {
    SOCKET s;
    SOCKADDR_IN sockaddr;
}MySockData, *LPMySockData;

//�����߳�
DWORD WINAPI ServerThread(LPVOID lpParam);

int main() {

    //�������
    WSADATA wsaData;
    SOCKET sListen, sAccept;
    SOCKADDR_IN ServerAddr, ClientAddr;
    HANDLE CompletionPort;
    SYSTEM_INFO SystemInfo;
    LPMyOverData lpmyoverdata;
    LPMySockData lpmysockdata;
    DWORD Flags = 0;

    //��ʼ���׽���
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    //������ɶ˿�
    CompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

    //��ȡϵͳ��Ϣ��cpu�ĺ���������ԭ����Ϊ�˾���������ϵͳ��Դ
    GetSystemInfo(&SystemInfo);

    //�����̳߳أ��̵߳ĸ���Ϊcpu������2��
    for (UINT i = 0; i < SystemInfo.dwNumberOfProcessors * 2; i++) {
        CreateThread(NULL, 0, ServerThread, (LPVOID)CompletionPort, NULL, NULL);
    }

    //��ʼ���������׽���
    sListen = socket(AF_INET, SOCK_STREAM, 0);
    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = htons(PORT);
    ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(sListen, (SOCKADDR*)&ServerAddr, sizeof(SOCKADDR));
    listen(sListen, 5);

    while (1) {
        int addrlen = sizeof(SOCKADDR);

        //��ȡ�ͻ����׽���
        sAccept = accept(sListen, (SOCKADDR*)&ClientAddr, &addrlen);

        //Ϊ�Զ���Ľṹ��ָ�����ռ�
        lpmyoverdata = (LPMyOverData)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(MyOverData));
        lpmysockdata = (LPMySockData)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(MySockData));

        //���ṹ�帳ֵ�����������߳���ʹ��
        lpmysockdata->s = sAccept;
        lpmysockdata->sockaddr = ClientAddr;
        lpmyoverdata->WsaBuf.len = MSGSIZE;
        lpmyoverdata->WsaBuf.buf = lpmyoverdata->buffer;

        //�����Ǹú����ĵڶ�����;�����׽��ֺ���ɶ˿ڽ��а󶨣������Զ���Ľṹ����Ϊ��ɼ�����
        CreateIoCompletionPort((HANDLE)sAccept, CompletionPort, (DWORD)lpmysockdata, 0);

        //�����ص�IO����������ʹ���Զ�����ص�IO�ṹ��Ϊ�ص��ṹ��ָ��
        WSARecv(lpmysockdata->s, &lpmyoverdata->WsaBuf, 1, &lpmyoverdata->RecvBytes,
            &Flags, &lpmyoverdata->overlapped, NULL);
    }
    return 0;
}



DWORD WINAPI ServerThread(LPVOID lpParam) {
    //���̲߳����л�ȡ��ɶ˿ھ��
    HANDLE CompletionPort = (HANDLE)lpParam;

    //��������ָ��
    LPMyOverData lpmyoverdata;
    LPMySockData lpmysockdata;
    DWORD Flags = 0;
    DWORD BytesTransferred;
    while (1) {

        //�ȴ���������IO��ɺ�ú���������
        GetQueuedCompletionStatus(CompletionPort,
            &BytesTransferred,
            (LPDWORD)&lpmysockdata,        //���������CreateIoCompletionPort()��������ɼ�����ȷ��
            (LPOVERLAPPED*)&lpmyoverdata,  //���������WSARecv()������lpOverlapped����ȷ��
            INFINITE);

        //���ӹرգ�ɨβ
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

        //�յ���Ϣ��������Ϣ����
        else {
            printf("%s\n", lpmyoverdata->buffer);
            char msg[] = "�յ���Ϣ!";
            send(lpmysockdata->s, msg, sizeof(msg), 0);
        }

        //���ñ���
        Flags = 0;
        memset(&lpmyoverdata->overlapped, 0, sizeof(OVERLAPPED));
        lpmyoverdata->WsaBuf.len = MSGSIZE;
        lpmyoverdata->WsaBuf.buf = lpmyoverdata->buffer;

        //�������׽��ֽ����ص�IO����
        WSARecv(lpmysockdata->s, &lpmyoverdata->WsaBuf, 1, &lpmyoverdata->RecvBytes,
            &Flags, &lpmyoverdata->overlapped, NULL);
    }
}