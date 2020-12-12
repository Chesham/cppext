#pragma once
#include <algorithm>
#include <string>
#include <ostream>
#include <functional>
#include <future>
#include <chrono>
#include <vector>
#include <sstream>
#include <gzip/decompress.hpp>
#include "lazy.hpp"
#include "net_http_parser.hpp"

namespace chesham
{
    namespace cppext
    {
        namespace net
        {
            using buffer_type = std::vector<char>;
            class http_parser;
            class http_response final
            {
            public:
                friend class http_parser;
            private:
                mutable lazy<std::vector<std::vector<char>>> lazyBuffers;
                mutable lazy<headers_type> lazyHeaders;
                mutable lazy<int> lazyStatusCode;
            public:
                std::chrono::system_clock::duration elapsed{ 0 };
                const std::vector<std::vector<char>>& buffers() const
                {
                    return lazyBuffers;
                }
                const headers_type& headers() const
                {
                    return lazyHeaders;
                }
                const int& statusCode() const
                {
                    return lazyStatusCode;
                }
                http_response() :
                    lazyHeaders([this](std::promise<headers_type>& prom)
                        {
                            try
                            {
                                http_parser parser;
                                prom.set_value(parser.parseHeaders(lazyBuffers.get()));
                            }
                            catch (const std::exception&)
                            {
                                prom.set_exception(std::current_exception());
                            }
                        }),
                    lazyStatusCode([this](std::promise<int>& prom)
                        {
                            try
                            {
                                http_parser parser;
                                prom.set_value(parser.parseStatusCode(lazyBuffers.get()));
                            }
                            catch (const std::exception&)
                            {
                                prom.set_exception(std::current_exception());
                            }
                        })
                {
                };
                http_response(const http_response&) = delete;
                http_response(http_response&&) = delete;
                http_response& operator=(const http_response&) = delete;
                http_response& operator=(http_response&&) = delete;
                static void decompress(decltype(lazyBuffers)::element_type&& buffers, std::promise<decltype(lazyBuffers)::element_type>& prom)
                {
                    using namespace std;
                    stringstream ss;
                    for (const auto& buffer : buffers)
                        ss << string(buffer.begin(), buffer.end());
                    auto compressed = ss.str();
                    auto delimiter = "\r\n\r\n"s;
                    auto bodyStartAt = compressed.find(delimiter);
                    if (bodyStartAt != compressed.npos)
                        bodyStartAt += delimiter.length();
                    vector<vector<char>> decompressedbuffers{ vector<char>(compressed.begin(), compressed.begin() + bodyStartAt) };
                    if (bodyStartAt < compressed.size())
                    {
                        auto decompressed = gzip::decompress(compressed.data() + bodyStartAt, compressed.size() - bodyStartAt);
                        decompressedbuffers.emplace_back(vector<char>(decompressed.begin(), decompressed.end()));
                    }
                    prom.set_value(decompressedbuffers);
                }
                template<class T>
                static std::unique_ptr<http_response> parse(T&& buffers, const decltype(lazyBuffers)::value_function& valueFunction = nullptr)
                {
                    using namespace std;
                    auto response = make_unique<http_response>();
                    if (valueFunction)
                        response->lazyBuffers.set_value_function(valueFunction);
                    else
                        response->lazyBuffers.set_value_function([buffers = move(buffers)](promise<vector<vector<char>>>& prom)
                        {
                            prom.set_value(move(buffers));
                        });
                    return response;
                }
            };
            struct http_request
            {
                std::string address;
            };
        }
    }
}