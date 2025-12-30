#include "StartWSA.h"

void startWSA()
{
    WSADATA wsaData;
    int wsaerr;
    WORD wVersion = MAKEWORD(2, 2);
    wsaerr = WSAStartup(wVersion, &wsaData);
    if (wsaerr != 0)
    {
        std::cout << "WSA Startup Failed!" << std::endl;
        WSACleanup();
    }
    else
    {
        std::cout << "WSA Startup OK!" << std::endl;
    }
}