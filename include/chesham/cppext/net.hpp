#pragma once
#ifdef _MSC_VER
#include "net_windows.hpp"
#endif
#include <ostream>
#include <future>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace chesham
{
    namespace cppext
    {
        namespace net
        {
            class http_client final : public http_traits
            {
            private:
                http_impl<> impl;
                struct no_case
                {
                    bool operator()(std::string x, std::string y) const
                    {
                        using namespace std;
                        transform(x.begin(), x.end(), x.begin(), ::tolower);
                        transform(y.begin(), y.end(), y.begin(), ::tolower);
                        return x == y;
                    }
                    std::size_t operator()(std::string x) const
                    {
                        using namespace std;
                        transform(x.begin(), x.end(), x.begin(), ::tolower);
                        return hash<string>()(x);
                    }
                };
                std::unordered_set<std::string, no_case, no_case> invalidHeadersWhenPost
                {
                    "Content-Length",
                    "Accept-Encoding",
                };
                buffer_type createPayload(std::string::const_pointer method, std::string::const_pointer urlString, const std::shared_ptr<const headers_type>& headers, const std::shared_ptr<buffer_type>& buffer) const
                {
                    using namespace std;
                    auto uri = make_shared<struct uri>(uri::parse(urlString));
                    stringstream ss;
                    ss << method << " " << uri->path << " HTTP/" << to_string(httpVersion) << "\r\n";
                    auto addHostHeader = [&]
                    {
                        ss << "Host: " << uri->domainName << "\r\n";
                    };
                    if (headers)
                    {
                        if (!headers->count("Host"))
                            addHostHeader();
                        for (const auto& k : *headers)
                        {
                            if (buffer && invalidHeadersWhenPost.count(k.first))
                                continue;
                            ss << k.first << ": " << k.second << "\r\n";
                        }
                    }
                    else
                        addHostHeader();
                    if (autoDecompression)
                        ss << "Accept-Encoding: gzip" << "\r\n";
                    if (buffer)
                    {
                        ss << "Content-Length: " << buffer->size() << "\r\n\r\n";
                        copy(buffer->begin(), buffer->end(), ostream_iterator<char>(ss));
                    }
                    else
                        ss << "\r\n";
                    auto text = ss.str();
                    return buffer_type(text.begin(), text.end());
                }
            public:
                std::shared_future<std::unique_ptr<http_response>> get(std::string::const_pointer url, const std::shared_ptr<const headers_type>& headers = nullptr) const
                {
                    using namespace std;
                    auto payload = make_shared<buffer_type>(createPayload("GET", url, headers, nullptr));
                    return impl.perform(*this, url, payload);
                }
                std::shared_future<std::unique_ptr<http_response>> post(std::string::const_pointer url, const std::shared_ptr<buffer_type>& buffer, const std::shared_ptr<const headers_type>& headers = nullptr) const
                {
                    using namespace std;
                    auto payload = make_shared<buffer_type>(createPayload("POST", url, headers, buffer));
                    return impl.perform(*this, url, payload);
                }
            };
        }
    }
}