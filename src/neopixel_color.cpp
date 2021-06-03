//
// Created by spak on 6/4/21.
//

#include "neopixel_color.hpp"
#include <cmath>
#include <algorithm>

namespace neo {

    static constexpr float pi_thirds = M_PI / 3.f;
    static constexpr float two_pi = M_2_PI;

    namespace {
        std::uint8_t f2b(float f) {
            return std::uint8_t(std::clamp(f, 0.f, 1.f) * 255);
        }
        std::array<std::uint8_t, 3> f2b(std::array<float, 3> f) {
            return {f2b(f[0]), f2b(f[1]), f2b(f[2])};
        }
    }

    std::array<float, 3> rgb_to_hsv(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
        const std::uint8_t value = std::max(r, std::max(g, b));
        const std::uint8_t min_c = std::min(r, std::min(g, b));
        const std::uint8_t chroma = value - min_c;

        const float hue = [&]() -> float {
            if (chroma == 0) {
                return 0.f;
            } else if (value == r) {
                return pi_thirds * float(g - b) / float(chroma);
            } else if (value == g) {
                return pi_thirds * (2.f + float(g - b) / float(chroma));
            } else if (value == b) {
                return pi_thirds * (4.f + float(g - b) / float(chroma));
            }
            return 0.f;
        }();

        const float saturation = (value == 0 ? 0.f : float(chroma) / float(value));

        return {hue, saturation, float(value) / 255.f};
    }


    std::array<std::uint8_t, 3> hsv_to_rgb(float h, float s, float v) {
        s = std::clamp(s, 0.f, 1.f);
        v = std::clamp(v, 0.f, 1.f);
        if (s < std::numeric_limits<float>::epsilon()) {
            const std::uint8_t gray = f2b(v);
            return {gray, gray, gray};
        }
        // Remap in 0...6
        h = std::fmod(h, two_pi) / pi_thirds;
        const auto hue_block = unsigned(std::floor(h));
        const float m = v * (1.f - s);
        const float x = v * (1.f - s * (hue_block % 2 == 0 ? 1.f - h + float(hue_block) : h - float(hue_block)));
        switch (hue_block) {
            case 0:
                return f2b({v, x, m});
            case 1:
                return f2b({x, v, m});
            case 2:
                return f2b({m, v, x});
            case 3:
                return f2b({m, x, v});
            case 4:
                return f2b({x, m, v});
            default:
                return f2b({v, m, x});
        }
    }
}
