//
// Created by spak on 6/6/21.
//

#include "neopixel_gradient_fx.hpp"
#include <cmath>

namespace neo {
    std::vector<rgb> gradient_fx::sample(std::size_t n_leds, std::chrono::milliseconds time_since_start,
                                         std::vector<rgb> recycle_buffer, blending_method method) const {
        recycle_buffer.resize(n_leds);
        // Get the fractional cycle time at the first led
        const float t0 = float(time_since_start.count() % duration().count()) / float(duration().count());
        // Compute the time increment for each led
        const float dt = repeats() / float(n_leds);

        for (std::size_t i = 0; i < n_leds; ++i) {
            auto &color = recycle_buffer[i];
            // Compute the correct time for this led
            float t = t0 + float(i) * dt;
            // Make sure it's in 0..1 range, handle also negative dts correctly
            t -= std::floor(t);
            assert(-std::numeric_limits<float>::epsilon() < t and t < std::nextafter(1.f, 2.f));
            color = gradient().sample(t, method);
        }

        return recycle_buffer;
    }
}