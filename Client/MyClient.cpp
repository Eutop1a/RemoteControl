#include "pch.h"
#include "MyClient.h"
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

//SOCKET MySocket::GetSock() {
//	return this->sock;
//}

bool MySocket::SocketInit(HANDLE Mutex) {

	// 初始化成员变量
	this->mutex = Mutex;
	this->pRecv = NULL;
	this->pSend = NULL;

	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		this->sock = INVALID_SOCKET;
		return false;
	}

	// 创建空的socket
	this->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (this->sock == INVALID_SOCKET) {
		WSACleanup();
		return false;
	}

	return true;
}

MySocket* MySocket::New() {
	MySocket* Copy = new MySocket;

	Copy->mutex = this->mutex;
	Copy->pRecv = NULL;
	Copy->pSend = NULL;
	Copy->sock = this->sock;

	return Copy;
}

void MySocket::FreeCopy() {
	this->sock = INVALID_SOCKET;
	delete this->pRecv;
	delete this->pSend;
	this->pRecv = NULL;
	this->pSend = NULL;
}

void MySocket::Free() {
	WaitForSingleObject(this->mutex, INFINITE);
	if (this->sock != INVALID_SOCKET) {
		// 关闭socket上面的连接
		shutdown(this->sock, SD_BOTH);
		closesocket(this->sock);
		WSACleanup();
		this->sock = INVALID_SOCKET;
		delete this->pRecv;
		delete this->pSend;
		this->pRecv = NULL;
		this->pSend = NULL;
	}
	ReleaseMutex(this->mutex);
}

bool MySocket::Connect(const char* ip, u_short port) {
	if (this->sock == INVALID_SOCKET) {
		if (!this->SocketInit(this->mutex)) {
			return false;
		}
	}
	// 绑定
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);//把主机字节顺序转换为网络字节顺序
	addr.sin_addr.S_un.S_addr = inet_addr(ip);

	if (connect(this->sock, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
		return false;
	}

	return true;
}

void MySocket::TcharToChar(const TCHAR* tchar, char* _char) {
	int iLength;
	//获取字节长度   
	iLength = WideCharToMultiByte(CP_ACP, 0, tchar, -1, NULL, 0, NULL, NULL);
	//将tchar值赋给_char    
	WideCharToMultiByte(CP_ACP, 0, tchar, -1, _char, iLength, NULL, NULL);
}

PCMsg* MySocket::GetPCMsg() {
	PCMsg* msg = new PCMsg;
	DWORD NameLen = PCMSGSIZE;
	//TCHAR UName[PCMSGSIZE] = { 0 };
	// 获得计算机名
	if (gethostname(msg->PCName, sizeof(msg->PCName)) == SOCKET_ERROR) {
		wprintf(L"Get hostname Error\n");
		return NULL;
	}

	struct hostent* p = gethostbyname(msg->PCName);
	if (p != NULL) {
		for (int i = 0; p->h_addr_list[i] != 0; i++) {
			struct in_addr in;
			memcpy(&in, p->h_addr_list[i], sizeof(struct in_addr));
			strcpy(msg->IP, inet_ntoa(in));
		}
	}
	else {
		wprintf(L"Get IP Error\n");
		return NULL;
	}
	TCHAR UName[PCMSGSIZE] = { 0 };

	if (!GetUserName(UName, &NameLen)) {
		wprintf(L"Get username Error\n");
		return NULL;
	}
	TcharToChar(UName, msg->UserName);

	return msg;
}

bool MySocket::SendMsg(PCMsg* msg) {
	//WaitForSingleObject(this->mutex, INFINITE);
	if (send(this->sock, msg->PCName, PCMSGSIZE, 0) <= 0) {
		wprintf(L"Send Msg Error %d\n", WSAGetLastError());
		return false;
	}

	if (send(this->sock, msg->UserName, PCMSGSIZE, 0) <= 0) {
		wprintf(L"Send Msg Error %d\n", WSAGetLastError());
		return false;
	}

	if (send(this->sock, msg->IP, PCMSGSIZE, 0) <= 0) {
		wprintf(L"Send Msg Error %d\n", WSAGetLastError());
		return false;
	}
	//WaitForSingleObject(this->mutex, INFINITE);
	return true;
}

bool MySocket::MySendHeartBeat() {
	if (this->pSend == NULL) {
		this->pSend = new struct HEARTBEAT;
		// 创建一个空结构体
		struct HEARTBEAT packet = { PacketMagic };
		// 写入发送缓冲区
		*(struct HEARTBEAT*)this->pSend = packet;
		int sendLen = sizeof(struct HEARTBEAT);

		WaitForSingleObject(this->mutex, INFINITE);
		// 发送
		if (send(this->sock, (const char*)this->pSend, sendLen, 0) <= 0) {
			wprintf(L"Send HearBeat error %d\n", WSAGetLastError());
			return false;
		}
		ReleaseMutex(this->mutex);
		delete this->pSend;
		this->pSend = NULL;
	}

	return true;
}

bool MySocket::RecvCommand(wchar_t* Command, char* buf) {
	// 接收比特流
	memset(buf, 0, CMDSIZE);
	int ret = 0;
	WaitForSingleObject(this->mutex, INFINITE);
	if ((ret = recv(this->sock, buf, CMDSIZE, 0)) <= 0) {
		wprintf(L"Recv cmd error %d\n", WSAGetLastError());
		return false;
	}
	ReleaseMutex(this->mutex);
	buf[ret] = '\0';
	int messageLength = MultiByteToWideChar(CP_ACP, 0, buf, ret, NULL, 0);
	//wchar_t* message = new wchar_t[messageLength];
	MultiByteToWideChar(CP_ACP, 0, buf, ret, Command, messageLength);
	//size_t dataLen = ret / sizeof(wchar_t);
	//memcpy(Command, (wchar_t*)buf, dataLen * sizeof(wchar_t));
	return true;
}

