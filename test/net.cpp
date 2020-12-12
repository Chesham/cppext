#include "pch.h"
#include <utility>
#include <chesham/cppext/cppext.hpp>
using namespace std;
using namespace std::chrono;
using namespace chesham::cppext;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace test
{
    TEST_CLASS(net_test)
    {
    public:

        TEST_METHOD(test_get_plain_text)
        {
            net::http_client cli;
            cli.autoDecompression = false;
            auto task = cli.get("http://ptsv2.com/t/math6-1609257285/post");
            auto& response = *task.get();
            try
            {
                const auto& buffers = response.buffers();
                const auto& headers = response.headers();
                stringstream ss;
                for (const auto& buffer : buffers)
                    ss << string(buffer.begin(), buffer.end());
                auto _ = ss.str();
                ss.str("");
                ss << "statusCode: " << response.statusCode() << endl
                    << "elapsed: " << duration_cast<milliseconds>(response.elapsed).count() << "ms" << endl;
                for (const auto& k : headers)
                    ss << k.first << ": " << k.second << endl;
                OutputDebugStringA(ss.str().c_str());
            }
            catch (const exception& ex)
            {
                OutputDebugStringA(ex.what());
                OutputDebugStringA("\n");
                Assert::Fail(wstring(ex.what(), ex.what() + strlen(ex.what())).c_str());
            }
        }

        TEST_METHOD(test_get_with_auto_decompression)
        {
            net::http_client cli;
            auto task = cli.get("http://ptsv2.com/t/math6-1609257285/post");
            auto& response = *task.get();
            try
            {
                const auto& buffers = response.buffers();
                const auto& headers = response.headers();
                stringstream ss;
                for (const auto& buffer : buffers)
                    ss << string(buffer.begin(), buffer.end());
                auto _ = ss.str();
                ss.str("");
                ss << "statusCode: " << response.statusCode() << endl
                    << "elapsed: " << duration_cast<milliseconds>(response.elapsed).count() << "ms" << endl;
                for (const auto& k : headers)
                    ss << k.first << ": " << k.second << endl;
                OutputDebugStringA(ss.str().c_str());
            }
            catch (const exception& ex)
            {
                OutputDebugStringA(ex.what());
                OutputDebugStringA("\n");
                Assert::Fail(wstring(ex.what(), ex.what() + strlen(ex.what())).c_str());
            }
        }

        TEST_METHOD(test_post)
        {
            net::http_client cli;
            vector<decltype(declval<net::http_client>().post({}, {}))> tasks;
            auto times = 100;
            for (auto i = 0; i < times; ++i)
            {
                tasks.emplace_back(cli.post("http://ptsv2.com/t/math6-1609257285/post", make_shared<net::buffer_type>(net::buffer_type(5, '?'))));
            }
            for (auto& task : tasks)
            {
                auto& response = *task.get();
                try
                {
                    const auto& buffers = response.buffers();
                    const auto& headers = response.headers();
                    stringstream ss;
                    for (const auto& buffer : buffers)
                        ss << string(buffer.begin(), buffer.end());
                    auto _ = ss.str();
                    ss.str("");
                    ss << "statusCode: " << response.statusCode() << endl
                        << "elapsed: " << duration_cast<milliseconds>(response.elapsed).count() << "ms" << endl;
                    for (const auto& k : headers)
                        ss << k.first << ": " << k.second << endl;
                    OutputDebugStringA(ss.str().c_str());
                }
                catch (const exception& ex)
                {
                    OutputDebugStringA(ex.what());
                    OutputDebugStringA("\n");
                    Assert::Fail(wstring(ex.what(), ex.what() + strlen(ex.what())).c_str());
                }
            }
        }

        TEST_METHOD(test_get_massive)
        {
            net::http_client cli;
            auto concurrency = 100;
            vector<shared_future<unique_ptr<net::http_response>>> tasks;
            for (auto i = 0; i < concurrency; ++i)
            {
            auto task = cli.get("http://example.com");
            tasks.emplace_back(move(task));
            }
            for (auto& task : tasks)
            {
                auto& response = *task.get();
                try
                {
                    const auto& buffers = response.buffers();
                    const auto& headers = response.headers();
                    stringstream ss;
                    for (const auto& buffer : buffers)
                        ss << string(buffer.begin(), buffer.end());
                    auto _ = ss.str();
                    ss.str("");
                    ss << "statusCode: " << response.statusCode() << endl
                        << "elapsed: " << duration_cast<milliseconds>(response.elapsed).count() << "ms" << endl;
                    //for (const auto& k : headers)
                    //    ss << k.first << ": " << k.second << endl;
                    OutputDebugStringA(ss.str().c_str());
                }
                catch (const exception& ex)
                {
                    OutputDebugStringA(ex.what());
                    OutputDebugStringA("\n");
                    Assert::Fail(wstring(ex.what(), ex.what() + strlen(ex.what())).c_str());
                }
            }
        }

        TEST_METHOD(test_parser)
        {
            vector<string> content
            {
                "HTTP/1.1 20",
                "0 OK\r\n",
                "Accept-Ranges: bytes",
                "\r\nAge: 356",
                "764\r\n",
                "\r\n"
            };
            const auto& headers = net::http_parser().parseHeaders(content);
            Assert::AreEqual("bytes"s, headers.at("Accept-Ranges"));
            Assert::AreEqual("356764"s, headers.at("Age"));
        }

        TEST_METHOD(test_header_case_insensitive)
        {
            vector<string> content
            {
                "HTTP/1.1 200 OK\r\n",
                "Accept-Ranges: bytes\r\n"
            };
            const auto& headers = net::http_parser().parseHeaders(content);
            Assert::AreEqual("bytes"s, headers.at("aCCept-RaNges"));
        }

        TEST_METHOD(test_noheader)
        {
            vector<string> content
            {
                "HTTP/1.1 200 OK\r\n\r\n"
            };
            const auto& headers = net::http_parser().parseHeaders(content);
            Assert::IsTrue(headers.empty());
        }

        TEST_METHOD(test_wrong_status_code)
        {
            vector<string> content
            {
                "HTTP/1.1 invalid-code OK\r\n\r\n"
            };
            Assert::ExpectException<invalid_argument>([&] { net::http_parser().parseStatusCode(content); });
        }
    };
}