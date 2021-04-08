#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define _WINSOCK_DEPRECATED_NO_WARNINGS
    #include<windows.h>
    #include<WinSock2.h>
    #pragma comment(lib,"ws2_32.lib")
#else
    #include<unistd.h> //uni std
    #include<arpa/inet.h>
    #include<string.h>

    #define SOCKET int
    #define INVALID_SOCKET  (SOCKET)(~0)
    #define SOCKET_ERROR            (-1)
#endif

#include<stdio.h>
#include<thread>
#include<vector>


using namespace std;

enum CMD
{
    CMD_LOGIN,
    CMD_LOGIN_RESULT,
    CMD_LOGINOUT,
    CMD_LOGINOUT_RESULT,
    CMD_NEW_USER_JOIN,
    CMD_ERROR
};

struct DataHeader
{
    short dataLength;
    short cmd;
};

struct Login :public DataHeader//datapackage
{
    Login() {
        dataLength = sizeof(Login);
        cmd = CMD_LOGIN;
    }
    char userName[32];
    char PassWord[32];
};

struct LoginResult :public DataHeader
{
    LoginResult() {
        dataLength = sizeof(LoginResult);
        cmd = CMD_LOGIN_RESULT;
        result = 0;
    }
    int result;
};

struct Loginout :public DataHeader
{
    Loginout() {
        dataLength = sizeof(Loginout);
        cmd = CMD_LOGINOUT;
    }
    char username[32];
};

struct LoginOutResult :public DataHeader
{
    LoginOutResult() {
        dataLength = sizeof(LoginOutResult);
        cmd = CMD_LOGINOUT_RESULT;
        result = 1;
    }
    int result;
};

struct NewUserJoin :public DataHeader
{
    NewUserJoin() {
        dataLength = sizeof(NewUserJoin);
        cmd = CMD_NEW_USER_JOIN;
        sock = 1;
    }
    int sock;
};

vector<SOCKET> g_clients;

int processor(SOCKET _cSock)
{
    //������
    char szRecv[4096] = {};
    //���տͻ�������
    int nLen = (int)recv(_cSock, szRecv, sizeof(DataHeader), 0);
    DataHeader* header = (DataHeader*)szRecv;
    if (nLen <= 0)
    {
        printf("�ͻ���<SOCKET = %d>�˳�,�������� \n", _cSock);
        return -1;
    }
    switch (header->cmd)
    {
    case CMD_LOGIN:
    {
        //Login login;
        recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
        Login* login = (Login*)szRecv;
        //pass check password
        LoginResult ret;
        printf("�յ��ͻ���<SOCKET = %d>ָ�� CMD_LOGIN  ���ȣ�%d,username=%s,Password=%s\n", _cSock, login->dataLength, login->userName, login->PassWord);
        send(_cSock, (char*)&ret, sizeof(LoginResult), 0);
    }
    break;
    case CMD_LOGINOUT:
    {
        //Loginout loginOut;
        recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
        //pass check password
        Loginout* loginOut = (Loginout*)szRecv;
        printf("�յ��ͻ���<SOCKET = %d>ָ�� CMD_LOGIN  ���ȣ�%d,username=%s\n", _cSock, loginOut->dataLength, loginOut->username);
        LoginOutResult ret;
        send(_cSock, (char*)&ret, sizeof(ret), 0);
    }
    break;
    default:
        DataHeader header = { 0,CMD_ERROR };
        send(_cSock, szRecv, sizeof(header), 0);
        break;
    }
    return 0;
}

