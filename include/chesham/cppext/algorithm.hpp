#pragma once
#include <iterator>
#include <type_traits>

namespace chesham
{
    namespace cppext
    {
        template<typename It>
        bool sequence_equal(It first1, It end1, It first2, It end2)
        {
            while (first1 != end1 && first2 != end2)
            {
                if (*first1++ != *first2++)
                    return false;
            }
            if (first1 != end1 || first2 != end2)
                return false;
            return true;
        }
    }
}
