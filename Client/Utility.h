#pragma once
#include "MyClient.h"

// 执行接收到的cmd指令
bool PipeCmd(wchar_t* pszCmd, wchar_t* recvbuf, bool wait);
