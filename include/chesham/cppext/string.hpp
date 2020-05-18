#pragma once
#include <algorithm>
#include <string>
#include <type_traits>
#include <limits>
#include <type_traits>

namespace chesham
{
    namespace cppext
    {
        template<typename T, typename U = char, typename V = std::remove_const<std::remove_reference<std::remove_pointer<std::remove_all_extents<T>::type>::type>::type>::type>
        struct to_string;

        template<typename T>
        struct to_string<T, wchar_t, wchar_t>
        {
            inline std::wstring operator()(const wchar_t* v) const
            {
                return std::wstring(v);
            }
        };

        template<>
        struct to_string<std::wstring, wchar_t, std::wstring>
        {
            inline const std::wstring& operator()(const std::wstring& v) const
            {
                return v;
            }
        };

        template<typename T, typename V>
        struct to_string<T, wchar_t, V>
        {
            inline std::wstring operator()(const V& v) const
            {
                return std::to_wstring(v);
            }
        };

        template<typename T>
        struct to_string<T, char, char>
        {
            inline std::string operator()(const char* v) const
            {
                return std::string(v);
            }
        };

        template<>
        struct to_string<std::string, char, std::string>
        {
            inline const std::string& operator()(const std::string& v) const
            {
                return v;
            }
        };

        template<typename T, typename V>
        struct to_string<T, char, V>
        {
            inline std::string operator()(const V& v) const
            {
                return std::to_string(v);
            }
        };

        template<typename C>
        class string_exted final
        {
            typedef std::basic_string<C> underlying_type;
            underlying_type underlying;
        public:
            string_exted(const underlying_type& v) : underlying(v) {}
            inline string_exted replace(const underlying_type& from, const underlying_type& to, std::size_t limit = std::numeric_limits<std::size_t>::max()) const
            {
                auto start = 0;
                auto end = underlying.find(from);
                underlying_type target;
                while (end != underlying_type::npos && limit--)
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
            inline string_exted replace(const T& from, const U& to, std::size_t limit = std::numeric_limits<std::size_t>::max()) const
            {
                return replace(to_string<T, C>{}(from), to_string<U, C>{}(to), limit);
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
