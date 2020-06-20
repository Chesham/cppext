#include "pch.h"
using namespace std;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace chesham;

namespace test
{
    TEST_MODULE_CLEANUP(cleanup)
    {
        _CrtDumpMemoryLeaks();
    }

    TEST_CLASS(io_test)
    {
    public:

        TEST_METHOD(讀取大小正好符合)
        {
            auto target = make_shared<cppext::io::async_file>(R"(Lorem Ipsum.txt)");
            auto actual = target->read_bytes(0, 2870).get();
            Assert::AreEqual((size_t)2870, actual->size());
        }

        TEST_METHOD(讀取大小大於目標)
        {
            auto target = make_shared<cppext::io::async_file>("Lorem Ipsum.txt");
            auto actual = target->read_bytes(0, 3000).get();
            Assert::AreEqual((size_t)2870, actual->size());
        }

        TEST_METHOD(讀取大小小於目標)
        {
            auto target = make_shared<cppext::io::async_file>("Lorem Ipsum.txt");
            auto actual = target->read_bytes(0, 2869).get();
            Assert::AreEqual((size_t)2869, actual->size());
        }

        TEST_METHOD(讀取大小不能為0)
        {
            auto target = make_shared<cppext::io::async_file>("Lorem Ipsum.txt");
            Assert::ExpectException<invalid_argument>([&] { target->read_bytes(0, 0); });
        }

        TEST_METHOD(讀取對象提早離開)
        {
            auto target = make_shared<cppext::io::async_file>("Lorem Ipsum.txt");
            target->read_bytes(0, 2870);
        }

        TEST_METHOD(讀取對象無法寫入)
        {
            auto target = make_shared<cppext::io::async_file>("Lorem Ipsum.txt");
            Assert::ExpectException<system_error>([&] { target->write_string(0, "0"s); });
        }

        TEST_METHOD(連續讀取)
        {
            auto target = make_shared<cppext::io::async_file>("Lorem Ipsum.txt");
            auto except = target->read_bytes(0, 10);
            target = make_shared<cppext::io::async_file>("Lorem Ipsum.txt");
            auto segs =
            {
                target->read_bytes(0, 5),
                target->read_bytes(5, 5)
            };
            decay<decltype(declval<decltype(segs)>().begin()->get())>::type::element_type join(accumulate(segs.begin(), segs.end(), 0, [](auto&& a, auto&& i) { return a + i.get()->size(); }));
            auto it = join.begin();
            for (const auto& i : segs)
            {
                const auto& src = *i.get();
                copy(src.begin(), src.end(), it);
                it += src.size();
            }
            Assert::IsTrue(cppext::sequence_equal(except.get()->begin(), except.get()->end(), join.begin(), join.end()));
        }

        TEST_METHOD(寫入緩衝區不能為空)
        {
            auto target = make_shared<cppext::io::async_file>("Lorem Ipsum.txt");
            Assert::ExpectException<invalid_argument>([&] { target->write_bytes(0, {}); });
        }

        TEST_METHOD(寫入測試)
        {
            auto target = make_shared<cppext::io::async_file>("output.txt", ios::out | ios::app);
            auto data = "hello world\n"s;
            auto fut = target->write_string(0, data);
            Assert::AreEqual(data.size(), fut.get());
        }

        TEST_METHOD(寫入對象可以讀取)
        {
            auto target = make_shared<cppext::io::async_file>("Lorem Ipsum.txt", ios::out | ios::app);
            auto fut = target->read_bytes(0, 10);
            Assert::AreEqual((size_t)10, fut.get()->size());
        }
    };
}
