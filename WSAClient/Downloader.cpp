#include "Downloader.h"

Downloader::Downloader(std::wstring fName, std::string fPath, int p, std::wstring host) {
	fileName = fName;
	filePath = fPath;
	port = p;
	wHost = host;
}
int Downloader::InitSession()
{
	hSession = WinHttpOpen(L"Test/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hSession)
		return -1;
	else {
		std::cout << "WinHTTP Started! Connecting..." << std::endl;
		return 0;
	}
}
int Downloader::ConnectAndRequest()
{
	if (port == 80) {
		std::cout << "Detected HTTP!" << std::endl;
		hConnect = WinHttpConnect(hSession, wHost.c_str(), INTERNET_DEFAULT_HTTP_PORT, 0);
		std::cout << "Connected!" << std::endl;
		hRequest = WinHttpOpenRequest(hConnect, L"GET", fileName.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
		std::cout << "Requested!" << std::endl;
		return 0;
	}

	else if (port == 443) {
		std::cout << "Detected HTTPS!" << std::endl;
		hConnect = WinHttpConnect(hSession, wHost.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0); \
			std::cout << "Connected!" << std::endl;
		hRequest = WinHttpOpenRequest(hConnect, L"GET", fileName.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
		std::cout << "Requested!" << std::endl;
		return 0;
	}
	else
	{
		throw std::runtime_error("Port returned by server not supported. Try using only http and https!");
		return -1;
	}
}
int Downloader::SetBufferSize()
{
	DWORD recvSize = 256*1024;
	WinHttpSetOption(hRequest, WINHTTP_OPTION_READ_BUFFER_SIZE, &recvSize, sizeof(recvSize));
	return 0;
}
int Downloader::AddRangeHeader()
{
	std::string str = std::to_string(resumeFrom);
	int size_needed = MultiByteToWideChar(
		CP_UTF8,
		MB_ERR_INVALID_CHARS,
		str.data(),
		(int)str.size(),
		nullptr,
		0
	);
	if (size_needed == 0)
		throw std::runtime_error("MultiByteToWideChar failed");
	std::wstring wstr(size_needed, 0);
	int result = MultiByteToWideChar(
		CP_UTF8,
		MB_ERR_INVALID_CHARS,
		str.data(),
		(int)str.size(),
		&wstr[0],
		size_needed
	);

	if (result == 0)
		throw std::runtime_error("MultiByteToWideChar failed");
	std::wstring rangeHeader = L"Range: bytes=" + wstr + L"-\r\n";
	BOOL ok = WinHttpAddRequestHeaders(
			hRequest,
			rangeHeader.c_str(),
			(DWORD)-1,
			WINHTTP_ADDREQ_FLAG_ADD
		);

	if (!ok)
		throw std::runtime_error("Failed to add Range header");
	return 0;
}
int Downloader::SendAndReceiveRequest()
{
	ok = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
	if (ok == FALSE)
		return -1;
	std::cout << "Request Sent!" << std::endl;
	ok = WinHttpReceiveResponse(hRequest, NULL);
	std::cout << "Response Received!" << std::endl;
	return 0;
}
int Downloader::ReceiveStatus()
{
	statusCode = 0;
	DWORD size = sizeof(statusCode);
	WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &size, WINHTTP_NO_HEADER_INDEX);
	std::cout << "Status Code Received!\nStatus Code:" << statusCode << std::endl;
	return 0;
}
std::ofstream Downloader::decideOp(int statusCode, std::string filePath)
{
	if (statusCode == 206&&resumeFrom>0)
	{
		std::cout << "Partially downloaded file found! Resuming...\n";
		downloaded = resumeFrom;
		return std::ofstream(filePath, std::ios::binary | std::ios::app);
	}
	else if (statusCode == 200)
	{
		return std::ofstream(filePath,std::ios::binary|std::ios::trunc);
	}
	else if (statusCode == 416)
	{
		std::cout << "File download already completed!" << std::endl;
		return std::ofstream{};
	}
	else
	{
		std::cout << "Error returned by server!" << std::endl;
		return std::ofstream{};
	}
}
int Downloader::ReceiveContentLength()
{
	DWORD size = sizeof(totalSize);
	BOOL ok = WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &totalSize, &size, WINHTTP_NO_HEADER_INDEX);
	if (!ok)
	{
		std::cout << "Couldn't get Con-len(chunked?)" << std::endl;
		totalSize = 0;
		return -1;
	}
	std::cout << "Total size:" << totalSize << " bytes" << std::endl;
}
int Downloader::DecideOpAndWrite()
{
	file = decideOp(statusCode, filePath);
	if (!file.is_open())
	{
		return -1;
	}
	std::cout << "File OP Decided!" << std::endl;
	file.rdbuf()->pubsetbuf(nullptr, 1 << 20);
	DWORD bytesRead = 0;
	static constexpr size_t BUF_SIZE = 256 * 1024;
	std::vector<char> buffer(BUF_SIZE);
	do {
		ok = WinHttpReadData(hRequest, buffer.data(), buffer.size(), &bytesRead);
		if (!ok)
			break;
		if (bytesRead > 0) {
			file.write(buffer.data(), bytesRead);
			downloaded += bytesRead;
			if (totalSize > 0)
			{
				double percent = (double)downloaded * 100.0 / totalSize;
				static auto last = std::chrono::steady_clock::now();

				auto now = std::chrono::steady_clock::now();
				if (now - last > std::chrono::milliseconds(200)) {
					std::cout << "\rProgress: " << (int)percent << "%" << std::flush;
					last = now;
				}
			}
			else
			{
				std::cout << "\rDownloaded:" << downloaded / 1024 << " KB" << std::flush;
			}
		}
	} while (bytesRead > 0);

	return 0;
}


void Downloader::preallocate_file(uint64_t size)
{
	std::ofstream out(filePath, std::ios::binary | std::ios::trunc);
	if (!out)
		throw std::runtime_error("Failed to create o/p file!");
	out.seekp(size - 1);
	out.write("", 1);
	out.close();
}
void Downloader::commonSession()
{
	hSeshCommon = WinHttpOpen(L"CommonSesh/1.0", WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hSeshCommon)
		throw std::runtime_error("WinHttpOpen Failed");

	WinHttpSetTimeouts(hSeshCommon, 5000, 5000, 5000, 30000);

}
HINTERNET Downloader::createConnection()
{
	HINTERNET hConnect = WinHttpConnect(hSeshCommon, wHost.c_str(), port==443 ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, 0);
	if (!hConnect)
		throw std::runtime_error("WinHttpConnect Failed!");
	return hConnect;
}
void Downloader::download_chunk(const Chunk& chunk, const std::wstring& host, const std::wstring file, bool https, const std::string& filePath,HINTERNET hConnect)
{
	HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", file.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, https ? WINHTTP_FLAG_SECURE : 0);
	if (!hRequest)
		throw std::runtime_error("WinHttpOpenRequest failed!");

	DWORD bufferSize = 1024 * 512;
	WinHttpSetOption(hRequest, WINHTTP_OPTION_READ_BUFFER_SIZE, &bufferSize, sizeof(bufferSize));

	std::wstring range = L"Range: bytes=" + string_to_wstring(std::to_string(chunk.start)) + L"-" + string_to_wstring(std::to_string(chunk.end)) + L"\r\n";
	WinHttpAddRequestHeaders(hRequest, range.c_str(), -1, WINHTTP_ADDREQ_FLAG_ADD);
	if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
		throw std::runtime_error("Send failed!");
	if (!WinHttpReceiveResponse(hRequest, nullptr))
		throw std::runtime_error("Receive failed!");

	DWORD status = 0;
	DWORD size = sizeof(status);
	WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX);
	if (status != 206)
		throw std::runtime_error("Server fraudass");
	std::ofstream out(filePath, std::ios::binary | std::ios::in | std::ios::out);
	out.seekp(chunk.start);
	out.rdbuf()->pubsetbuf(nullptr, 0);
	constexpr DWORD BUF_SIZE = 512 * 1024;
	std::vector<char> buffer(BUF_SIZE);
	DWORD bytesRead = 0;
	while (WinHttpReadData(hRequest, buffer.data(), BUF_SIZE, &bytesRead) && bytesRead > 0) {
		out.write(buffer.data(), bytesRead);
		chunk.stats->bytes.fetch_add(bytesRead, std::memory_order_relaxed);
	}
	WinHttpCloseHandle(hRequest);
	
}
void Downloader::worker(ChunkQueue& queue, const std::wstring& host, const std::wstring path, const std::string& filePath, bool https)
{
	HINTERNET hConnect = createConnection();
	std::fstream file(filePath, std::ios::binary | std::ios::in | std::ios::out);
	Chunk c;
	while (queue.pop(c))
	{
		download_chunk(c, host, path, https, filePath, hConnect);
	}
	WinHttpCloseHandle(hConnect);
}
void Downloader::dlMon(const std::vector<Chunk>& chunks) {
	using clock = std::chrono::steady_clock;
	auto lastTime = clock::now();
	uint64_t last = 0;
	while (!done.load(std::memory_order_relaxed))
	{
	    std::this_thread::sleep_for(std::chrono::milliseconds(500));
		uint64_t currBytes = 0;
		for (const auto& c : chunks)
		{
			currBytes += c.stats->bytes.load(std::memory_order_relaxed);
		}
		auto now = clock::now();
		double dur = std::chrono::duration<double>(now - lastTime).count();
		uint64_t del = currBytes - last;
		double mbps = (del * 8.0) / (dur * 1e6);
		std::cout << "\r" << mbps << " Mbps    " << std::flush;
		last = currBytes;
		lastTime = now;
	}
}
void Downloader::download_multi(ThingInfo info)
{
	preallocate_file(info.conLen);
	std::vector<ChunkStats> stats(info.chunks.size());
	for (size_t i = 0; i < info.chunks.size(); i++)
	{
		info.chunks[i].stats = &stats[i];
	}
	ChunkQueue queue;
	for (const auto& c : info.chunks)
	{
		queue.push(c);
	}
	done.store(false);
	std::thread mon(&Downloader::dlMon,this,std::cref(info.chunks));
	commonSession();
	constexpr int WORKERS = 8;
	std::vector<std::thread> threads;
	for (int i = 0; i < WORKERS; i++) {
		threads.emplace_back(&Downloader::worker,this,std::ref(queue),wHost,fileName,filePath,port == 443);

		std::this_thread::sleep_for(std::chrono::milliseconds(250));
	}

	for (auto& t : threads)
		t.join();

	done.store(true);
	mon.join();
}

int Downloader::run()
{
	if (std::filesystem::exists(filePath))
		resumeFrom = std::filesystem::file_size(filePath);
	InitSession();
	ConnectAndRequest();
	if (resumeFrom > 0)
		AddRangeHeader();
	SetBufferSize();
	SendAndReceiveRequest();
	ReceiveStatus();
	ReceiveContentLength();
	if (statusCode == 206)
		totalSize += resumeFrom;
	DecideOpAndWrite();
	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);
	return 0;
}
