//
// Created by spak on 6/6/21.
//

#include "neopixel_gradient_fx.hpp"
#include "skate_data.hpp"
#include "mlab_mutex.hpp"
#include "mlab_float_io.hpp"
#include <cmath>

namespace neo {

    std::vector<rgb> gradient_fx::sample(std::size_t n_leds, std::chrono::milliseconds time_since_start,
                                         std::vector<rgb> recycle_buffer, blending_method method) const {
        recycle_buffer.resize(n_leds);
        // Get the fractional cycle time at the first led
        const float t0 = float(time_since_start.count() % duration().count()) / float(duration().count());
        // Compute the time increment for each led
        const float dt = repeats() / float(n_leds);

        std::scoped_lock lock{_gradient_mutex};
        return _gradient.sample_uniform(dt, t0, n_leds, std::move(recycle_buffer), method);
    }

    gradient_fx::gradient_fx(gradient_fx &&g_fx) noexcept : gradient_fx{neo::gradient{}} {
        *this = std::move(g_fx);
    }

    gradient_fx &gradient_fx::operator=(gradient_fx &&g_fx) noexcept {
        std::scoped_lock lock{_gradient_mutex, g_fx._gradient_mutex};
        // Signal that a move operation is in place. This RAII object upon destruction will swap the content of
        // the corresponding trackers.
        auto hold_move = g_fx.swap(*this);
        std::swap(_gradient, g_fx._gradient);
        std::swap(_duration, g_fx._duration);
        std::swap(_repeats, g_fx._repeats);
        return *this;
    }


    std::vector<rgb> gradient_fx::render_frame(transmittable_rgb_strip &strip, rmt_channel_t channel, std::chrono::milliseconds elapsed,
                                  std::vector<rgb> recycle_buffer, blending_method method) const
    {
        // Quickly try to lock, if fails, just drop frame
        mlab::scoped_try_lock lock{_gradient_mutex};
        if (lock) {
            // Use lambda initialization syntax and mutability to always recycle the buffer
            recycle_buffer = sample(strip.size(), elapsed, std::move(recycle_buffer), method);
            ESP_ERROR_CHECK_WITHOUT_ABORT(strip.update(recycle_buffer, channel, false));
        } else {
            ESP_LOGW("NEO", "Dropping frame due to locked gradient fx operation");
        }
        return recycle_buffer;
    }

    std::function<void(std::chrono::milliseconds)> gradient_fx::make_steady_timer_callback(
            transmittable_rgb_strip &strip, rmt_channel_t channel, blending_method method) const
    {
        return [buffer = std::vector<rgb>{}, &strip, channel, method, tracker = tracker()]
            (std::chrono::milliseconds elapsed) mutable
        {
            // Do not capture this, to enable movement of the object
            if (auto *g_fx = mlab::uniquely_tracked::track<gradient_fx>(tracker); g_fx != nullptr) {
                buffer = g_fx->render_frame(strip, channel, elapsed, std::move(buffer), method);
            } else {
                ESP_LOGE("NEO", "Unable to track gradient fx object.");
            }
        };
    }

    void gradient_fx_config::apply(gradient_fx &g_fx) const {
        g_fx.set_gradient(gradient);
        g_fx.set_repeats(repeats);
        g_fx.set_duration(std::chrono::milliseconds{duration_ms});
    }

    std::string gradient_fx_config::to_string() const {
        std::string buffer;
        const auto gradient_desc = this->gradient.to_string();
        auto attempt_snprintf = [&](std::string *buffer) -> std::size_t {
            return std::snprintf(buffer != nullptr ? buffer->data() : nullptr,
                                 buffer != nullptr ? buffer->size() : 0,
                                 "gradient, %01.1f×, %d ms, %s",
                                 repeats, duration_ms, gradient_desc.c_str());
        };
        buffer.clear();
        buffer.resize(attempt_snprintf(nullptr) + 1 /* null terminator */, '\0');
        attempt_snprintf(&buffer);
        buffer.resize(buffer.size() - 1); // Remove null terminator
        return buffer;
    }
}

namespace mlab {

    mlab::bin_stream &operator>>(mlab::bin_stream &s, neo::gradient_fx_config &g_fx_cfg) {
        return s >> g_fx_cfg.gradient >> float_lsb >> g_fx_cfg.repeats >> lsb32 >> g_fx_cfg.duration_ms;
    }
}