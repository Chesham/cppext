#pragma once
#include <WinSock2.h>
#include <MSWSock.h>
#include <WS2tcpip.h>
#include <memory>
#include <system_error>
#include <future>
#include <chrono>
#include "net_response.hpp"
#include "uri.hpp"
#pragma comment(lib, "Kernel32.lib")
#pragma comment(lib, "Ws2_32.lib")

namespace std
{
    template<>
    struct default_delete<SOCKET>
    {
        void operator()(SOCKET* p) const
        {
            if (p)
            {
                if (*p != INVALID_SOCKET)
                    closesocket(*p);
                delete p;
            }
        }
    };
    template<>
    struct default_delete<HANDLE>
    {
        void operator()(HANDLE* p) const
        {
            if (p)
            {
                if (*p != INVALID_HANDLE_VALUE)
                    CloseHandle(*p);
                delete p;
            }
        }
    };
}

namespace chesham
{
    namespace cppext
    {
        namespace net
        {
            template<class T = void>
            class http_impl
            {
            public:
                static std::unique_ptr<HANDLE> iocp;
                static void cleanupIOCP()
                {
                    iocp.reset();
                }
                struct overlapped_context : public OVERLAPPED
                {
                    std::chrono::system_clock::time_point startAt{ std::chrono::system_clock::now() };
                    std::unique_ptr<SOCKET> socket;
                    std::function<void(DWORD, DWORD)> onCompleted;
                };
                class managed_wsabuf final
                {
                private:
                    WSABUF underlying;
                    std::vector<char> buffer;
                public:
                    managed_wsabuf() = default;
                    managed_wsabuf(const managed_wsabuf&) = delete;
                    managed_wsabuf(managed_wsabuf&&) = delete;
                    managed_wsabuf& operator=(const managed_wsabuf&) = delete;
                    managed_wsabuf& operator=(managed_wsabuf&&) = delete;
                    operator LPWSABUF()
                    {
                        return &underlying;
                    }
                    managed_wsabuf& operator=(std::vector<char>&& from)
                    {
                        buffer = move(from);
                        underlying.buf = buffer.data();
                        underlying.len = buffer.size();
                        return *this;
                    }
                    operator std::vector<char> && ()
                    {
                        underlying.buf = nullptr;
                        underlying.len = 0;
                        return std::move(buffer);
                    }
                };
                static void WINAPI onCompleted(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
                {
                    auto& ctx = *(overlapped_context*)lpOverlapped;
                    if (ctx.onCompleted)
                        ctx.onCompleted(dwErrorCode, dwNumberOfBytesTransfered);
                }
                std::shared_future<std::unique_ptr<http_response>> perform(const http_traits& traits, std::string::const_pointer url, const std::shared_ptr<buffer_type>& buffer) const
                {
                    using namespace std;
                    static auto createIOCP = []
                    {
                        auto h = std::make_unique<HANDLE>(CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0));
                        if (!h)
                            throw system_error(GetLastError(), system_category());
                        return h;
                    };
                    if (!iocp)
                    {
                        iocp = createIOCP();
                        atexit(cleanupIOCP);
                    }
                    static auto load = []
                    {
                        WSADATA wsaData;
                        if (WSAStartup(MAKEWORD(2, 2), &wsaData))
                            throw system_error(GetLastError(), system_category());
                        if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
                        {
                            WSACleanup();
                            throw runtime_error("Your computer is from the wrong millenium.");
                        }
                        auto sock = make_unique<SOCKET>(socket(AF_INET, SOCK_STREAM, 0));
                        if (*sock == INVALID_SOCKET)
                            throw system_error(GetLastError(), system_category());
                        GUID guid = WSAID_CONNECTEX;
                        DWORD dwBytes{};
                        LPFN_CONNECTEX connectEx{ nullptr };
                        auto rc = WSAIoctl(*sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &connectEx, sizeof(connectEx), &dwBytes, nullptr, nullptr);
                        if (rc != 0)
                            throw system_error(GetLastError(), system_category());
                        return connectEx;
                    };
                    static LPFN_CONNECTEX connectEx{ nullptr };
                    if (!connectEx)
                        connectEx = load();
                    auto connect = [](LPFN_CONNECTEX connectEx, string::const_pointer addrString)
                    {
                        auto sock = make_unique<SOCKET>(socket(AF_INET, SOCK_STREAM, 0));
                        if (*sock == INVALID_SOCKET)
                            throw system_error(WSAGetLastError(), system_category());
                        sockaddr_in addr{ 0 };
                        addr.sin_family = AF_INET;
                        addr.sin_addr.s_addr = INADDR_ANY;
                        addr.sin_port = 0;
                        if (::bind(*sock, (SOCKADDR*)&addr, sizeof(addr)))
                            throw system_error(WSAGetLastError(), system_category());
                        auto ctx = make_unique<overlapped_context>();
                        addr = { 0 };
                        addr.sin_family = AF_INET;
                        if (!inet_pton(AF_INET, addrString, &addr.sin_addr.s_addr))
                            throw system_error(WSAGetLastError(), system_category());
                        addr.sin_port = htons(80);
                        if (connectEx(*sock, (SOCKADDR*)&addr, sizeof(addr), nullptr, 0, nullptr, ctx.get()) && WSAGetLastError() != ERROR_IO_PENDING)
                            throw system_error(WSAGetLastError(), system_category());
                        ctx->socket = move(sock);
                        return ctx;
                    };
                    auto uri = make_shared<struct uri>(uri::parse(url));
                    auto ctx = connect(connectEx, uri->ip.c_str());
                    if (!BindIoCompletionCallback((HANDLE)*ctx->socket, &onCompleted, 0))
                        throw system_error(GetLastError(), system_category());
                    auto prom = make_shared<promise<unique_ptr<http_response>>>();
                    auto task = prom->get_future();
                    static unordered_map<LPOVERLAPPED, unique_ptr<overlapped_context>> ctxs;
                    auto& ctxsRef = ctxs;
                    ctx->onCompleted = [&ctxsRef, ctx = ctx.get(), prom = move(prom), uri = move(uri), buffer, traits](DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered) mutable
                    {
                        if (dwErrorCode)
                        {
                            auto err = system_error(dwErrorCode, system_category());
                            prom->set_exception(make_exception_ptr(err));
                            ctxsRef.erase(ctx);
                        }
                        else
                        {
                            auto wsaBuf = make_shared<managed_wsabuf>();
                            *wsaBuf = vector<char>(make_move_iterator(buffer->begin()), make_move_iterator(buffer->end()));
                            auto ctxKeeper = ctx;
                            ctx->onCompleted = [&ctxsRef, ctx, prom, wsaBuf, traits](DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered) mutable
                            {
                                *wsaBuf = vector<char>(64, 0);
                                DWORD flags{ 0 };
                                vector<vector<char>> buffers;
                                auto promCopy = prom;
                                auto wsaBufCopy = wsaBuf;
                                auto ctxKeeper = ctx;
                                ctx->onCompleted = [&ctxsRef, ctx, prom, wsaBuf, buffers = move(buffers), traits](DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered) mutable
                                {
                                    if (dwErrorCode)
                                    {
                                        prom->set_exception(make_exception_ptr(system_error(dwErrorCode, system_category())));
                                        return;
                                    }
                                    if (dwNumberOfBytesTransfered > 0)
                                    {
                                        auto buf = (vector<char>&&) * wsaBuf;
                                        if (dwNumberOfBytesTransfered != buf.size())
                                        {
                                            buf.resize(dwNumberOfBytesTransfered);
                                            buffers.emplace_back(move(buf));
                                            unique_ptr<http_response> response;
                                            if (traits.autoDecompression)
                                                response = http_response::parse(decltype(buffers)(), [buffers = move(buffers)](promise<vector<vector<char>>>& prom) mutable
                                                {
                                                    http_response::decompress(move(buffers), prom);
                                                });
                                            else
                                                response = http_response::parse(move(buffers));
                                            response->elapsed = chrono::system_clock::now() - ctx->startAt;
                                            auto promKeeper = prom;
                                            ctxsRef.erase(ctx);
                                            promKeeper->set_value(move(response));
                                            return;
                                        }
                                        else
                                            buffers.emplace_back(move(buf));
                                        *wsaBuf = vector<char>(64, 0);
                                        DWORD flags{ 0 };
                                        if (WSARecv(*ctx->socket, *wsaBuf, 1, nullptr, &flags, ctx, nullptr) && WSAGetLastError() != ERROR_IO_PENDING)
                                            prom->set_exception(make_exception_ptr(system_error(WSAGetLastError(), system_category())));
                                        return;
                                    }
                                };
                                if (WSARecv(*ctxKeeper->socket, *wsaBufCopy, 1, nullptr, &flags, ctxKeeper, nullptr) && WSAGetLastError() != ERROR_IO_PENDING)
                                    promCopy->set_exception(make_exception_ptr(system_error(WSAGetLastError(), system_category())));
                            };
                            if (WSASend(*ctxKeeper->socket, *wsaBuf, 1, nullptr, 0, ctxKeeper, nullptr))
                                prom->set_exception(make_exception_ptr(system_error(WSAGetLastError(), system_category())));
                        }
                    };
                    ctxs.emplace(ctx.get(), move(ctx));
                    return task;
                }
            };
            std::unique_ptr<HANDLE> http_impl<>::iocp;
        }
    }
}