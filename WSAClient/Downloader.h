#pragma once
#include <Windows.h>
#include <winhttp.h>
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <thread>
#include "Structs.h"
class Downloader
{
private:
	HINTERNET hSession;
	HINTERNET hConnect;
	HINTERNET hRequest;
	HINTERNET hSeshCommon;
	std::atomic<bool> done{ false };
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
	void preallocate_file(uint64_t size);
	void download_chunk(const Chunk& chunk, const std::wstring& host, const std::wstring file, bool https, const std::string& filePath,HINTERNET hConnect);
	void commonSession();
	HINTERNET createConnection();
	void worker(ChunkQueue& queue, const std::wstring& host, const std::wstring path, const std::string& filePath, bool https);
	void dlMon(const std::vector<Chunk>& chunks);
public:
	Downloader(std::wstring fName, std::string fPath, int p, std::wstring host);
	int run();
	void download_multi(ThingInfo info);

};

