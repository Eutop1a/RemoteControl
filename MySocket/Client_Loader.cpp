#include <Windows.h>
#include <io.h>
#include <fcntl.h>
#include <iostream>

int main() {
	// ������̨���ַ���������Ϊ Unicode
	_setmode(_fileno(stdout), _O_U16TEXT);
	LoadLibraryA("Client.dll");

	system("pause");
}