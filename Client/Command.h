#pragma once
enum Command {
	HELP,
	CMD,
	UPLOAD,
	DOWNLOAD,
	SCREENSHOT,
	SUICIDE
};

enum DownloadFileFlag {
	SUCCESS,
	NOSUCHFILE,
	READERROR
};