int main()
{
#ifdef _WIN32
    //����windowsocket����
    WORD ver = MAKEWORD(2, 2);
    WSADATA dat;
    WSAStartup(ver, &dat);
#endif
    //����һ��Socket
    SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//�׽���
    //bind�����ڽ��տͻ��˵�����˿�
    sockaddr_in _sin = {};
    _sin.sin_family = AF_INET;
    _sin.sin_port = htons(4567);
#ifdef _WIN32
    _sin.sin_addr.S_un.S_addr = INADDR_ANY;
    //_sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
#else
    _sin.sin_addr.s_addr = INADDR_ANY;
#endif
    //::bind���������macos10.15�汾
    if (SOCKET_ERROR == ::bind(_sock, (sockaddr*)&_sin, sizeof(_sin)))
    {
        printf("�˿ڰ�ʧ��\n");
    }
    else
    {
        printf("�˿ڰ󶨳ɹ�\n");
    }
    //listen��������˿�
    if (SOCKET_ERROR == listen(_sock, 5))
    {
        printf("�����˿ڰ�ʧ��\n");
    }
    else
    {
        printf("�����˿ڰ󶨳ɹ�\n");
    }
    while (true)
    {
        //������BSDsocket
        fd_set fdRead;//������
        fd_set fdWrite;
        fd_set fdExp;
        //������
        FD_ZERO(&fdRead);
        FD_ZERO(&fdWrite);
        FD_ZERO(&fdExp);
        //�����������뼯��
        FD_SET(_sock, &fdRead);
        FD_SET(_sock, &fdWrite);
        FD_SET(_sock, &fdExp);
        SOCKET maxSock = _sock;
        for (int n = (int)g_clients.size() - 1; n >= 0; n--)
        {
            FD_SET(g_clients[n], &fdRead);
            if (maxSock < g_clients[n])
            {
                maxSock = g_clients[n];
            }
        }
        //������������ֵ���ֵ+1 ��Windows�в�������д��0
        timeval t = { 1,0 };

        int ret = select(maxSock + 1, &fdRead, &fdWrite, &fdExp, &t);

        if (ret < 0)
        {
            printf("select�������� \n");
            break;
        }
        //�ж��������Ƿ��ڼ�����
        if (FD_ISSET(_sock, &fdRead))
        {
            FD_CLR(_sock, &fdRead);
            sockaddr_in clientAddr = {};
            int nAddrLen = sizeof(sockaddr_in);
            //int addrlen = sizeof(clientAddr);
            SOCKET _cSock = INVALID_SOCKET;
#ifdef _WIN32
            _cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
            _cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t*)&nAddrLen);
            //����vs2019��Xcode��Դ�ļ��Ķ��岻ͬ
#endif
            if (INVALID_SOCKET == _cSock)
            {
                printf("����,���յ���Ч�ͻ���\n");
            }
            else
            {
                for (int n = (int)g_clients.size() - 1; n >= 0; n--)
                {
                    NewUserJoin userJoin;
                    send(g_clients[n], (const char*)&userJoin, sizeof(NewUserJoin), 0);
                }
                g_clients.push_back(_cSock);
                printf("�¿ͻ��˼��� socket: %d ip��%s \n", (int)_cSock, inet_ntoa(clientAddr.sin_addr));
            }
        }

        for (int n = (int)g_clients.size() - 1; n >= 0; n--)
        {
            if (FD_ISSET(g_clients[n], &fdRead))
            {
                if (processor(g_clients[n]) == -1)
                {
                    auto iter = g_clients.begin() + n;
                    //C++11���ϵ�
                    //std::vector<SOCKET>::iterator:: iter = g_clients.begin();
                    //C++11���µİ汾д��
                    if (iter != g_clients.end())
                    {
                        g_clients.erase(iter);
                    }
                }
            }
        }
        //�������
        //Sleep(100);
        //printf("����ʱ�䴦��ҵ��\n");
    }
#ifdef _WIN32
    for (int n = (int)g_clients.size() - 1; n >= 0; n--)
    {
        //�ر��׽���closesocket
        closesocket(g_clients[n]);
    }
    //���Windowssocket����
    WSACleanup();
#else
    for (int n = (int)g_clients.size() - 1; n >= 0; n--)
    {
        //�ر��׽���closesocket
        close(g_clients[n]);
    }
    close(_sock);
#endif
    printf("���˳����������\n");
    getchar();
    return 0;
}
