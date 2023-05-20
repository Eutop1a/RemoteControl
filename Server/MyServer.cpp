#include "MyServer.h"

#pragma comment(lib, "ws2_32.lib")

bool MySocket::SocketInit(CRITICAL_SECTION criticalSection) {
	this->criticalSection = criticalSection;
	this->sClient = NULL;
	// 初始化成员变量
	//this->mutex = Mutex;
	this->pRecv = NULL;
	this->pSend = NULL;

	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		this->sock = INVALID_SOCKET;
		//wprintf(L"WSAStartup error %d\n", WSAGetLastError());
		return false;
	}

	// 创建空的socket
	this->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (this->sock == INVALID_SOCKET) {
		//wprintf(L"Create socket error %d\n", WSAGetLastError());
		WSACleanup();
		return false;
	}

	return true;
}

MySocket* MySocket::New() {
	MySocket* Copy = new MySocket;
	Copy->criticalSection = this->criticalSection;
	//Copy->mutex = this->mutex;
	Copy->pRecv = NULL;
	Copy->pSend = NULL;
	Copy->sock = this->sock;
	Copy->sClient = this->sClient;

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
	DeleteCriticalSection(&this->criticalSection);
	//WaitForSingleObject(this->mutex, INFINITE);
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
	//ReleaseMutex(this->mutex);
}

