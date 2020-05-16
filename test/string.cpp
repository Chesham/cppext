#include "pch.h"
#include "CppUnitTest.h"
#include <chesham/cppext/cppext.hpp>
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;
using namespace chesham;

namespace test
{
    TEST_CLASS(string)
    {
    public:

        TEST_METHOD(replace_cstr)
        {
            auto src = "hello world heworldllo"s;
            auto expect = "hello c++ hec++llo"s;
            auto actual = cppext::ext(src).replace("world", "c++");
            Assert::AreEqual(expect, (decltype(expect))actual);
        }

        TEST_METHOD(replace)
        {
            auto src = "hello world heworldllo"s;
            auto expect = "hello c++ hec++llo"s;
            auto actual = cppext::ext(src).replace("world"s, "c++"s);
            Assert::AreEqual(expect, (decltype(expect))actual);
        }

        TEST_METHOD(replace_wcstr)
        {
            auto src = L"hello world heworldllo"s;
            auto expect = L"hello c++ hec++llo"s;
            auto actual = cppext::ext(src).replace(L"world", L"c++");
            Assert::AreEqual(expect, (decltype(expect))actual);
        }

        TEST_METHOD(replace_wstr)
        {
            auto src = L"hello world heworldllo"s;
            auto expect = L"hello c++ hec++llo"s;
            auto actual = cppext::ext(src).replace(L"world"s, L"c++"s);
            Assert::AreEqual(expect, (decltype(expect))actual);
        }
    };
}
