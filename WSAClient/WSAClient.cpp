#include <iostream>
#include <fstream>
#include "FileNameParser.h"
#include "IPPResolver.h"
#include <sstream>
#include <chrono>
#include <Windows.h>
#include <winhttp.h>
#include <locale>
#include <codecvt>
using namespace std;



string filePathBuilder(string fileName,string usrDirectory)
{
    ostringstream fP;
    fP << usrDirectory;
    fP << fileName;
    return fP.str();
}
string requestBuilder(string fileName,string filePath,IPPResolver ipr)
{
    ifstream in(filePath, ios::binary | ios::ate);
    uint64_t existingSize = 0;
    if (in)
    {
        existingSize = in.tellg();
        in.close();
    }
    ostringstream request;
    request << "GET /" << fileName << " HTTP/1.1\r\n";
    request << "Host: " << ipr.ResolveHost() << "\r\n";
    if (existingSize > 0)
    {
        request << "Range: bytes=" << existingSize << "-\r\n";
    }
    request << "Connection: close\r\n\r\n";


    return request.str();
}

ofstream decideOp(int statusCode,string filePath)
{
    if (statusCode == 206)
    {
        cout << "Partially downloaded file found! Resuming...\n";
        return ofstream(filePath, ios::binary | ios::app);
    }
    else if (statusCode == 200)
    {
        return ofstream(filePath);
    }
    else if(statusCode==416)
    {
        cout << "File download already completed!" << endl;
        return ofstream{};
    }
    else
    {
        cout << "Error returned by server!" << endl;
        return ofstream{};
    }
}
int getStatusCode(string header)
{
    int statusCode = 0;
    istringstream iss(header);
    string http;
    iss >> http >> statusCode;
    return statusCode;
}
std::wstring string_to_wstring(const std::string& str) {
    // Setup converter (UTF-8 string to UTF-16 wstring)
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(str);
}


int main(int argc,char*argv[])
{
    if (argc < 3) {
        std::cerr << "Usage: <url> <output_dir>\n";
        return -1;
    }
    IPPResolver ipr(argv[1]);
    string fileName = fNameParser(argv[1]);
    string usrDirectory = argv[2];
    string filePath = filePathBuilder(fileName,usrDirectory);
    
    

    HINTERNET hSession = WinHttpOpen(L"Test/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession)
        return -1;
    cout << "WinHTTP Started! Connecting..." << endl;
    int port = ipr.ResolvePort();
    HINTERNET hConnect;
    HINTERNET hRequest;
    wstring wHost = string_to_wstring(ipr.ResolveHost());
    string temp = "/" + fileName;
    wstring fName = string_to_wstring(temp);
    if (port == 80) {
        cout << "Detected HTTP!" << endl;
        hConnect = WinHttpConnect(hSession, wHost.c_str(), INTERNET_DEFAULT_HTTP_PORT, 0);
        cout << "Connected!" << endl;
        hRequest = WinHttpOpenRequest(hConnect, L"GET", fName.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
        cout << "Requested!" << endl;
    }

    else if (port == 443) {
        cout << "Detected HTTPS!" << endl;
        hConnect = WinHttpConnect(hSession, wHost.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);\
            cout << "Connected!" << endl;
        hRequest = WinHttpOpenRequest(hConnect, L"GET", fName.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
        cout << "Requested!" << endl;
    }
    else
    {
        throw runtime_error("Port returned by server not supported. Try using only http and https!");
        return -1;
    }
    
    BOOL ok = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    if (ok == FALSE)
        return -1;
    cout << "Request Sent!" << endl;
    ok = WinHttpReceiveResponse(hRequest, NULL);
    cout << "Response Received!" << endl;
    DWORD statusCode = 0;
    DWORD size = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &size, WINHTTP_NO_HEADER_INDEX);
    cout << "Status Code Received!\nStatus Code:"<<statusCode<< endl;
 
    ofstream file = decideOp(statusCode, filePath);
    if (!file.is_open())
    {
        return -1;
    }
    cout << "File OP Decided!" << endl;
    DWORD bytesRead = 0;
    char buffer[8192];
    do {
        ok = WinHttpReadData(hRequest, buffer, sizeof(buffer), &bytesRead);
        if (!ok)
            break;
        if (bytesRead > 0)
            file.write(buffer, bytesRead);
    } while (bytesRead > 0);
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return 0;
    
}
