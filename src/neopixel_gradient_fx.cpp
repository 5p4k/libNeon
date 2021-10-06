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

    gradient_fx::gradient_fx(gradient_fx &&g_fx) noexcept : gradient_fx{neo::gradient{}} {
        *this = std::move(g_fx);
    }

    gradient_fx &gradient_fx::operator=(gradient_fx &&g_fx) noexcept {
        // Signal that a move operation is in place. This RAII object upon destruction will swap the content of
        // the corresponding trackers.
        auto hold_move = g_fx.swap(*this);
        std::swap(_gradient, g_fx._gradient);
        std::swap(_duration, g_fx._duration);
        std::swap(_repeats, g_fx._repeats);
        return *this;
    }


    std::function<void(std::chrono::milliseconds)> gradient_fx::make_steady_timer_callback(
            transmittable_rgb_strip &strip, rmt_channel_t channel, blending_method method) const
    {
        return [buffer = std::vector<rgb>{}, &strip, channel, method, tracker = tracker()]
            (std::chrono::milliseconds elapsed) mutable
        {
            // Do not capture this, to enable movement of the object
            if (auto *g_fx = mlab::uniquely_tracked::track<gradient_fx>(tracker); g_fx != nullptr) {
                // Use lambda initialization syntax and mutability to always recycle the buffer
                buffer = g_fx->sample(strip.size(), elapsed, std::move(buffer), method);
                ESP_ERROR_CHECK_WITHOUT_ABORT(strip.update(buffer, channel, false));
            } else {
                ESP_LOGE("NEO", "Unable to track gradient fx object.");
            }
        };
    }
}