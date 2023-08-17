//
// Created by spak on 6/5/21.
//

#include <cassert>
#include <cmath>
#include <iomanip>
#include <neo/gradient.hpp>
#include <neo/math.hpp>

namespace neo {

    std::vector<srgb> gradient_sample_normalized_sorted(std::vector<std::pair<float, srgb>> const &gradient_normalized_sorted, std::size_t count, blend_fn_t blend_fn) {
        // Alias for readability
        auto &g = gradient_normalized_sorted;
        if (g.empty() or count == 0) {
            return {};
        }
        if (g.size() == 1) {
            return {g.front().second};
        }
        std::vector<srgb> retval;
        retval.reserve(count);

        auto lb = std::begin(g);
        auto ub = std::next(lb);
        auto less = positioned_compare{};
        for (std::size_t i = 0; i < count; ++i) {
            const float t = float(i) / float(count - 1);
            while (ub != std::end(g) and not less(t, ub->first)) {
                lb = ub++;
            }
            if (ub == std::end(g)) {
                retval.emplace_back(lb->second);
            } else {
                assert(less(t, ub->first) and not less(t, lb->first));
                const float blend_factor = (t - lb->first) / (ub->first - lb->first);
                retval.emplace_back(blend_fn(lb->second, ub->second, blend_factor));
            }
        }
        return retval;
    }



}// namespace mlab