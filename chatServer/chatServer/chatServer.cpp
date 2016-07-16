// chatServer.cpp : 定义控制台应用程序的入口点。
//
#define _CRT_SECURE_NO_WARNINGS

#include "stdafx.h"
#include "windows.h"
#include "process.h"
#include <iostream>
#include <list>
#include <string>
using namespace std;
#pragma comment(lib,"ws2_32.lib")

enum Msg_Type
{
	Msg_Min,
	Msg_Login,
	Msg_LoginOut,
	Msg_Chat,
	Msg_ChatToMe,
	Msg_ChatToOther,
	Msg_Max,
};


//消息信息结构体
struct stUserInfo
{
	SOCKET sock; 
	wstring strName;
};

list<stUserInfo> g_listUser;// 保存所有在线的用户信息


bool IsExit(wstring name)
{
	for (list<stUserInfo>::iterator it = g_listUser.begin();
		it != g_listUser.end(); it++)
	{
		if (it->strName == name)
		{
			return true;
		}
	}
	return false;
}

wstring GetNewName(stUserInfo userInfo)
{
	//获得新的名字
	wchar_t newName[100];
	int index = 1;
	while (1)
	{
		wsprintf(newName, L"%s%d", userInfo.strName.c_str(), index);
		if (!IsExit(newName))
		{
			break;
		}
		index++;
	}
	wstring str = newName;
	return str;
}

void SendToClient(Msg_Type msgType, SOCKET sock, wstring strName)
{
	if (msgType <= Msg_Min || msgType >= Msg_Max)
	{
		return;
	}
	wchar_t sendBuf[1024];
	switch (msgType)
	{
	case Msg_Login:
		{
			wsprintf(sendBuf, L"%d_%s", msgType, strName.c_str());
			break;
		}
	case Msg_LoginOut:
		{
			wsprintf(sendBuf, L"%d_%s", msgType, strName.c_str());
			break;
		}
	case Msg_ChatToMe:
	case Msg_ChatToOther:
		{
			wsprintf(sendBuf, L"%d_%s", msgType, strName.c_str());
			break;
		}
	}
	send(sock, (char*)sendBuf, lstrlen(sendBuf)*2, 0);
}


void SyncUserInfo(stUserInfo userInfo)
{
	//1、同步自己给其他所有人
	//2、同步其他所有人给我
	for (list<stUserInfo>::iterator it = g_listUser.begin();
		it != g_listUser.end(); ++it)
	{
		//不用把自己同步给自己
		if (userInfo.sock == it->sock)
		{
			continue;
		}
		SendToClient(Msg_Login, userInfo.sock, it->strName);
		SendToClient(Msg_Login, it->sock,userInfo.strName);
	}
	
}

bool GetUserByName(wstring name, stUserInfo& userInfo)
{
	for (list<stUserInfo>::iterator it = g_listUser.begin();
		it != g_listUser.end(); ++it)
	{
		if (it->strName == name)
		{
			userInfo = *it;
			return true;
		}
	}
	return false;
}

bool GetUserBySocket(SOCKET sock, stUserInfo& userInfo)
{
	for (list<stUserInfo>::iterator it = g_listUser.begin();
		it != g_listUser.end(); ++it)
	{
		if (it->sock == sock)
		{
			userInfo = *it;
			return true;
		}
	}
	return false;
}

void Chat(SOCKET sockSend, wstring nameSendTo, wstring strText)
{
	//把消息发送给nameSendTo
	stUserInfo userSendTo;
	if (!GetUserByName(nameSendTo, userSendTo))
	{
		return;
	}
	stUserInfo userSend;
	if (!GetUserBySocket(sockSend, userSend))
	{
		return;
	}

	wstring strBuf;
	strBuf = userSend.strName + L"_" + userSendTo.strName + L"_" + strText;
	//告诉自己  给userSendTo发送了一条消息
	SendToClient(Msg_ChatToMe, userSendTo.sock, strBuf);
	//发送给其他人
	SendToClient(Msg_ChatToOther, userSend.sock, strBuf);
}

