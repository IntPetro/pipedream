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
#include "Downloader.h"


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


std::wstring string_to_wstring(const std::string& str)
{
    if (str.empty())
        return L"";
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

    return wstr;
}


int main(int argc,char*argv[])
{
    if (argc < 3) {
        std::cerr << "Usage: <url> <output_dir>\n";
        return -1;
    }
    srand(time(0));
    cout << "EL. PSY. KONGROO." << endl;
    cout <<fixed<<setprecision(6)<<"Divergence Number:" <<((double)rand())/RAND_MAX<< endl;
    IPPResolver ipr(argv[1]);
    string fileName = fNameParser(argv[1]);
    string usrDirectory = argv[2];
    string filePath = filePathBuilder(fileName,usrDirectory);
    wstring wHost = string_to_wstring(ipr.ResolveHost());
    string temp = "/" + fileName;
    wstring fName = string_to_wstring(temp);

    Downloader d(fName, filePath, ipr.ResolvePort(), wHost);
    d.run();
    return 0;
    
}
