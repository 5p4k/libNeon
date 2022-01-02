//
// Created by spak on 9/23/21.
//

#ifndef NEO_GAMMA_H
#define NEO_GAMMA_H

#include <array>
#include <map>
#include <memory>
#include <mutex>

namespace neo {
    struct gamma_table {
        using lut_table_t = std::array<std::uint8_t, std::numeric_limits<std::uint8_t>::max() + 1>;
        lut_table_t lut = {};

        gamma_table() = default;
        inline explicit gamma_table(lut_table_t lut_);

        [[nodiscard]] inline std::uint8_t operator[](std::uint8_t v) const;

        /**
         * @note This bisects @ref lut, so it's **very** slow.
         */
        [[nodiscard]] std::uint8_t reverse_lookup(std::uint8_t v) const;

        /**
         * Builds a lookup gamma table from sRGB values to the corresponding gamma-corrected intensity values.
         * The returned table implements essentially `gamma(srgb_to_linear(v))`.
         */
        [[nodiscard]] static gamma_table build(float gamma);

    };

    class gamma_table_cache {
        std::map<signed, std::unique_ptr<gamma_table>> _gamma_to_table;
        float _gamma_multiplier;
        std::mutex _lookup_mutex;

        [[nodiscard]] signed gamma_to_key(float gamma) const;
    public:
        static constexpr unsigned default_precision = 2;

        explicit gamma_table_cache(unsigned precision = default_precision);

        [[nodiscard]] gamma_table const &operator[](float gamma);
    };

    /**
     * Looks up a cached local copy of a gamma table for the given @p gamma. Lookup is done via a @ref std::map, so
     * it's logarithmic. Note that due to @p gamma being a floating point, the lookup is done by looking at at most
     * gamma_table_cache::default_precision fractional decimal digits, e.g. @p gamma = 2.200 and @p gamma = 2.201 yield
     * the same table.
     * @return A reference to the gamma table. It is safe to store such reference, it's guaranteed to stay alive as long
     *  as the program runs (it is stored in a `unique_ptr` inside a `static` storage duration map).
     */
    [[nodiscard]] gamma_table const &get_cached_gamma_table(float gamma);

}

namespace neo {
    gamma_table::gamma_table(lut_table_t lut_) : lut{lut_} {}

    std::uint8_t gamma_table::operator[](std::uint8_t v) const {
        return lut[v];
    }
}

#endif //NEO_GAMMA_H
