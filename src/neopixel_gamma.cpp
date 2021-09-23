//
// Created by spak on 9/23/21.
//

#include "neopixel_gamma.hpp"
#include "neopixel_color.hpp"
#include <cmath>
#include <algorithm>
#include <mutex>

namespace neo {

    gamma_table gamma_table::build(float gamma) {
        lut_table_t table{};
        for (std::size_t i = 0; i < table.size(); ++i) {
            const float linear_v = srgb_to_linear(std::uint8_t(i));
            table[i] = std::uint8_t(std::clamp(std::round(std::pow(linear_v, gamma)), 0.f, 255.f));
        }
        return gamma_table{table};
    }

    gamma_table_cache::gamma_table_cache(unsigned precision) :
        _gamma_to_table{},
        _gamma_multiplier{std::pow(10.f, float(precision))},
        _lookup_mutex{}
    {}

    gamma_table const &gamma_table_cache::operator[](float gamma) {
        const auto key = gamma_to_key(gamma);
        const std::lock_guard<std::mutex> lock(_lookup_mutex);
        auto it = _gamma_to_table.lower_bound(key);
        if (it == std::end(_gamma_to_table) or it->first != key) {
            it = _gamma_to_table.emplace_hint(it, key, nullptr);
            it->second = std::make_unique<gamma_table>(gamma_table::build(gamma));
        }
        return *it->second;
    }

    signed gamma_table_cache::gamma_to_key(float gamma) const {
        return signed(std::round(gamma * _gamma_multiplier));
    }

    std::uint8_t gamma_table::reverse_lookup(std::uint8_t v) const {
        return std::uint8_t(std::lower_bound(std::begin(lut), std::end(lut), v) - std::begin(lut));
    }

    gamma_table const &get_cached_gamma_table(float gamma) {
        static gamma_table_cache _cache{};
        return _cache[gamma];
    }
}