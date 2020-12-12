#pragma once
#include <algorithm>
#include <string>
#include <map>
#include <stdexcept>
#include <memory>
#include <tuple>
#include "string.hpp"
#ifdef _MSC_VER
#include <WinSock2.h>
#endif

namespace chesham
{
    namespace cppext
    {
        namespace net
        {
            struct uri
            {
                enum class schemes
                {
                    UNKNOWN,
                    HTTP,
                    HTTPS,
                };
                schemes scheme{ schemes::UNKNOWN };
                std::string domainName;
                std::string path;
                std::string ip;
                int port;
                static uri parse(std::string origin)
                {
                    using namespace std;
                    static map<string, std::tuple<schemes, int>> supportedSchemes
                    {
                        { "http://", std::tuple<schemes, int>(schemes::HTTP, 80) },
                        { "https://", std::tuple<schemes, int>(schemes::HTTPS, 443) },
                    };
                    auto schemeIt = supportedSchemes.end();
                    for (auto it = supportedSchemes.begin(); it != supportedSchemes.end(); ++it)
                    {
                        const auto& i = it->first;
                        auto pos = origin.find(i);
                        if (pos == origin.npos || pos != 0)
                            continue;
                        if (schemeIt != supportedSchemes.end() && i.length() <= schemeIt->first.length())
                            continue;
                        schemeIt = it;
                    }
                    if (schemeIt == supportedSchemes.end())
                        throw runtime_error("not supported scheme");
                    auto path = origin.substr(schemeIt->first.length());
                    auto firstSlashPos = path.find('/');
                    auto domainName = path.substr(0, firstSlashPos);
                    if (firstSlashPos == path.npos)
                        path = "/";
                    else
                        path = path.substr(firstSlashPos);
                    auto pos = domainName.find(':');
                    auto port = get<1>(schemeIt->second);
                    if (pos != domainName.npos && pos != 0)
                    {
                        domainName = domainName.substr(0, pos);
                        port = stol(domainName.substr(pos + 1));
                    }
                    auto ip = domainName;
                    auto tokens = ext(domainName).split(".");
                    if (tokens.size() == 4 && all_of(tokens.begin(), tokens.end(), [](const string& i) { return all_of(i.begin(), i.end(), [](const char& i) { return ::isdigit(i); }); }))
                    {
#ifdef _MSC_VER
                        sockaddr_in addr;
                        addr.sin_family = AF_INET;
                        if (!inet_pton(AF_INET, ip.c_str(), &addr.sin_addr.s_addr))
                            throw system_error(WSAGetLastError(), system_category());
                        addr.sin_port = htons(port);
                        string domainNameBuffer(NI_MAXHOST, 0);
                        string serviceName(NI_MAXSERV, 0);
                        if (getnameinfo((sockaddr*)&addr, sizeof(sockaddr), (char*)domainNameBuffer.data(), domainNameBuffer.size(), (char*)serviceName.data(), serviceName.size(), NI_NUMERICSERV))
                            throw system_error(WSAGetLastError(), system_category());
                        domainNameBuffer.resize(domainNameBuffer.find('\0'));
                        domainName = domainNameBuffer;
#endif
                    }
                    else
                    {
#ifdef _MSC_VER
                        struct addrinfo hints { 0 };
                        hints.ai_family = AF_UNSPEC;
                        hints.ai_socktype = SOCK_STREAM;
                        hints.ai_protocol = IPPROTO_TCP;
                        auto serviceName = std::to_string(port);
                        PADDRINFOA result;
                        if (getaddrinfo(domainName.c_str(), serviceName.c_str(), &hints, &result))
                            throw system_error(WSAGetLastError(), system_category());
                        for (auto it = result; it != nullptr; it = it->ai_next)
                        {
                            if (it->ai_family != AF_INET)
                                continue;
                            const auto& addr = *(sockaddr_in*)it->ai_addr;
                            string ipBuffer(16, 0);
                            inet_ntop(addr.sin_family, &addr.sin_addr, (char*)ipBuffer.data(), ipBuffer.size());
                            ip = ipBuffer;
                            break;
                        }
#endif
                    }
                    uri uri;
                    uri.scheme = get<0>(schemeIt->second);
                    uri.domainName = move(domainName);
                    uri.path = move(path);
                    uri.ip = move(ip);
                    uri.port = port;
                    return uri;
                }
                uri() = default;
                uri(const uri& from) = default;
                uri(uri&&) = default;
                uri& operator=(const uri& from) = default;
                uri& operator=(uri&&) = default;
            };
        }
    }
}