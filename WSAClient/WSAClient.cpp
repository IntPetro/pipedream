#include <iostream>
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <fstream>
#include "FileNameParser.h"
#include "IPPResolver.h"
#include <sstream>
#include <chrono>
#include "StartWSA.h"
using namespace std;


struct Endpoint {
    sockaddr_storage addr;
    socklen_t addrLen;
    int family;
    int socktype;
    int protocol;
};

Endpoint resolveAndSelect(IPPResolver& ipr)
{
    addrinfo* result = ipr.HostResolver();
    bool found = false;
    SOCKET testSocket;
    Endpoint ep{};
    for (addrinfo* p = result; p != nullptr; p = p->ai_next)
    {
        testSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (testSocket == INVALID_SOCKET)
        {
            cout << "Sock failed! Continuing!\n";
            continue;
        }
        if (connect(testSocket, p->ai_addr, p->ai_addrlen) == 0)
        {
            cout << "Connection to host successful!" << endl;
            memcpy(&ep.addr, p->ai_addr, p->ai_addrlen);
            ep.addrLen = p->ai_addrlen;
            ep.family = p->ai_family;
            ep.socktype = p->ai_socktype;
            ep.protocol = p->ai_protocol;
            found = true;
            closesocket(testSocket);
            break;
        }
        closesocket(testSocket);
    }
    freeaddrinfo(result);
    if (!found)
    {
        throw runtime_error("No reachable endpoint");
    }
    return ep;
}

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
    else
    {
        cout << "Error returned by Server!" << endl;
        WSACleanup();
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
string getHeader(SOCKET sock)
{
    string header;
    char hBuffer;
    while (recv(sock, &hBuffer, 1, 0) == 1)
    {
        header += hBuffer;
        if (header.size() >= 4 && header.substr(header.size() - 4) == "\r\n\r\n")
            break;
    }
    return header;
}
int main(int argc,char*argv[])
{
    if (argc < 3) {
        std::cerr << "Usage: <url> <output_dir>\n";
        return -1;
    }
    startWSA();
    IPPResolver ipr(argv[1]);
    Endpoint ep = resolveAndSelect(ipr);
    string fileName = fNameParser(argv[1]);
    string usrDirectory = argv[2];
    string filePath = filePathBuilder(fileName,usrDirectory);
    string req = requestBuilder(fileName,filePath,ipr);
    SOCKET clientSocket = socket(ep.family, ep.socktype, ep.protocol);
    connect(clientSocket, (sockaddr*)&ep.addr, ep.addrLen);
    send(clientSocket, req.c_str(), req.size(), 0);
    string header = getHeader(clientSocket);
    int statusCode = getStatusCode(header);
    ofstream file=decideOp(statusCode,filePath);
    if (!file.is_open())
    {
        return -1;
    }


    char buffer[8192];
    while (true)
    {
        int byteRec = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (byteRec > 0)
        {
            file.write(buffer, byteRec);
        }
        else
        {
            cout << "Download complete/terimated!" << endl;
            break;
        }
    }
    WSACleanup();
    return 0;
    
}
