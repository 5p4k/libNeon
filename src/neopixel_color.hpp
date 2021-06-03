//
// Created by spak on 6/4/21.
//

#ifndef PICOSKATE_NEOPIXEL_COLOR_HPP
#define PICOSKATE_NEOPIXEL_COLOR_HPP

#include <cstdint>
#include <array>

namespace neo {

    /**
     * @return Hue, saturation, value in range [0, 2PI], [0, 1], [0, 1].
     */
    std::array<float, 3> rgb_to_hsv(std::uint8_t r, std::uint8_t g, std::uint8_t b);

    std::array<std::uint8_t, 3> hsv_to_rgb(float h, float s, float v);

}

#endif //PICOSKATE_NEOPIXEL_COLOR_HPP
