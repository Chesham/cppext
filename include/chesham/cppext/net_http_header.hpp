#pragma once
#include <unordered_map>
#include "functional.hpp"

namespace chesham
{
    namespace cppext
    {
        namespace net
        {
            using headers_type = std::unordered_map<std::string, std::string, string_no_case_comparer<std::string>, string_no_case_comparer<std::string>>;

            enum HTTP_VERSION
            {
                V1_0,
                V1_1,
                V2_0,
            };
            class http_traits
            {
            public:
                virtual ~http_traits() { }
                std::string to_string(const HTTP_VERSION& version) const
                {
                    switch (version)
                    {
                    case HTTP_VERSION::V1_0:
                        return "1.0";
                    case HTTP_VERSION::V1_1:
                        return "1.1";
                    case HTTP_VERSION::V2_0:
                        return "2.0";
                    default:
                        throw std::runtime_error("not supported http version");
                    }
                }
                bool autoDecompression{ true };
                HTTP_VERSION httpVersion{ HTTP_VERSION::V1_0 };
            };
        }
    }
}