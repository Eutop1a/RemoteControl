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
	// �������ݻ�����
	void* pRecv;
	// �������ݻ�����
	void* pSend;
	// �����壬��֤���ͺͽ��ղ���ͻ
	HANDLE mutex;

public:
	// ��ʼ��Socket
	bool SocketInit(HANDLE mutex);
	// ����һ��������ͬsocket��mutex���࣬��FreeCopy�ͷ�
	MySocket* New();
	// �ͷŸ��Ƶ���
	void FreeCopy();
	// �ͷ�ԭʼ����
	void Free();
	// ����
	// bool Connect(const char* ip, u_short port);
	// ����
	bool Listen(u_short port);
	// ��������
	bool Accept();
	// ����PCmsg
	bool RecvMsg();
	// ѭ����������
	bool Recv();
	// ���տ��ַ�����
	bool WidthRecv();
	// ����������
	bool MyRecvHeartBeat();
	// ����command
	bool SendCommand(UINT32 command);
	// ����cmdָ��
	bool SendCmd(wchar_t* shell, char* byteData);
	// ����cmd���
	bool RecvCmd(wchar_t* result, char* resbuf);
	// ������ָͨ��
	bool Send(wchar_t* Command);
	// ��ȡ�ļ�
	bool ReadFromFile(wchar_t* Path, UINT32* len, char** FileBuf);
	// �����ļ�
	bool SendFile(char* File, int len);
};