bool MySocket::RecvIns(UINT32* command) {
	char* buf = new char[sizeof(UINT32)];
	memset(buf, 0, sizeof(UINT32));
	// 接收比特流
	int ret = 0;
	WaitForSingleObject(this->mutex, INFINITE);
	if ((ret = recv(this->sock, buf, sizeof(UINT32), 0)) <= 0) {
		wprintf(L"Recv cmd error %d\n", WSAGetLastError());
		return false;
	}
	//*command = *reinterpret_cast<const UINT32*>(buf);
	*command = atoi(buf);
	delete[] buf;
	ReleaseMutex(this->mutex);
	return true;
}

bool MySocket::SendResult(wchar_t* res, char* buf) {
	// 清空buf字符串
	memset(buf, 0, CMDSIZE);
	int ret = 0;
	size_t dataLen = wcslen(res);
	int bufferSize = WideCharToMultiByte(CP_ACP, 0, res, dataLen, NULL, 0, NULL, NULL);
	size_t byteLen = sizeof(wchar_t) * dataLen;
	WideCharToMultiByte(CP_ACP, 0, res, dataLen, buf, bufferSize, NULL, NULL);
	//memcpy(buf, res, byteLen);
	//ret = send(this->sock, buf, byteLen, 0);
	WaitForSingleObject(this->mutex, INFINITE);
	ret = send(this->sock, buf, byteLen, 0);
	if (ret <= 0) {
		return false;
	}
	ReleaseMutex(this->mutex);
	return true;
}

bool MySocket::WriteToFile(char* FileBuf, wchar_t* path) {
	// 先对FileBuf文件进行编码转换
	int wideCharSize = MultiByteToWideChar(CP_UTF8, 0, FileBuf, -1, NULL, 0);

	// 分配宽字符字符串的缓冲区
	wchar_t* wideCharStr = new wchar_t[wideCharSize + 1];
	memset(wideCharStr, 0, wideCharSize + 1);

	// 转换为宽字符字符串
	MultiByteToWideChar(CP_UTF8, 0, FileBuf, -1, wideCharStr, wideCharSize);

	// 然后将wideCharStr转换为ANSI编码的多字节字符串
	int bufferSize = WideCharToMultiByte(CP_ACP, 0, wideCharStr, -1, NULL, 0, NULL, NULL);
	char* ansiBuffer = new char[bufferSize + 1];
	memset(ansiBuffer, 0, bufferSize + 1);
	WideCharToMultiByte(CP_ACP, 0, wideCharStr, -1, ansiBuffer, bufferSize, NULL, NULL);

	// 然后将ansiBuffer写入文件
	// wchar_t Path[] = L"D:\\test.txt";
	HANDLE hFile = CreateFileW(
		path, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL
	);

	if (hFile == INVALID_HANDLE_VALUE) {
		wprintf(L"CreateFile Error %d\n", GetLastError());
		delete[] wideCharStr;
		delete[] ansiBuffer;
		return false;
	}

	DWORD dwBytesWritten;
	if (!WriteFile(hFile, ansiBuffer, strlen(ansiBuffer), &dwBytesWritten, NULL)) {
		wprintf(L"Write File error %d\n", GetLastError());
		CloseHandle(hFile);
		delete[] wideCharStr;
		delete[] ansiBuffer;
		return false;
	}

	CloseHandle(hFile);
	delete[] wideCharStr;
	delete[] ansiBuffer;

	return true;
}

bool MySocket::RecvFile(char* File, int len) {
	WaitForSingleObject(this->mutex, INFINITE);
	int ret = recv(this->sock, File, len, 0);
	if (ret <= 0) {
		return false;
	}
	ReleaseMutex(this->mutex);
	File[ret] = '\0';
	return true;
}

int MySocket::ReadFromFile(wchar_t* Path, UINT32* len, char** FileBuf) {
	// 打开文件
	HANDLE hFile = CreateFile(
		Path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL
	);
	DWORD dwFileSize, dwBytesRead;
	if (hFile == INVALID_HANDLE_VALUE) {
		//wprintf(L"OpenFile error %d\n", GetLastError());
		return 1;
	}
	// 获取文件大小
	dwFileSize = GetFileSize(hFile, NULL);

	*len = dwFileSize;

	//wprintf(L"File size is 0x%X\n", dwFileSize);

	*FileBuf = new char[dwFileSize + 1]; // 添加一个额外的字节用于存储字符串结束符
	ZeroMemory(*FileBuf, dwFileSize + 1);

	if (!ReadFile(hFile, *FileBuf, dwFileSize, &dwBytesRead, NULL)) {
		//wprintf(L"ReadFile error %d\n", GetLastError());
		CloseHandle(hFile);
		return 2;
	}

	CloseHandle(hFile);

	return 0;
}

bool MySocket::SendFile(char* File, int len) {

	WaitForSingleObject(this->mutex, INFINITE);
	int ret = send(this->sock, File, len, 0);
	if (ret <= 0) {
		return false;
	}

	ReleaseMutex(this->mutex);

	return true;
}

bool MySocket::SendCommand(UINT32 command) {
	// UINT32强转为const char*
	//const char* data = reinterpret_cast<const char*>(&command);
	char* data = new char[sizeof(UINT32)];
	ZeroMemory(data, sizeof(UINT32));
	itoa(command, data, 10);
	WaitForSingleObject(this->mutex, INFINITE);
	int ret = send(this->sock, data, strlen(data), 0);

	if (ret <= 0) {
		return false;
	}
	ReleaseMutex(this->mutex);
	delete[] data;

	return true;
}