bool MySocket::Listen(u_short port) {
	if (this->sock == INVALID_SOCKET) {
		if (!this->SocketInit(this->criticalSection)) {
			return false;
		}
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	// 让系统自动获取本机的IP地址
	addr.sin_addr.S_un.S_addr = INADDR_ANY;
	// 绑定	
	if (bind(this->sock, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR) {
		//wprintf(L"Bind error %d\n", WSAGetLastError());
		return false;
	}
	// 开始监听
	if (listen(this->sock, MaxCount) == SOCKET_ERROR) {
		//wprintf(L"Listen error %d\n", WSAGetLastError());
		return false;
	}
	return true;
}

bool MySocket::RecvMsg() {
	PCMsg pcmsg = { 0 };
	//WaitForSingleObject(this->mutex, INFINITE);
	if (recv(this->sClient, pcmsg.PCName, PCMSGSIZE, 0) <= 0) {
		//wprintf(L"Recv Error %d\n", WSAGetLastError());
		return false;
	}
	if (recv(this->sClient, pcmsg.UserName, PCMSGSIZE, 0) <= 0) {
		//wprintf(L"Recv Error %d\n", WSAGetLastError());
		return false;
	}
	if (recv(this->sClient, pcmsg.IP, PCMSGSIZE, 0) <= 0) {
		//wprintf(L"Recv Error %d\n", WSAGetLastError());
		return false;
	}
	//ReleaseMutex(this->mutex);
	printf("Client name is %s, username is %s, ip is %s\n", pcmsg.PCName, pcmsg.UserName, pcmsg.IP);
	return true;
}

bool MySocket::Accept() {
	SOCKET Client;
	sockaddr_in remoteAddr;
	int nAddrLen = sizeof(remoteAddr);
	Client = accept(this->sock, (struct sockaddr*)&remoteAddr, &nAddrLen);

	if (Client == SOCKET_ERROR) {
		//wprintf(L"Accept error %d\n", WSAGetLastError());
		return false;
	}
	this->sClient = Client;

	//printf("Client connected: %s:%d\n", inet_ntoa(remoteAddr.sin_addr), ntohs(remoteAddr.sin_port));

	if (this->sClient == NULL) {
		return false;
	}

	//wprintf(L"Connect Success\n");
	return true;
}

bool MySocket::Recv() {

	//char RecvData[BUFSIZ];
	char* RecvData = new char[CMDSIZE];
	ZeroMemory(RecvData, CMDSIZE);
	while (true) {
		// 接收数据

		//WaitForSingleObject(this->mutex, INFINITE);
		EnterCriticalSection(&this->criticalSection);
		int ret = recv(this->sClient, RecvData, BUFSIZ, 0);
		if (ret > 0) {
			RecvData[ret] = 0;
			std::wcout << RecvData << "\n";
		}
		LeaveCriticalSection(&this->criticalSection);
		//ReleaseMutex(this->mutex);
	}
	delete[] RecvData;
	return true;
}

bool MySocket::MyRecvHeartBeat() {
	char buf[sizeof(struct HEARTBEAT)] = { 0 };
	//WaitForSingleObject(this->mutex, INFINITE);
	EnterCriticalSection(&this->criticalSection);
	if (recv(this->sClient, buf, sizeof(struct HEARTBEAT), 0) <= 0) {
		//wprintf(L"Recv HeartBeat error %d \n", WSAGetLastError());
		LeaveCriticalSection(&this->criticalSection);
		return false;
	}
	//ReleaseMutex(this->mutex);
	LeaveCriticalSection(&this->criticalSection);
	return true;
}

bool MySocket::SendCmd(wchar_t* shell, char* byteData) {
	ZeroMemory(byteData, CMDSIZE);
	// 将wchar_t转换为字节流后发送给服务端
	int ret = 0;
	size_t dataLen = wcslen(shell);
	size_t bufferSize = WideCharToMultiByte(CP_ACP, 0, shell, dataLen, NULL, 0, NULL, NULL);
	size_t byteLen = sizeof(wchar_t) * dataLen;
	WideCharToMultiByte(CP_ACP, 0, shell, byteLen, byteData, bufferSize, NULL, NULL);
	//size_t dataLen = wcslen(shell);
	//size_t byteLen = sizeof(wchar_t) * dataLen;
	////char* byteData = new char[byteLen];
	//memset(byteData, 0, byteLen);
	//memcpy(byteData, shell, byteLen);
	//WaitForSingleObject(this->mutex, INFINITE);
	EnterCriticalSection(&this->criticalSection);
	ret = send(this->sClient, byteData, byteLen, 0);
	if (ret <= 0) {
		//wprintf(L"Send cmd error %d\n", WSAGetLastError());
		LeaveCriticalSection(&this->criticalSection);
		return false;
	}
	LeaveCriticalSection(&this->criticalSection);
	//ReleaseMutex(this->mutex);
	return true;
}

bool MySocket::SendCommand(UINT32 command) {
	// UINT32强转为const char*
	//const char* data = reinterpret_cast<const char*>(&command);
	char* data = new char[sizeof(UINT32)];
	ZeroMemory(data, sizeof(UINT32));
	itoa(command, data, 10);
	EnterCriticalSection(&this->criticalSection);
	//WaitForSingleObject(this->mutex, INFINITE);
	int ret = send(this->sClient, data, strlen(data), 0);

	if (ret <= 0) {
		LeaveCriticalSection(&this->criticalSection);
		return false;
	}
	LeaveCriticalSection(&this->criticalSection);
	//ReleaseMutex(this->mutex);
	delete[] data;

	return true;
}

bool MySocket::RecvCmd(wchar_t* result, char* resbuf) {
	ZeroMemory(resbuf, CMDSIZE);
	int ret = 0;
	EnterCriticalSection(&this->criticalSection);
	//WaitForSingleObject(this->mutex, INFINITE);
	ret = recv(this->sClient, resbuf, CMDSIZE, 0);

	if (ret <= 0) {
		//wprintf(L"Recv cmd error %d\n", WSAGetLastError());
		LeaveCriticalSection(&this->criticalSection);
		return false;
	}
	LeaveCriticalSection(&this->criticalSection);
	//ReleaseMutex(this->mutex);
	resbuf[ret] = '\0';
	int messageLength = MultiByteToWideChar(CP_ACP, 0, resbuf, ret, NULL, 0);
	MultiByteToWideChar(CP_ACP, 0, resbuf, ret, result, messageLength);
	//size_t dataLen = ret / sizeof(wchar_t);

	//memcpy(result, (wchar_t*)resbuf, dataLen * sizeof(wchar_t));

	return true;
}

bool MySocket::Send(wchar_t* Command) {

	size_t len = wcslen(Command);
	char* buf = new char[len * sizeof(wchar_t)];
	ZeroMemory(buf, len * sizeof(wchar_t));
	int bufferSize = WideCharToMultiByte(CP_ACP, 0, Command, len, NULL, 0, NULL, NULL);
	size_t byteLen = sizeof(wchar_t) * len;
	WideCharToMultiByte(CP_ACP, 0, Command, byteLen, buf, bufferSize, NULL, NULL);
	EnterCriticalSection(&this->criticalSection);
	//WaitForSingleObject(this->mutex, INFINITE);
	size_t ret = send(this->sClient, buf, byteLen, 0);
	if (ret <= 0) {
		//wprintf(L"Send cmd error %d\n", WSAGetLastError());
		LeaveCriticalSection(&this->criticalSection);
		return false;
	}
	LeaveCriticalSection(&this->criticalSection);
	//ReleaseMutex(this->mutex);
	delete[] buf;

	return false;
}

bool MySocket::ReadFromFile(wchar_t* Path, UINT32* len, char** FileBuf) {
	// 打开文件
	HANDLE hFile = CreateFile(
		Path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL
	);
	DWORD dwFileSize, dwBytesRead;
	if (hFile == INVALID_HANDLE_VALUE) {
		//wprintf(L"OpenFile error %d\n", GetLastError());
		return false;
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
		return false;
	}

	CloseHandle(hFile);

	return true;
}

bool MySocket::SendFile(char* File, int len) {
	EnterCriticalSection(&this->criticalSection);
	//WaitForSingleObject(this->mutex, INFINITE);
	int ret = send(this->sClient, File, len, 0);
	if (ret <= 0) {
		LeaveCriticalSection(&this->criticalSection);
		return false;
	}
	LeaveCriticalSection(&this->criticalSection);
	//ReleaseMutex(this->mutex);

	return true;
}

bool MySocket::RecvFile(char* File, int len) {
	EnterCriticalSection(&this->criticalSection);
	//WaitForSingleObject(this->mutex, INFINITE);
	int ret = recv(this->sClient, File, len, 0);
	if (ret <= 0) {
		LeaveCriticalSection(&this->criticalSection);
		return false;
	}
	LeaveCriticalSection(&this->criticalSection);
	//ReleaseMutex(this->mutex);
	File[ret] = '\0';
	return true;
}


bool MySocket::WriteToFile(char* FileBuf, wchar_t* path) {
	// 先对FileBuf文件进行编码转换
	int wideCharSize = MultiByteToWideChar(CP_ACP, 0, FileBuf, -1, NULL, 0);

	// 分配宽字符字符串的缓冲区
	wchar_t* wideCharStr = new wchar_t[wideCharSize + 1];
	ZeroMemory(wideCharStr, (wideCharSize + 1) * sizeof(wchar_t));

	// 转换为宽字符字符串
	MultiByteToWideChar(CP_ACP, 0, FileBuf, -1, wideCharStr, wideCharSize);

	// 然后将wideCharStr转换为ANSI编码的多字节字符串
	int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wideCharStr, -1, NULL, 0, NULL, NULL);
	char* ansiBuffer = new char[bufferSize + 1];
	ZeroMemory(ansiBuffer, bufferSize + 1);
	WideCharToMultiByte(CP_UTF8, 0, wideCharStr, -1, ansiBuffer, bufferSize, NULL, NULL);

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

bool MySocket::RecvIns(UINT32* command) {
	char* buf = new char[sizeof(UINT32)];
	ZeroMemory(buf, sizeof(UINT32));
	// 接收比特流
	int ret = 0;
	EnterCriticalSection(&this->criticalSection);
	//WaitForSingleObject(this->mutex, INFINITE);
	if ((ret = recv(this->sClient, buf, sizeof(UINT32), 0)) <= 0) {
		//wprintf(L"Recv cmd error %d\n", WSAGetLastError());
		LeaveCriticalSection(&this->criticalSection);
		return false;
	}
	LeaveCriticalSection(&this->criticalSection);
	//*command = *reinterpret_cast<const UINT32*>(buf);
	*command = atoi(buf);
	delete[] buf;

	//ReleaseMutex(this->mutex);
	return true;
}