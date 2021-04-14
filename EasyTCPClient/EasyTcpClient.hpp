#ifndef _EasyTcpClient_hpp_
#define _EasyTcpClient_hpp_
#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
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
#include"MessageHeader.hpp"
class EasyTcpClient
{
	SOCKET _sock;
public:
	EasyTcpClient()
	{
		_sock = INVALID_SOCKET;
	}
	//虚析构
	virtual ~EasyTcpClient()
	{
		Close();
	}

	void initSocket()//初始化socket
	{
		//启动Socket
#ifdef _WIN32
//启动 socket 环境
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif
		if (INVALID_SOCKET != _sock)
		{
			printf("<socket = %d>关闭旧连接...\n", _sock);
			Close();
		}
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock)
		{
			printf("错误，建立Socket失败...\n");
		}
		else {
			printf("建立Socket成功...\n");
		}

	}

	int Connect(const char* ip, unsigned short port)//连接服务器
	{
		if (INVALID_SOCKET == _sock)
		{
			initSocket();
		}
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);
#ifdef _WIN32
		_sin.sin_addr.S_un.S_addr = inet_addr(ip);
		//实际地址以测试主机为主
		//win IP:192.168.56.129
		//macos IP:192.168.56.1
		//乌班图 IP:192.168.56.134
#else
		_sin.sin_addr.s_addr = inet_addr(ip);
		//win IP:192.168.56.135
		//macos IP:192.168.56.1
		//乌班图 IP:192.168.56.134
#endif
		int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
		if (SOCKET_ERROR == ret)
		{
			printf("错误，连接服务器失败...\n");
		}
		else {
			printf("连接服务器成功...\n");
		}
		return ret;
	}

	void Close()//关闭socket
	{
		if (_sock != INVALID_SOCKET)
		{
#ifdef _WIN32
			closesocket(_sock);
			//清除Windows socket环境
			WSACleanup();
#else
			//Linux、Unix用
			close(_sock);
#endif
			_sock == INVALID_SOCKET;
		}
	}

	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}

	int SendData(DataHeader* header)
	{
		if (isRun() && header)
		{
			return send(_sock, (const char*)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}

	int RecvData(SOCKET _cSock)
	{
		//缓冲区
		char szRecv[4096] = {};
		//接收客户端数据
		int nLen = (int)recv(_cSock, szRecv, sizeof(DataHeader), 0);
		//recv强转int是因为在Linux和macos下底层实现不同它们实现的类型是 long
		DataHeader* header = (DataHeader*)szRecv;
		if (nLen <= 0)
		{
			printf("与服务器断开连接，任务结束。\n");
			return -1;
		}
		recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);

		OnNetMag(header);
		return 0;
	}

	void OnNetMag(DataHeader* header)
	{
		switch (header->cmd)
		{
		case CMD_LOGIN_RESULT:
		{

			LoginResult* login = (LoginResult*)header;
			printf("收到服务端消息：CMD_LOGIN_RESULT,数据长度：%d\n", login->dataLength);
		}
		break;
		case CMD_LOGOUT_RESULT:
		{

			LogoutResult* logout = (LogoutResult*)header;
			printf("收到服务端消息：CMD_LOGOUT_RESULT,数据长度：%d\n", logout->dataLength);
		}
		break;
		case CMD_NEW_USER_JOIN:
		{

			NewUserJoin* userJoin = (NewUserJoin*)header;
			printf("收到服务端消息：CMD_NEW_USER_JOIN,数据长度：%d\n", userJoin->dataLength);
		}
		break;
		}
	}

	bool OnRun()
	{
		if (isRun())
		{
			fd_set fdReads;
			FD_ZERO(&fdReads);
			FD_SET(_sock, &fdReads);
			timeval t = { 1,0 };
			int ret = select(_sock + 1, &fdReads, 0, 0, &t);
			if (ret < 0)
			{
				printf("<socket = %d>select任务结束1\n", _sock);
				return false;
			}
			if (FD_ISSET(_sock, &fdReads))
			{
				FD_CLR(_sock, &fdReads);

				if (-1 == RecvData(_sock))
				{
					printf("<socket = %d>select任务结束2\n", _sock);
					return false;
				}
				return true;
			}
			return false;

		}

	}
};
#endif