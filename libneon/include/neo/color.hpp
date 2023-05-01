//
// Created by spak on 6/4/21.
//

#ifndef NEO_COLOR_HPP
#define NEO_COLOR_HPP

#include <algorithm>
#include <array>
#include <cstdint>
#include <neo/channel.hpp>
#include <mlab/bin_data.hpp>

namespace neo {
    struct srgb;
    struct hsv;

    namespace literals {
        constexpr srgb operator""_rgb(unsigned long long int);
    }

    /**
     * The values are expressed in sRGB color space.
     */
    struct srgb {
        std::uint8_t r = 0;
        std::uint8_t g = 0;
        std::uint8_t b = 0;

        constexpr srgb() = default;

        explicit constexpr srgb(std::uint32_t rgb_);

        constexpr srgb(std::uint8_t r_, std::uint8_t g_, std::uint8_t b_);

        [[nodiscard]] srgb blend(srgb target, float factor) const;
        [[nodiscard]] srgb lerp(srgb target, float factor) const;

        [[nodiscard]] hsv to_hsv() const;


        [[nodiscard]] static float to_linear(std::uint8_t v);
        [[nodiscard]] static std::uint8_t from_linear(float v);

        [[nodiscard]] static std::array<float, 3> to_linear(srgb const &rgb);
        [[nodiscard]] static srgb from_linear(std::array<float, 3> const &linear_rgb);

        [[nodiscard]] std::string to_string() const;

        [[nodiscard]] constexpr std::uint8_t operator[](channel c) const;
    };

    struct hsv {
        float h = 0.f;
        float s = 0.f;
        float v = 0.f;

        constexpr hsv() = default;

        constexpr hsv(float h_, float s_, float v_);

        [[nodiscard]] hsv clamped() const;

        [[nodiscard]] srgb to_rgb() const;
    };

}// namespace neo

namespace neo {

    constexpr srgb literals::operator""_rgb(unsigned long long int c) {
        return srgb{std::uint32_t(c)};
    }

    constexpr srgb::srgb(std::uint8_t r_, std::uint8_t g_, std::uint8_t b_)
        : r{r_}, g{g_}, b{b_} {}

    constexpr srgb::srgb(std::uint32_t rgb_)
        : r{std::uint8_t(0xff & (rgb_ >> 16))},
          g{std::uint8_t(0xff & (rgb_ >> 8))},
          b{std::uint8_t(0xff & rgb_)} {}

    constexpr std::uint8_t srgb::operator[](channel c) const {
        switch (c) {
            case channel::r: return r;
            case channel::g: return g;
            case channel::b: return b;
        }
        return 0;
    }

    constexpr hsv::hsv(float h_, float s_, float v_) : h{h_}, s{s_}, v{v_} {}
}// namespace neo

namespace mlab {
    bin_data &operator<<(bin_data &o, neo::srgb c);
    bin_stream &operator>>(bin_stream &i, neo::srgb &c);
}// namespace mlab

#endif//NEO_COLOR_HPP
