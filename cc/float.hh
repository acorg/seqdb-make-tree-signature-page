#pragma once

#include <cmath>
#include <limits>
#include <type_traits>
#include <algorithm>

// ----------------------------------------------------------------------

// http://en.cppreference.com/w/cpp/types/numeric_limits/epsilon
template<typename T> typename std::enable_if<!std::numeric_limits<T>::is_integer, bool>::type
inline float_equal(T x, T y, int ulp=1)
{
    // the machine epsilon has to be scaled to the magnitude of the values used
    // and multiplied by the desired precision in ULPs (units in the last place)
    return std::abs(x-y) < std::numeric_limits<T>::epsilon() * std::abs(x+y) * ulp
    // unless the result is subnormal
                           || std::abs(x-y) < std::numeric_limits<T>::min();
}

// ----------------------------------------------------------------------

template<typename T> typename std::enable_if<!std::numeric_limits<T>::is_integer, bool>::type
inline float_zero(T x, int ulp=1)
{
    return float_equal(x, T(0), ulp);
}

// ----------------------------------------------------------------------
