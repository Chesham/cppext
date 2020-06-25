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

        TEST_METHOD(Ū���j�p���n�ŦX)
        {
            auto target = make_shared<cppext::io::async_file>(R"(Lorem Ipsum.txt)");
            auto actual = target->read_bytes(0, 2870).get();
            Assert::AreEqual((size_t)2870, actual->size());
        }

        TEST_METHOD(Ū���j�p�j��ؼ�)
        {
            auto target = make_shared<cppext::io::async_file>("Lorem Ipsum.txt");
            auto actual = target->read_bytes(0, 3000).get();
            Assert::AreEqual((size_t)2870, actual->size());
        }

        TEST_METHOD(Ū���j�p�p��ؼ�)
        {
            auto target = make_shared<cppext::io::async_file>("Lorem Ipsum.txt");
            auto actual = target->read_bytes(0, 2869).get();
            Assert::AreEqual((size_t)2869, actual->size());
        }

        TEST_METHOD(Ū���j�p���ର0)
        {
            auto target = make_shared<cppext::io::async_file>("Lorem Ipsum.txt");
            Assert::ExpectException<invalid_argument>([&] { target->read_bytes(0, 0); });
        }

        TEST_METHOD(Ū����H�������})
        {
            auto target = make_shared<cppext::io::async_file>("Lorem Ipsum.txt");
            target->read_bytes(0, 2870);
        }

        TEST_METHOD(Ū����H�L�k�g�J)
        {
            auto target = make_shared<cppext::io::async_file>("Lorem Ipsum.txt");
            Assert::ExpectException<system_error>([&] { target->write_string(0, "0"s); });
        }

        TEST_METHOD(�s��Ū��)
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

        TEST_METHOD(�g�J�w�İϤ��ର��)
        {
            auto target = make_shared<cppext::io::async_file>("Lorem Ipsum.txt");
            Assert::ExpectException<invalid_argument>([&] { target->write_bytes(0, {}); });
        }

        TEST_METHOD(�g�J����)
        {
            auto target = make_shared<cppext::io::async_file>("output.txt", ios::out | ios::app);
            auto data = "hello world\n"s;
            auto fut = target->write_string(0, data);
            Assert::AreEqual(data.size(), fut.get());
        }

        TEST_METHOD(�g�J��H�i�HŪ��)
        {
            auto target = make_shared<cppext::io::async_file>("Lorem Ipsum.txt", ios::out | ios::app);
            auto fut = target->read_bytes(0, 10);
            Assert::AreEqual((size_t)10, fut.get()->size());
        }
    };
}
