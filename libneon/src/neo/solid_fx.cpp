//
// Created by spak on 10/6/21.
//

#include <mlab/mutex.hpp>
#include <neo/solid_fx.hpp>

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

    void solid_fx::render_frame(transmittable_rgb_strip &strip, rmt_channel_t channel) const {
        // Quickly try to lock, if fails, just drop frame
        mlab::scoped_try_lock lock{_color_mutex};
        if (lock) {
            ESP_ERROR_CHECK_WITHOUT_ABORT(strip.update(color(), channel, false));
        } else {
            ESP_LOGW("NEO", "Dropping frame due to locked solid fx operation");
        }
    }

    std::function<void(std::chrono::milliseconds)> solid_fx::make_steady_timer_callback(
            transmittable_rgb_strip &strip, rmt_channel_t channel) const {
        return [&strip, channel, tracker = tracker()](std::chrono::milliseconds) {
            // Do not capture this, to enable movement of the object
            if (auto *s_fx = mlab::uniquely_tracked::track<solid_fx>(tracker); s_fx != nullptr) {
                s_fx->render_frame(strip, channel);
            } else {
                ESP_LOGE("NEO", "Unable to track solid fx object.");
            }
        };
    }

    std::string solid_fx_config::to_string() const {
        std::string buffer;
        const auto color_str = color.to_string();
        auto attempt_snprintf = [&](std::string *buffer) -> std::size_t {
            return std::snprintf(buffer != nullptr ? buffer->data() : nullptr,
                                 buffer != nullptr ? buffer->size() : 0,
                                 "solid, %s",
                                 color_str.c_str());
        };
        buffer.clear();
        buffer.resize(attempt_snprintf(nullptr) + 1 /* null terminator */, '\0');
        attempt_snprintf(&buffer);
        buffer.resize(buffer.size() - 1);// Remove null terminator
        return buffer;
    }
}// namespace neo

namespace mlab {
    mlab::bin_stream &operator>>(mlab::bin_stream &s, neo::solid_fx_config &s_fx_cfg) {
        std::uint8_t r = 0, g = 0, b = 0;
        s >> r >> g >> b;
        if (not s.bad()) {
            s_fx_cfg.color = neo::rgb{r, g, b};
        }
        return s;
    }
}// namespace mlab