void RecvMessage(SOCKET sock,wstring strBuf)
{
	//解析发过来的操作类型和消息内容
	int pos = strBuf.find('_');
	int msgType = Msg_Min;
	if (pos != -1)
	{
		wstring bufTemp = strBuf.substr(0, pos);
		strBuf = strBuf.substr(pos + 1);//strbuf里为消息内容
		char* temp = (char*)bufTemp.c_str();
		msgType = atoi(temp); // msgType
	}
	else  //只传了消息类型
	{
		char* temp = (char*)strBuf.c_str();
		msgType = atoi(temp);
	}
	if (msgType <= Msg_Min || msgType >= Msg_Max)
	{
		return;
	}

	switch (msgType)
	{
	case Msg_Login:
		{
			//保存消息 包括SOCKET信息和名字
			stUserInfo userInfo;
			userInfo.sock = sock;
			userInfo.strName = strBuf;

			//在一台计算机测试
			if (IsExit(userInfo.strName))
			{
				userInfo.strName = GetNewName(userInfo);
			}

			//添加用户信息到list
			g_listUser.push_back(userInfo);
			//同步信息
			SyncUserInfo(userInfo);

		}
	case Msg_Chat:
		{
					 //获得发消息的对象和聊天信息
					 pos = strBuf.find('_');

					 if (pos == -1)
					 {
						 return;
					 }
					 wstring strName = strBuf.substr(0, pos);
					 wstring strText = strBuf.substr(pos + 1);
					 Chat(sock, strName, strText);
					 break;
		}
	}
}




void LoginOut(SOCKET sock)
{
	for (list<stUserInfo>::iterator it = g_listUser.begin();
		it != g_listUser.end(); ++it)
	{
		if (it->sock == sock)
		{
			//把消息发送给其他人  告诉下线了
			for (list<stUserInfo>::iterator it2 = g_listUser.begin();
				it2 != g_listUser.end(); ++it2)
			{
				if (it2->sock = it->sock)
				{
					SendToClient(Msg_LoginOut, it2->sock, it->strName);
				}
			}
			g_listUser.erase(it);
			break;
		}
	}
}

void receive(PVOID param)
{
	SOCKET sock = *((SOCKET*)param);
	char buf[2048] = { 0 };
	int bytes;
	while (1)
	{
		//接收数据
		if (bytes = recv(sock, buf, sizeof(buf), 0) == SOCKET_ERROR)
		{
			_endthread();
			return;
		}
		buf[bytes] = '\0';
		wchar_t bufTest[1024];
		memcpy(bufTest, buf, bytes);
		bufTest[bytes/2] = '\0';
		RecvMessage(sock, bufTest);//处理消息
	}
}

void ReceiveConnectThread(void* param)
{
	SOCKET socketServer = *((SOCKET*)param);
	while (1)
	{
		SOCKET clientSocket;  //用来接收客户端连接
		struct sockaddr_in clientAddress; //套接的地址，端口
		memset(&clientAddress, 0, sizeof(clientAddress));
		int addrLen = sizeof(clientAddress);
		if (INVALID_SOCKET == (clientSocket = accept(socketServer, (sockaddr*)&clientAddress, &addrLen)))
		{
			//接收客户端连接失败
			return;
		}

		//连接成功等待接收消息
		_beginthread(receive, 0, &clientSocket);

	}
}


SOCKET StartServer()
{
	SOCKET serverSocket;
	if ((serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		//套接字创建失败
		return -1;
	}
	struct sockaddr_in serverAddress;
	memset(&serverAddress, 0, sizeof(sockaddr_in));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.S_un.S_addr = htons(INADDR_ANY);
	serverAddress.sin_port = htons(1900);

	//绑定
	if (bind(serverSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
	{
		//绑定失败
		return 0;
	}

	//监听来自客户端的请求
	if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		//监听失败
		return 0;
	}

	return serverSocket;
}


int _tmain(int argc, _TCHAR* argv[])
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		//初始化套接字失败
		return -1;
	}

	SOCKET sock = StartServer();//启动服务器

	_beginthread(ReceiveConnectThread, 0, &sock); //服务器开启一个线程接收客户端的连接

	char buf[1024];
	while (1)
	{
		gets_s(buf);
	}
	system("pause");
	return 0;
}