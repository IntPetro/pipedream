#include "IPPResolver.h"
#include <ws2tcpip.h>
#include <cstring>

IPPResolver::IPPResolver(std::string u)
{
    url = u;
}

int IPPResolver::ResolvePort()
{
    int protocol_end = url.find("://");
    if (protocol_end == std::string::npos)
    {
        return -1;
    }
    int host_start = (protocol_end == std::string::npos) ? 0 : protocol_end + 3;
    int port_start = url.find(':', host_start);
    if (port_start != std::string::npos)
    {
        int port_end = url.find('/', port_start);
        if (port_end == std::string::npos)
        {
            port_end = url.length();
        }
        std::string port_str = url.substr(port_start + 1, port_end - port_start - 1);
        try
        {
            return std::stoi(port_str);
        }
        catch (...)
        {
            return -1;
        }
    }

    if (url.substr(0, protocol_end) == "https")
        return 443;
    else if (url.substr(0, protocol_end) == "http")
        return 80;
    else
        return -1;
}

std::string IPPResolver::ResolveHost()
{
    int protocol_end = url.find("://");
    if (protocol_end == std::string::npos)
        return "";

    int host_start = protocol_end + 3;
    int host_end = url.find('/', host_start);

    if (host_end == std::string::npos)
        host_end = url.length();
    int colon_pos = url.find(':', host_start);
    if (colon_pos != std::string::npos && colon_pos < host_end)
        host_end = colon_pos;

    return url.substr(host_start, host_end - host_start);
}

addrinfo* IPPResolver::HostResolver()
{
    addrinfo hint;
    addrinfo* result;
    memset(&hint, 0, sizeof(hint));
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_family = AF_UNSPEC;

    std::string host = ResolveHost();
    std::string port = std::to_string(ResolvePort());

    int status = getaddrinfo(host.c_str(), port.c_str(), &hint, &result);
    if (status)
        return NULL;
    else
        return result;
}