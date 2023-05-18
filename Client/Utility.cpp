#include "pch.h"
#include "Utility.h"
#include <iostream>

 bool PipeCmd(wchar_t* pszCmd, wchar_t* recvbuf, bool wait) {
	memset(recvbuf, 0, CMDSIZE);
	HANDLE hReadPipe = NULL;
	HANDLE hWritePipe = NULL;
	SECURITY_ATTRIBUTES securityAttributes = { 0 };
	BOOL bRet = FALSE;
	// 设置安全属性
	// 新进程将继承句柄
	securityAttributes.bInheritHandle = TRUE;
	securityAttributes.lpSecurityDescriptor = NULL;
	securityAttributes.nLength = sizeof(securityAttributes);

	// 创建匿名管道
	bRet = CreatePipe(&hReadPipe, &hWritePipe, &securityAttributes, 0);
	if (bRet == FALSE) {
		printf("Create pipe failed %d\n", GetLastError());
		return false;
	}

	STARTUPINFOW si = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	// 设置新进程参数
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	si.hStdError = hWritePipe;
	si.hStdOutput = hWritePipe;
	// 创建新进程执行命令，将执行结果写入匿名管道中
	bRet = CreateProcessW(
		NULL, pszCmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi
	);

	if (FALSE == bRet) {
		return false;
	}

	if (wait) {
		// 等待命令执行结束
		WaitForSingleObject(pi.hThread, INFINITE);
		WaitForSingleObject(pi.hProcess, INFINITE);
		// 从匿名管道中读取结果到输出缓冲区
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
				// 输出测试结果

				wprintf(L"%s", recvbuf);
				HeapFree(GetProcessHeap(), 0, (void*)(utf8buf));
			}
		}

	}

	// 关闭句柄
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	CloseHandle(hWritePipe);
	CloseHandle(hReadPipe);
	return true;
}
