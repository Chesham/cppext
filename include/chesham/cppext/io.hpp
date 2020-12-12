#pragma once
#include <future>
#include <vector>
#include <memory>
#include <mutex>
#include <numeric>
#include <unordered_map>
#include <type_traits>
#include <Windows.h>
#include "finalizer.hpp"

namespace chesham
{
    namespace cppext
    {
        namespace io
        {
            using buffer_type = std::vector<char>;
            using handle_type = std::shared_ptr<HANDLE>;
            struct io_type {};
            struct async_io : public io_type {};
            struct io_context
            {
                virtual ~io_context() {}
            };
            class io_traits
            {
            public:
                buffer_type make_buffer(const std::string& s) const
                {
                    return buffer_type{ std::make_move_iterator(s.begin()), std::make_move_iterator(s.end()) };
                }
            protected:
                handle_type handle;
                handle_type make_handle(HANDLE handle) const
                {
                    return handle_type(new HANDLE(handle), [](HANDLE* p)
                        {
                            if (!p)
                                return;
                            auto h = *p;
                            delete p;
                            if (!h)
                                return;
                            ::CloseHandle(h);
                        });
                }
                std::size_t minCrumbSize{ 65536 };
                std::size_t maxCrumbSize{ 536870912 };
                enum class task_types
                {
                    read,
                    write
                };
                struct task
                {
                    virtual ~task() { }
                    virtual task_types type() const = 0;
                    virtual void set_exception(std::exception_ptr ex) = 0;
                    virtual void wait() = 0;
                };
                template<typename T>
                struct task_traits : public task
                {
                    using promise_type = std::promise<T>;
                    using future_type = std::shared_future<T>;
                    using value_type = T;
                };
                template<task_types T>
                struct task_;
                template<>
                class task_<task_types::read> final : public task_traits<std::shared_ptr<buffer_type>>
                {
                    promise_type prom;
                    future_type fut{ prom.get_future() };
                public:
                    virtual task_types type() const override
                    {
                        return task_types::read;
                    }
                    virtual void set_exception(std::exception_ptr ex) override
                    {
                        prom.set_exception(ex);
                    }
                    virtual void wait() override
                    {
                        fut.wait();
                    }
                    future_type get_future()
                    {
                        return fut;
                    }
                    void set_value(value_type&& v)
                    {
                        prom.set_value(std::forward<decltype(v)>(v));
                    }
                };
                template<>
                class task_<task_types::write> final : public task_traits<std::size_t>
                {
                    promise_type prom;
                    future_type fut{ prom.get_future() };
                public:
                    virtual task_types type() const override
                    {
                        return task_types::write;
                    }
                    virtual void set_exception(std::exception_ptr ex) override
                    {
                        prom.set_exception(ex);
                    }
                    virtual void wait() override
                    {
                        fut.wait();
                    }
                    future_type get_future()
                    {
                        return fut;
                    }
                    void set_value(value_type&& v)
                    {
                        prom.set_value(std::forward<decltype(v)>(v));
                    }
                };
                using write_task = task_<task_types::write>;
                using read_task = task_<task_types::read>;
            };

