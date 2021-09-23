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

        return gradient().sample_uniform(dt, t0, n_leds, std::move(recycle_buffer), method);
    }

    std::function<void(std::chrono::milliseconds)> gradient_fx::make_steady_timer_callback(
            transmittable_rgb_strip &strip, rmt_channel_t channel, blending_method method) const
    {
        return [buffer = std::vector<rgb>{}, &strip, channel, method, *this] (std::chrono::milliseconds elapsed) mutable {
            // Use lambda initialization syntax and mutability to always recycle the buffer
            buffer = sample(strip.size(), elapsed, std::move(buffer), method);
            ESP_ERROR_CHECK_WITHOUT_ABORT(strip.update(buffer, channel, false));
        };
    }
}