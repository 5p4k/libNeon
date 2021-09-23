//
// Created by spak on 9/23/21.
//

#include "neopixel_gamma.hpp"
#include "neopixel_color.hpp"
#include <cmath>
#include <algorithm>
#include <map>
#include <memory>
#include <mutex>

namespace neo {
    gamma_table build_gamma_table(float gamma) {
        gamma_table table{};
        for (std::size_t i = 0; i < table.size(); ++i) {
            const float linear_v = srgb_to_linear(std::uint8_t(i));
            table[i] = std::uint8_t(std::clamp(std::round(std::pow(linear_v, gamma)), 0.f, 255.f));
        }
        return table;
    }

    gamma_table const &lookup_gamma_table(float gamma) {
        static constexpr auto gamma_lookup_precision = 2;
        static std::map<signed, std::unique_ptr<gamma_table>> _lookup;
        static std::mutex _lookup_mutex;
        const std::lock_guard<std::mutex> lock(_lookup_mutex);
        const auto key = signed(std::round(gamma * std::pow(10, gamma_lookup_precision)));
        auto it = _lookup.lower_bound(key);
        if (it == std::end(_lookup) or it->first != key) {
            it = _lookup.emplace_hint(it, key, nullptr);
            it->second = std::make_unique<gamma_table>(build_gamma_table(gamma));
        }
        return *it->second;
    }
}