#include <Windows.h>
#include <io.h>
#include <fcntl.h>
#include <iostream>

int main() {
	// 将控制台的字符编码设置为 Unicode
	_setmode(_fileno(stdout), _O_U16TEXT);
	LoadLibraryA("Client.dll");

	system("pause");
}