//
// Created by spak on 5/2/23.
//

#ifndef LIBNEON_MATH_HPP
#define LIBNEON_MATH_HPP

#include <cmath>

namespace neo {
    [[nodiscard]] constexpr float modclamp(float f, float low = 0.f, float high = 1.f);
}

namespace neo {

    constexpr float modclamp(float f, float low, float high) {
        f = (f - low) / (high - low);
        return std::lerp(low, high, f - std::floor(f));
    }

}// namespace neo

#endif//LIBNEON_MATH_HPP
