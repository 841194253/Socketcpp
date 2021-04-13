//如果需要跨系统运行，请修改目标IP地址
//win10：ipconfig
//macos40.14.6：ifconfig
//乌班图16.04：ip addr/ifconfig
//centos7.6: ip addr
// linux指令
//g++ client.cpp -std=c++11 -pthread -o client
//.client
#include "EasyTcpClient.hpp"
#include <thread>

bool g_bRun = true;

void cmdThread(EasyTcpClient* client)
{
	while (true)
	{
		char cmdBuf[256] = {};
		scanf("%s", cmdBuf);
		if (0 == strcmp(cmdBuf, "exit"))
		{
			client->Close();
			printf("退出cmdThread线程\n");
			break;
		}
		else if (0 == strcmp(cmdBuf, "login"))
		{
			Login login;
			strcpy(login.userName, "LRX");
			strcpy(login.PassWord, "LRX1111");
			client->SendData(&login);
		}
		else if (0 == strcmp(cmdBuf, "logout"))
		{
			Logout logout;
			strcpy(logout.userName, "LRX");
			client->SendData(&logout);
		}
		else {
			printf("不支持的命令。\n");
		}
	}
}

int main()
{
	EasyTcpClient client;
	//client.initSocket();
	client.Connect("127.0.0.1", 4567);
	//启动UI线程
	std::thread t1(cmdThread, &client);
	t1.detach();

	while (client.isRun())
	{
		client.OnRun();
	}
	client.Close();


	printf("已退出。\n");
	getchar();
	return 0;
}