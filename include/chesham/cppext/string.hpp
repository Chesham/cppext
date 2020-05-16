#pragma once
#include <algorithm>
#include <string>
#include <type_traits>

namespace chesham
{
    namespace cppext
    {
        template<typename C>
        class string_exted final
        {
            typedef std::basic_string<C> underlying_type;
            underlying_type underlying;
        public:
            string_exted(const underlying_type& v) : underlying(v) {}
            inline string_exted replace(const underlying_type& from, const underlying_type& to) const
            {
                auto start = 0;
                auto end = underlying.find(from);
                underlying_type target;
                while (end != underlying_type::npos)
                {
                    auto offset = target.length();
                    target.resize(target.length() + end - start + to.length());
                    auto it = target.begin() + offset;
                    std::copy(underlying.begin() + start, underlying.begin() + end, it);
                    it += end - start;
                    std::copy(to.begin(), to.end(), it);
                    it += to.length();
                    start = end + from.length();
                    end = underlying.find(from, start);
                }
                if (start != underlying.length())
                {
                    auto offset = target.length();
                    target.resize(target.length() + underlying.length() - start);
                    std::copy(underlying.begin() + start, underlying.end(), target.begin() + offset);
                }
                return target;
            }
            template<typename T, typename U>
            string_exted replace(const T& from, const U& to) const
            {
                return replace(underlying_type(from), underlying_type(to));
            }
            operator underlying_type () noexcept
            {
                return underlying;
            }
            operator underlying_type& () noexcept
            {
                return underlying;
            }
            operator const underlying_type& () const noexcept
            {
                return underlying;
            }
            operator underlying_type && () noexcept
            {
                return std::move(underlying);
            }
        };

        template<typename C>
        string_exted<C> ext(const std::basic_string<C>& s)
        {
            return string_exted<C>(s);
        }
    }
}
