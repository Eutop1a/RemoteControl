#pragma once

#define WIN32_LEAN_AND_MEAN 
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <Windows.h>
#include <WinSock2.h>
#include <iostream>

#define PacketMagic 0xdeadbeef
#define MaxCount 2
#define PCMSGSIZE 50
#define CMDSIZE 8192
#define FILEBUFFERLEN 2048
#define MAXFILESIZE (50*1024*1024)//50MB

struct PCMsg {
	char PCName[PCMSGSIZE];
	char UserName[PCMSGSIZE];
	char IP[PCMSGSIZE];
	UINT32 Magic;
};

struct HEARTBEAT {
	UINT32 Magic;
};

class MySocket {
private:
	// Client socket
	SOCKET sClient;
	// Server socket
	SOCKET sock;
	// 接收数据缓冲区
	void* pRecv;
	// 发送数据缓冲区
	void* pSend;
	// 互斥体，保证发送和接收不冲突
	HANDLE mutex;

public:
	// 初始化Socket
	bool SocketInit(HANDLE mutex);
	// 复制一个具有相同socket和mutex的类，用FreeCopy释放
	MySocket* New();
	// 释放复制的类
	void FreeCopy();
	// 释放原始的类
	void Free();
	// 连接
	// bool Connect(const char* ip, u_short port);
	// 监听
	bool Listen(u_short port);
	// 接收连接
	bool Accept();
	// 接收PCmsg
	bool RecvMsg();
	// 循环接受数据
	bool Recv();
	// 接收宽字符数据
	bool WidthRecv();
	// 接收心跳包
	bool MyRecvHeartBeat();
	// 发送command
	bool SendCommand(UINT32 command);
	// 发送cmd指令
	bool SendCmd(wchar_t* shell, char* byteData);
	// 接收cmd结果
	bool RecvCmd(wchar_t* result, char* resbuf);
	// 发送普通指令
	bool Send(wchar_t* Command);
	// 读取文件
	bool ReadFromFile(wchar_t* Path, UINT32* len, char** FileBuf);
	// 发送文件
	bool SendFile(char* File, int len);
};

