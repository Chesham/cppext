#include "pch.h"
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;
using namespace chesham;

namespace test
{
    TEST_CLASS(algorithm_tests)
    {
    public:

        TEST_METHOD(sequence_equal)
        {
            auto s1 = { 10, 20, 33, 50, 230, 70 };
            Assert::IsTrue(cppext::sequence_equal(begin(s1), end(s1), begin(s1), end(s1)));

            auto s2 = { 10, 20, 33, 50, 230, 70 };
            Assert::IsTrue(cppext::sequence_equal(begin(s1), end(s1), begin(s2), end(s2)));

            auto s3 = { 10, 20, 32, 50, 230, 70 };
            Assert::IsFalse(cppext::sequence_equal(begin(s1), end(s1), begin(s3), end(s3)));

            auto s4 = { 10, 20, 33, 50, 70 };
            Assert::IsFalse(cppext::sequence_equal(begin(s1), end(s1), begin(s4), end(s4)));

            auto str1 = "hello world"s;
            auto str2 = "hello world"s;
            Assert::IsTrue(cppext::sequence_equal(str1.begin(), str1.end(), str1.begin(), str1.end()));
            Assert::IsTrue(cppext::sequence_equal(str1.begin(), str1.end(), str2.begin(), str2.end()));
        }
    };
}
