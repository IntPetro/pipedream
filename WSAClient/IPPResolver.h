#ifndef IPPRESOLVER_H
#define IPPRESOLVER_H

#include <string>
#include <ws2tcpip.h>

class IPPResolver
{
private:
	int port;
	std::string url;
public:
	IPPResolver(std::string u);
	int ResolvePort();
	addrinfo* HostResolver();
	std::string ResolveHost();
};

#endif // IPPRESOLVER_H
