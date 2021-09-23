//
// Created by spak on 9/23/21.
//

#ifndef PICOSKATE_NEOPIXEL_GAMMA_H
#define PICOSKATE_NEOPIXEL_GAMMA_H

#include <array>

namespace neo {
    using gamma_table = std::array<std::uint8_t, 0x100>;

    gamma_table build_gamma_table(float gamma = 2.8);

    gamma_table const &lookup_gamma_table(float gamma = 2.8);
}

#endif //PICOSKATE_NEOPIXEL_GAMMA_H
