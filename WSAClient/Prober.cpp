#include "Prober.h"
#include "IPPResolver.h"

Prober::Prober(const std::wstring& h, const std::wstring& p, bool b)
{
	host = h;
	path = p;
	https = b;
}

std::string parseContentDisposition(const std::wstring& header)
{
    size_t pos = header.find(L"filename=");
    if (pos != std::wstring::npos)
    {
        pos += 9;
        if (pos < header.length() && header[pos] == L'"')
        {
            pos++;
            size_t endPos = header.find(L'"', pos);
            if (endPos != std::wstring::npos)
            {
                return wstring_to_string(header.substr(pos, endPos - pos));
            }
        }
        else
        {
            size_t endPos = header.find(L';', pos);
            if (endPos == std::wstring::npos)
                endPos = header.length();
            return wstring_to_string(header.substr(pos, endPos - pos));
        }
    }
    pos = header.find(L"filename*=");
    if (pos != std::wstring::npos)
    {
        pos += 11;
        size_t quote_pos = header.find(L"''", pos);
        if (quote_pos != std::wstring::npos)
        {
            pos = quote_pos + 2;
        }
        size_t endPos = header.find(L';', pos);
        if (endPos == std::wstring::npos)
            endPos = header.length();
        return wstring_to_string(header.substr(pos, endPos - pos));
    }
    return "";
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

	DWORD cdSize = 0;
	WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CUSTOM, L"Content-Disposition", nullptr, &cdSize, WINHTTP_NO_HEADER_INDEX);
	if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
		std::wstring cdBuf(cdSize / sizeof(wchar_t), 0);
		if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CUSTOM, L"Content-Disposition", &cdBuf[0], &cdSize, WINHTTP_NO_HEADER_INDEX)) {
			info.serverFileName = parseContentDisposition(cdBuf);
		}
	}

	DWORD dwUrlSize = 0;
	WinHttpQueryOption(hRequest, WINHTTP_OPTION_URL, NULL, &dwUrlSize);
	if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
		std::wstring urlBuf(dwUrlSize / sizeof(wchar_t), 0);
		if (WinHttpQueryOption(hRequest, WINHTTP_OPTION_URL, &urlBuf[0], &dwUrlSize)) {
			std::string finalUrlStr = wstring_to_string(urlBuf);
			IPPResolver ipr(finalUrlStr);
			info.finalHost = string_to_wstring(ipr.ResolveHost());
			info.finalPath = string_to_wstring(ipr.ResolvePath());
			info.finalPort = ipr.ResolvePort();
		}
	}
	if (info.finalHost.empty()) {
		info.finalHost = host;
		info.finalPath = path;
		info.finalPort = https ? 443 : 80;
	}

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
