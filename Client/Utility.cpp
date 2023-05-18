#include "pch.h"
#include "Utility.h"
#include <iostream>

 bool PipeCmd(wchar_t* pszCmd, wchar_t* recvbuf, bool wait) {
	memset(recvbuf, 0, CMDSIZE);
	HANDLE hReadPipe = NULL;
	HANDLE hWritePipe = NULL;
	SECURITY_ATTRIBUTES securityAttributes = { 0 };
	BOOL bRet = FALSE;
	// ���ð�ȫ����
	// �½��̽��̳о��
	securityAttributes.bInheritHandle = TRUE;
	securityAttributes.lpSecurityDescriptor = NULL;
	securityAttributes.nLength = sizeof(securityAttributes);

	// ���������ܵ�
	bRet = CreatePipe(&hReadPipe, &hWritePipe, &securityAttributes, 0);
	if (bRet == FALSE) {
		printf("Create pipe failed %d\n", GetLastError());
		return false;
	}

	STARTUPINFOW si = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	// �����½��̲���
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	si.hStdError = hWritePipe;
	si.hStdOutput = hWritePipe;
	// �����½���ִ�������ִ�н��д�������ܵ���
	bRet = CreateProcessW(
		NULL, pszCmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi
	);

	if (FALSE == bRet) {
		return false;
	}

	if (wait) {
		// �ȴ�����ִ�н���
		WaitForSingleObject(pi.hThread, INFINITE);
		WaitForSingleObject(pi.hProcess, INFINITE);
		// �������ܵ��ж�ȡ��������������
		DWORD size;
		if (PeekNamedPipe(hReadPipe, NULL, NULL, NULL, &size, NULL)) {
			if (size > 0) {
				void* utf8buf = HeapAlloc(GetProcessHeap(), 0, (size_t)(CMDSIZE));
				if (utf8buf == NULL) {
					printf("HeapAlloc failed %d\n", GetLastError());
					return false;
				}
				memset(utf8buf, 0, CMDSIZE);
				if (ReadFile(hReadPipe, utf8buf, size, NULL, NULL) == FALSE) {
					printf("ReadFile Error %d\n", GetLastError());
					HeapFree(GetProcessHeap(), 0, (void*)(utf8buf));
					return false;
				}
				int len = MultiByteToWideChar(CP_ACP, 0, (char*)utf8buf, -1, NULL, 0);
				if (MultiByteToWideChar(CP_ACP, 0, (char*)utf8buf, -1, recvbuf, len) == 0) {
					return false;
				}
				/*int wchars_num = MultiByteToWideChar(CP_UTF8, 0, utf8buf, -1, NULL, 0);
				MultiByteToWideChar(CP_UTF8, 0, utf8buf, -1, recvbuf, wchars_num);*/
				// ������Խ��

				wprintf(L"%s", recvbuf);
				HeapFree(GetProcessHeap(), 0, (void*)(utf8buf));
			}
		}

	}

	// �رվ��
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	CloseHandle(hWritePipe);
	CloseHandle(hReadPipe);
	return true;
}
