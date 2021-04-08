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
    //缓冲区
    char szRecv[4096] = {};
    //接收客户端请求
    int nLen = (int)recv(_cSock, szRecv, sizeof(DataHeader), 0);
    DataHeader* header = (DataHeader*)szRecv;
    if (nLen <= 0)
    {
        printf("客户端<SOCKET = %d>退出,结束服务 \n", _cSock);
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
        printf("收到客户端<SOCKET = %d>指令 CMD_LOGIN  长度：%d,username=%s,Password=%s\n", _cSock, login->dataLength, login->userName, login->PassWord);
        send(_cSock, (char*)&ret, sizeof(LoginResult), 0);
    }
    break;
    case CMD_LOGINOUT:
    {
        //Loginout loginOut;
        recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
        //pass check password
        Loginout* loginOut = (Loginout*)szRecv;
        printf("收到客户端<SOCKET = %d>指令 CMD_LOGIN  长度：%d,username=%s\n", _cSock, loginOut->dataLength, loginOut->username);
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
    //启动windowsocket环境
    WORD ver = MAKEWORD(2, 2);
    WSADATA dat;
    WSAStartup(ver, &dat);
#endif
    //建立一个Socket
    SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//套接字
    //bind绑定用于接收客户端的网络端口
    sockaddr_in _sin = {};
    _sin.sin_family = AF_INET;
    _sin.sin_port = htons(4567);
#ifdef _WIN32
    _sin.sin_addr.S_un.S_addr = INADDR_ANY;
    //_sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
#else
    _sin.sin_addr.s_addr = INADDR_ANY;
#endif
    //::bind问题出现在macos10.15版本
    if (SOCKET_ERROR == ::bind(_sock, (sockaddr*)&_sin, sizeof(_sin)))
    {
        printf("端口绑定失败\n");
    }
    else
    {
        printf("端口绑定成功\n");
    }
    //listen监听网络端口
    if (SOCKET_ERROR == listen(_sock, 5))
    {
        printf("监听端口绑定失败\n");
    }
    else
    {
        printf("监听端口绑定成功\n");
    }
    while (true)
    {
        //伯克利BSDsocket
        fd_set fdRead;//描述符
        fd_set fdWrite;
        fd_set fdExp;
        //清理集合
        FD_ZERO(&fdRead);
        FD_ZERO(&fdWrite);
        FD_ZERO(&fdExp);
        //将描述符加入集合
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
        //即是所有描述值最大值+1 在Windows中参数可以写成0
        timeval t = { 1,0 };

        int ret = select(maxSock + 1, &fdRead, &fdWrite, &fdExp, &t);

        if (ret < 0)
        {
            printf("select结束服务 \n");
            break;
        }
        //判断描述符是否在集合中
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
            //这是vs2019和Xcode中源文件的定义不同
#endif
            if (INVALID_SOCKET == _cSock)
            {
                printf("错误,接收到无效客户端\n");
            }
            else
            {
                for (int n = (int)g_clients.size() - 1; n >= 0; n--)
                {
                    NewUserJoin userJoin;
                    send(g_clients[n], (const char*)&userJoin, sizeof(NewUserJoin), 0);
                }
                g_clients.push_back(_cSock);
                printf("新客户端加入 socket: %d ip：%s \n", (int)_cSock, inet_ntoa(clientAddr.sin_addr));
            }
        }

        for (int n = (int)g_clients.size() - 1; n >= 0; n--)
        {
            if (FD_ISSET(g_clients[n], &fdRead))
            {
                if (processor(g_clients[n]) == -1)
                {
                    auto iter = g_clients.begin() + n;
                    //C++11以上的
                    //std::vector<SOCKET>::iterator:: iter = g_clients.begin();
                    //C++11以下的版本写法
                    if (iter != g_clients.end())
                    {
                        g_clients.erase(iter);
                    }
                }
            }
        }
        //调试语句
        //Sleep(100);
        //printf("空闲时间处理业务\n");
    }
#ifdef _WIN32
    for (int n = (int)g_clients.size() - 1; n >= 0; n--)
    {
        //关闭套节字closesocket
        closesocket(g_clients[n]);
    }
    //清除Windowssocket环境
    WSACleanup();
#else
    for (int n = (int)g_clients.size() - 1; n >= 0; n--)
    {
        //关闭套节字closesocket
        close(g_clients[n]);
    }
    close(_sock);
#endif
    printf("已退出，任务结束\n");
    getchar();
    return 0;
}
