#include "pch.h"
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;
using namespace chesham;

namespace test
{
    TEST_CLASS(string_tests)
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

        TEST_METHOD(replace_str_cstr)
        {
            auto src = "hello world heworldllo"s;
            auto expect = "hello c++ hec++llo"s;
            auto actual = cppext::ext(src).replace("world", "c++"s);
            Assert::AreEqual(expect, (decltype(expect))actual);
        }

        TEST_METHOD(replace_str_int)
        {
            auto src = "hello world heworldllo"s;
            auto expect = "hello 123 he123llo"s;
            auto actual = cppext::ext(src).replace("world", 123);
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

        TEST_METHOD(replace_wstr_wcstr)
        {
            auto src = L"hello world heworldllo"s;
            auto expect = L"hello c++ hec++llo"s;
            auto actual = cppext::ext(src).replace(L"world"s, L"c++");
            Assert::AreEqual(expect, (decltype(expect))actual);
        }

        TEST_METHOD(replace_wstr_int)
        {
            auto src = L"hello world heworldllo"s;
            auto expect = L"hello 123 he123llo"s;
            auto actual = cppext::ext(src).replace(L"world"s, 123);
            Assert::AreEqual(expect, (decltype(expect))actual);
        }

        TEST_METHOD(replace_limit_str)
        {
            auto src = "hello world heworldllo"s;
            auto expect = "hello c++ heworldllo"s;
            auto actual = cppext::ext(src).replace("world"s, "c++"s, 1);
            Assert::AreEqual(expect, (decltype(expect))actual);
        }

        TEST_METHOD(replace_limit)
        {
            auto src = "hello world heworldllo"s;
            auto expect = "hello c++ heworldllo"s;
            auto actual = cppext::ext(src).replace("world", "c++"s, 1);
            Assert::AreEqual(expect, (decltype(expect))actual);
        }

        TEST_METHOD(split)
        {
            auto target = ",1,,2,3,4,5,6,";
            auto expect = vector<string>{ "", "1", "", "2", "3", "4", "5", "6", "" };
            auto actual = cppext::ext(target).split(",");
            Assert::IsTrue(cppext::sequence_equal(expect.begin(), expect.end(), actual.begin(), actual.end()));
        }

        TEST_METHOD(split_limit)
        {
            auto target = ",1,,2,3,4,5,6,";
            auto expect = vector<string>{ "", "1", "" };
            auto actual = cppext::ext(target).split(",", false, 3);
            Assert::IsTrue(cppext::sequence_equal(expect.begin(), expect.end(), actual.begin(), actual.end()));
        }

        TEST_METHOD(split_skipEmpty)
        {
            auto target = ",1,,2,3,4,5,6,";
            auto expect = vector<string>{ "1", "2", "3", "4", "5", "6" };
            auto actual = cppext::ext(target).split(",", true);
            Assert::IsTrue(cppext::sequence_equal(expect.begin(), expect.end(), actual.begin(), actual.end()));
        }

        TEST_METHOD(split_skipEmpty_limit)
        {
            auto target = ",1,,2,3,4,5,6,";
            auto expect = vector<string>{ "1", "2" };
            auto actual = cppext::ext(target).split(",", true, 2);
            Assert::IsTrue(cppext::sequence_equal(expect.begin(), expect.end(), actual.begin(), actual.end()));
        }
    };
}
