//
// Created by spak on 10/6/21.
//

#include "neopixel_solid_fx.hpp"
#include "mlab_mutex.hpp"

namespace neo {


    solid_fx::solid_fx(solid_fx &&other) noexcept {
        *this = std::move(other);
    }

    solid_fx &solid_fx::operator=(solid_fx &&other) noexcept {
        std::scoped_lock lock{_color_mutex, other._color_mutex};
        auto swap_hold = other.swap(*this);
        std::swap(_color, other._color);
        return *this;
    }

    std::function<void(std::chrono::milliseconds)> solid_fx::make_steady_timer_callback(
            transmittable_rgb_strip &strip, rmt_channel_t channel) const {
        return [&strip, channel, tracker = tracker()](std::chrono::milliseconds) {
            // Do not capture this, to enable movement of the object
            if (auto *s_fx = mlab::uniquely_tracked::track<solid_fx>(tracker); s_fx != nullptr) {
                // Quickly try to lock, if fails, just drop frame
                mlab::scoped_try_lock lock{s_fx->_color_mutex};
                if (lock) {
                    ESP_ERROR_CHECK_WITHOUT_ABORT(strip.update(s_fx->color(), channel, false));
                } else {
                    ESP_LOGW("NEO", "Dropping frame due to locked solid fx operation");
                }
            } else {
                ESP_LOGE("NEO", "Unable to track solid fx object.");
            }
        };
    }

}

namespace mlab {
    mlab::bin_stream &operator>>(mlab::bin_stream &s, neo::solid_fx_config &s_fx_cfg) {
        std::uint8_t r = 0, g = 0, b = 0;
        s >> r >> g >> b;
        if (not s.bad()) {
            s_fx_cfg.color = neo::rgb{r, g, b};
        }
        return s;
    }
}