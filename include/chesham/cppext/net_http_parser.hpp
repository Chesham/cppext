#pragma once
#include <vector>
#include <stack>
#include "net_http_header.hpp"

namespace chesham
{
    namespace cppext
    {
        namespace net
        {
            class http_parser
            {
            public:
                template<class T>
                headers_type parseHeaders(const T& buffers) const
                {
                    using namespace std;
                    auto match = "HTTP/"s;
                    auto matchIt = match.begin();
                    string current;
                    stack<function<bool(string&, string::const_reference c)>> parsers;
                    headers_type headers;
                    auto headerIt = headers.end();
                    parsers.emplace([&](string& a, string::const_reference c)
                        {
                            if (headerIt == headers.end())
                            {
                                if (c == ':')
                                {
                                    headerIt = headers.emplace(move(a), string{}).first;
                                    return true;
                                }
                            }
                            else
                            {
                                if (c == ' ')
                                    return true;
                                if (c == '\r' || c == '\n')
                                {
                                    headerIt->second = move(a);
                                    headerIt = headers.end();
                                }
                            }
                            if (c != '\r' && c != '\n' && all_of(a.begin(), a.end(), [](auto c) { return c == '\r' || c == '\n'; }))
                                a.clear();
                            a += c;
                            if (a == "\r\n\r\n")
                                return false;
                            return true;
                        });
                    parsers.emplace([&](string& a, string::const_reference c)
                        {
                            if (c == '\r')
                                a += c;
                            return a.empty() || c != '\n';
                        });
                    parsers.emplace([&](string& a, string::const_reference c)
                        {
                            if (c != ' ')
                            {
                                a += c;
                                return true;
                            }
                            //response.statusCode = stol(a);
                            return false;
                        });
                    parsers.emplace([&](string& a, string::const_reference c)
                        {
                            return c != ' ';
                        });
                    parsers.emplace([&](string& a, string::const_reference c)
                        {
                            if (c != *matchIt++)
                                throw runtime_error("parse error");
                            return matchIt != match.end();
                        });
                    for (const auto& buffer : buffers)
                    {
                        for (auto it = buffer.begin(); it != buffer.end() && !parsers.empty(); )
                        {
                            if (!parsers.top()(current, *it++))
                            {
                                parsers.pop();
                                current.clear();
                            }
                        }
                    }
                    return headers;
                }
                template<class T>
                int parseStatusCode(const T& buffers) const
                {
                    using namespace std;
                    auto match = "HTTP/"s;
                    auto matchIt = match.begin();
                    string current;
                    stack<function<bool(string&, string::const_reference c)>> parsers;
                    int statusCode{ 0 };
                    parsers.emplace([&](string& a, string::const_reference c)
                        {
                            if (c != ' ')
                            {
                                a += c;
                                return true;
                            }
                            statusCode = stol(a);
                            return false;
                        });
                    parsers.emplace([&](string& a, string::const_reference c)
                        {
                            return c != ' ';
                        });
                    parsers.emplace([&](string& a, string::const_reference c)
                        {
                            if (c != *matchIt++)
                                throw runtime_error("parse error");
                            return matchIt != match.end();
                        });
                    for (const auto& buffer : buffers)
                    {
                        for (auto it = buffer.begin(); it != buffer.end() && !parsers.empty(); )
                        {
                            if (!parsers.top()(current, *it++))
                            {
                                parsers.pop();
                                current.clear();
                            }
                        }
                    }
                    return statusCode;
                }
            };
        }
    }
}