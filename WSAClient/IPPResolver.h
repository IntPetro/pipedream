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
	std::string ResolveHost();
	std::string ResolvePath();
};

#endif // IPPRESOLVER_H
