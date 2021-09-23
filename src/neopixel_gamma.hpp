//
// Created by spak on 9/23/21.
//

#ifndef PICOSKATE_NEOPIXEL_GAMMA_H
#define PICOSKATE_NEOPIXEL_GAMMA_H

#include <array>

namespace neo {
    using gamma_table = std::array<std::uint8_t, std::numeric_limits<std::uint8_t>::max() + 1>;

    /**
     * Builds a lookup gamma table from sRGB values to the corresponding gamma-corrected intensity values.
     * The returned table implements essentially `gamma(srgb_to_linear(v))`.
     */
    gamma_table build_gamma_table(float gamma);

    /**
     * Looks up a cached local copy of a gamma table for the given @p gamma. Lookup is done via a @ref std::map, so
     * it's logarithmic. Note that due to @p gamma being a floating point, the lookup is done by looking at at most
     * two fractional decimal digits, e.g. @p gamma = 2.200 and @p gamma = 2.201 yield the same table.
     * @return A reference to the gamma table. It is safe to store such reference, it's guaranteed to stay alive as long
     *  as the program runs (it is stored in a `unique_ptr` inside a `static` storage duration map).
     */
    gamma_table const &lookup_gamma_table(float gamma);
}

#endif //PICOSKATE_NEOPIXEL_GAMMA_H
