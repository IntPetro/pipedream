#include "Prober.h"

Prober::Prober(const std::wstring& h, const std::wstring& p, bool b)
{
	host = h;
	path = p;
	https = b;
}

ThingInfo Prober::probe(int parts)
{
	ThingInfo info{};
	HINTERNET hSession = WinHttpOpen(L"Prober/1.0", WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hSession)
		throw std::runtime_error("WinHttpOpen failed!");
	HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), https?INTERNET_DEFAULT_HTTPS_PORT:INTERNET_DEFAULT_HTTP_PORT, 0);
	if (!hConnect)
		throw std::runtime_error("WinHttpConnect Failed!");
	HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, https?WINHTTP_FLAG_SECURE:0);
	if (!hRequest)
		throw std::runtime_error("WinHttpOpenRequest failed!");
	if(!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
		throw std::runtime_error("WinHttpSendRequest failed!");
	if(!WinHttpReceiveResponse(hRequest,nullptr))
		throw std::runtime_error("WinHttpReceiveResponse failed!");


	DWORD size = 0;
	WinHttpQueryHeaders(
		hRequest,WINHTTP_QUERY_CONTENT_LENGTH,WINHTTP_HEADER_NAME_BY_INDEX,nullptr,&size,WINHTTP_NO_HEADER_INDEX);

	std::wstring lenStr(size / sizeof(wchar_t), 0);

	if (!WinHttpQueryHeaders(
		hRequest,WINHTTP_QUERY_CONTENT_LENGTH,WINHTTP_HEADER_NAME_BY_INDEX,lenStr.data(),&size,WINHTTP_NO_HEADER_INDEX))
		throw std::runtime_error("Missing Content-Length");

	info.conLen = std::stoull(lenStr);

	DWORD temp = 0;
	info.supportsRanges = WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_ACCEPT_RANGES, WINHTTP_HEADER_NAME_BY_INDEX, nullptr, &temp, WINHTTP_NO_HEADER_INDEX) || GetLastError() == ERROR_INSUFFICIENT_BUFFER;
	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);

	uint64_t base = info.conLen / parts;
	uint64_t rem = info.conLen % parts;
	uint64_t offset = 0;
	for (int i = 0; i < parts; i++)
	{
		uint64_t partSize = base + (i == parts - 1 ? rem : 0);
		info.chunks.push_back({ offset,offset + partSize - 1 });
		offset = offset + partSize;

	}
	return info;
}
