//
// Created by spak on 9/23/21.
//

#ifndef NEO_GAMMA_H
#define NEO_GAMMA_H

#include <mlab/bin_data.hpp>
#include <cmath>

namespace neo {

    struct gamma_tag {};

    struct gamma : public mlab::tagged_array<gamma_tag, std::numeric_limits<std::uint8_t>::max() + 1> {
        using base = mlab::tagged_array<gamma_tag, std::numeric_limits<std::uint8_t>::max() + 1>;
        explicit constexpr gamma(float g) : base{} {
            constexpr const auto fmax = float(std::numeric_limits<std::uint8_t>::max());
            constexpr const auto fmin = float(std::numeric_limits<std::uint8_t>::min());
            for (std::uint8_t i = 0; i < std::uint8_t(size()); ++i) {
                (*this)[i] = std::uint8_t(std::clamp(std::round(fmax * std::pow(srgb_to_linear(i), g)), fmin, fmax));
            }
        }
    };

    template <float Gamma>
    static constexpr auto cached_gamma = gamma{Gamma};

}// namespace neo

#endif//NEO_GAMMA_H