            class async_file final : public io_traits, public io_context
            {
            private:
                struct file_options
                {
                    DWORD desiredAccess{ GENERIC_READ };
                    DWORD shareMode{ FILE_SHARE_READ };
                    DWORD creationDisposition{ OPEN_EXISTING };
                };
                struct overlapped_context : public OVERLAPPED
                {
                    io_context* ctx{ nullptr };
                    std::mutex mtx;
                    std::size_t bytesToRead{ 0 };
                    std::size_t bytesRead{ 0 };
                    std::size_t crumbSize{ 0 };
                    std::size_t byteOffsets{ 0 };
                    std::shared_ptr<buffer_type> buffer;
                    std::vector<std::shared_ptr<buffer_type>> buffers;
                    std::unique_ptr<task> task;
                };
                std::mutex mtx;
                std::condition_variable signal;
                file_options fileOptions;
                handle_type handle;
                std::unordered_map<LPOVERLAPPED, std::unique_ptr<overlapped_context>> ctxs;
                template<typename T>
                void erase(T&& key)
                {
                    std::lock_guard<std::mutex> l(mtx);
                    ctxs.erase(std::forward<T>(key));
                    if (ctxs.empty())
                        signal.notify_all();
                }
                template<typename ...Args>
                void emplace(Args ...args)
                {
                    std::lock_guard<std::mutex> l(mtx);
                    ctxs.emplace(std::forward<decltype(args)>(args)...);
                }
                void readAsync(overlapped_context& ctx, std::size_t size)
                {
                    ctx.buffer = std::make_shared<buffer_type>(size);
                    ctx.Offset = ctx.bytesRead + ctx.byteOffsets;
                    if (!::ReadFile(*handle, ctx.buffer->data(), ctx.buffer->size(), nullptr, &ctx) && ::GetLastError() != ERROR_IO_PENDING)
                        throw std::system_error(::GetLastError(), std::system_category());
                }
                static void WINAPI completionRoutine(DWORD errorCode, DWORD transferedBytes, LPOVERLAPPED overlappedKey)
                {
                    finalizer<> fin;
                    {
                        auto& overlapped = *reinterpret_cast<overlapped_context*>(overlappedKey);
                        if (!dynamic_cast<async_file*>(overlapped.ctx))
                            return;
                        std::lock_guard<std::mutex> l(overlapped.mtx);
                        auto& source = dynamic_cast<async_file&>(*overlapped.ctx);
                        if (transferedBytes != 0)
                        {
                            if (errorCode)
                            {
                                overlapped.task->set_exception(std::make_exception_ptr(std::system_error(::GetLastError(), std::system_category())));
                                return;
                            }
                            if (overlapped.task->type() == task_types::read)
                            {
                                overlapped.bytesRead += transferedBytes;
                                overlapped.buffer->resize(transferedBytes);
                                overlapped.buffers.emplace_back(std::move(overlapped.buffer));
                            }
                        }
                        if (overlapped.bytesRead < overlapped.bytesToRead && transferedBytes != 0)
                        {
                            auto targetSize = overlapped.crumbSize;
                            if (targetSize < source.maxCrumbSize)
                                targetSize *= 2;
                            auto remainBytes = overlapped.bytesToRead - overlapped.bytesRead;
                            if (targetSize > remainBytes)
                                targetSize = remainBytes;
                            overlapped.crumbSize = targetSize;
                            try
                            {
                                source.readAsync(overlapped, overlapped.crumbSize);
                            }
                            catch (const std::exception&)
                            {
                                overlapped.task->set_exception(std::current_exception());
                            }
                            return;
                        }
                        fin([&] { source.erase(overlappedKey); });
                        switch (overlapped.task->type())
                        {
                        case task_types::read:
                        {
                            auto& task = dynamic_cast<read_task&>(*overlapped.task);
                            if (overlapped.buffers.size() == 1)
                            {
                                task.set_value(std::move(overlapped.buffers.front()));
                            }
                            else
                            {
                                auto totalSize = std::accumulate(overlapped.buffers.begin(), overlapped.buffers.end(), 0, [](auto a, const auto& c) { return a + c->size(); });
                                auto buffer = std::make_shared<buffer_type>(totalSize);
                                auto dest = buffer->begin();
                                for (auto it = overlapped.buffers.begin(); it != overlapped.buffers.end(); )
                                {
                                    auto& buffer = **it;
                                    std::copy(buffer.begin(), buffer.end(), dest);
                                    std::advance(dest, buffer.size());
                                    it = overlapped.buffers.erase(it);
                                }
                                task.set_value(std::move(buffer));
                            }
                            break;
                        }
                        case task_types::write:
                        {
                            auto& task = dynamic_cast<write_task&>(*overlapped.task);
                            task.set_value(std::forward<decltype(transferedBytes)>(transferedBytes));
                            break;
                        }
                        default:
                            break;
                        }
                    }
                }
                file_options evalOptions(int mode) const
                {
                    file_options fo;
                    auto& desiredAccess = fo.desiredAccess;
                    auto& shareMode = fo.shareMode;
                    auto& creationDisposition = fo.creationDisposition;
                    if (mode & std::ios::out)
                    {
                        desiredAccess = GENERIC_WRITE;
                        if (mode & std::ios::trunc)
                            creationDisposition = TRUNCATE_EXISTING;
                        else
                        {
                            if (mode & std::ios::app)
                            {
                                desiredAccess = FILE_APPEND_DATA;
                                creationDisposition = OPEN_ALWAYS;
                            }
                            else
                                creationDisposition = CREATE_ALWAYS;
                        }
                        desiredAccess |= FILE_READ_DATA;
                    }
                    return fo;
                }
                handle_type open(const std::string& path, file_options fileOptions) const
                {
                    auto h = ::CreateFileA(path.c_str(), fileOptions.desiredAccess, fileOptions.shareMode, nullptr, fileOptions.creationDisposition, FILE_FLAG_OVERLAPPED, nullptr);
                    if (h == INVALID_HANDLE_VALUE && ::GetLastError())
                        throw std::system_error(::GetLastError(), std::system_category());
                    return make_handle(std::move(h));
                }
            public:
                async_file(const std::string& path) :
                    fileOptions(evalOptions(std::ios::in)),
                    handle(open(path, fileOptions))
                {
                    if (!::BindIoCompletionCallback(*handle, &completionRoutine, 0))
                        throw std::system_error(::GetLastError(), std::system_category());
                }
                async_file(const std::string& path, int mode) :
                    fileOptions(evalOptions(mode)),
                    handle(open(path, fileOptions))
                {
                    if (!::BindIoCompletionCallback(*handle, &completionRoutine, 0))
                        throw std::system_error(::GetLastError(), std::system_category());
                }
                virtual ~async_file()
                {
                    std::unique_lock<std::mutex> l(mtx);
                    signal.wait(l, [&] { return ctxs.empty(); });
                }
                read_task::future_type read_bytes(std::size_t byteOffsets, std::size_t length)
                {
                    if (!length)
                        throw std::invalid_argument("The reading length cannot be zero.");
                    auto ctx = std::make_unique<overlapped_context>();
                    std::lock_guard<std::mutex> l(ctx->mtx);
                    ctx->byteOffsets = byteOffsets;
                    ctx->ctx = this;
                    ctx->bytesToRead = length;
                    ctx->crumbSize = length;
                    readAsync(*ctx, ctx->crumbSize);
                    auto task = std::make_unique<read_task>();
                    auto fut = task->get_future();
                    ctx->task = move(task);
                    auto k = ctx.get();
                    emplace(k, std::move(ctx));
                    return fut;
                }
                write_task::future_type write_bytes(std::size_t byteOffsets, const buffer_type& buffer)
                {
                    if (buffer.empty())
                        throw std::invalid_argument("The buffer cannot be empty while writing.");
                    auto ctx = std::make_unique<overlapped_context>();
                    std::lock_guard<std::mutex> l(ctx->mtx);
                    ctx->byteOffsets = byteOffsets;
                    ctx->ctx = this;
                    ctx->Offset = ctx->byteOffsets;
                    if (!::WriteFile(*handle, buffer.data(), buffer.size(), nullptr, ctx.get()) && ::GetLastError() != ERROR_IO_PENDING)
                        throw std::system_error(::GetLastError(), std::system_category());
                    auto task = std::make_unique<write_task>();
                    auto fut = task->get_future();
                    ctx->task = move(task);
                    auto k = ctx.get();
                    emplace(k, std::move(ctx));
                    return fut;
                }
                template<typename T>
                typename std::enable_if<
                    std::is_base_of<std::input_iterator_tag, typename decltype(std::declval<T>().begin())::iterator_category>::value,
                    write_task::future_type>::type
                    write_string(std::size_t byteOffsets, T&& v)
                {
                    return write_bytes(byteOffsets, buffer_type{ std::make_move_iterator(v.begin()), std::make_move_iterator(v.end()) });
                }
            };
        }
    }
}
