#pragma once
#include <algorithm>
#include <cctype>
#include <functional>

namespace chesham
{
    template<class T>
    struct string_no_case_comparer
    {
        T& to_lower(T& s) const
        {
            transform(s.begin(), s.end(), s.begin(), [](const auto& i) { return ::tolower(i); });
            return s;
        }
        std::size_t operator()(T s) const
        {
            using namespace std;
            return std::hash<T>()(to_lower(s));
        }
        bool operator()(T x, T y) const
        {
            using namespace std;
            return to_lower(x) == to_lower(y);
        }
    };
}