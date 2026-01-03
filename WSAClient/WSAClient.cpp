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
#include "Prober.h"
#include "Structs.h"


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
    Prober p(wHost, fName, ipr.ResolvePort() == 443 ? 1 : 0);
    ThingInfo info = p.probe(2);
    if (!info.supportsRanges)
    {
        cout << "Going solo!" << endl;
        cout << info.conLen << endl;
    }
    else
    {
        cout << "Going squad!" << endl;
        cout << info.conLen << endl;
        for (auto& it : info.chunks)
            cout << it.start << " " << it.end << endl;
        d.download_multi(info);
    }

    return 0;
    
}
