#pragma once
#include <Windows.h>
#include <winhttp.h>
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>


class Downloader
{
private:
	HINTERNET hSession;
	HINTERNET hConnect;
	HINTERNET hRequest;
	BOOL ok;
	std::wstring fileName;
	std::string filePath;
	int port;
	std::wstring wHost;
	std::ofstream file;
	DWORD statusCode;
	ULONGLONG totalSize = 0;
	ULONGLONG downloaded = 0;
	ULONGLONG resumeFrom = 0;
	int InitSession();
	int ConnectAndRequest();
	int SendAndReceiveRequest();
	int ReceiveStatus();
	int SetBufferSize();
	int AddRangeHeader();
	std::ofstream decideOp(int statusCode,std::string filePath);
	int DecideOpAndWrite();
	int ReceiveContentLength();
public:
	Downloader(std::wstring fName, std::string fPath, int p, std::wstring host);
	int run();

};

