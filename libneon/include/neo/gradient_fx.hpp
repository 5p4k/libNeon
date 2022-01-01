//
// Created by spak on 6/6/21.
//

#ifndef PICOSKATE_NEOPIXEL_GRADIENT_FX_HPP
#define PICOSKATE_NEOPIXEL_GRADIENT_FX_HPP

#include <neo/gradient.hpp>
#include <neo/strip.hpp>
#include <chrono>
#include <functional>
#include <mlab/unique_tracker.hpp>
#include <mlab/bin_data.hpp>

namespace neo {
    namespace {
        using namespace std::chrono_literals;
    }

    class gradient_fx : public mlab::uniquely_tracked {
        neo::gradient _gradient = {};
        std::chrono::milliseconds _duration = 0ms;
        float _repeats = 1.f;
        mutable std::recursive_mutex _gradient_mutex = {};
    public:
        gradient_fx() = default;

        inline explicit gradient_fx(neo::gradient g, std::chrono::milliseconds duration = 1s, float repeats = 1.f);

        gradient_fx(gradient_fx &&g_fx) noexcept;
        gradient_fx &operator=(gradient_fx &&g_fx) noexcept;

        [[nodiscard]] inline neo::gradient get_gradient() const;
        inline void set_gradient(neo::gradient g);
        [[nodiscard]] inline float repeats() const;
        [[nodiscard]] inline std::chrono::milliseconds duration() const;

        inline void set_repeats(float n);
        inline void set_duration(std::chrono::milliseconds d);

        [[nodiscard]] std::vector<rgb> sample(std::size_t n_leds, std::chrono::milliseconds time_since_start, std::vector<rgb> recycle_buffer = {}, blending_method method = blend_linear) const;

        [[nodiscard]] std::function<void(std::chrono::milliseconds)> make_steady_timer_callback(
                transmittable_rgb_strip &strip, rmt_channel_t channel, blending_method method = blend_linear) const;

        std::vector<rgb> render_frame(transmittable_rgb_strip &strip, rmt_channel_t channel,
                                      std::chrono::milliseconds elapsed, std::vector<rgb> recycle_buffer = {},
                                      blending_method method = blend_linear) const;
    };

    struct gradient_fx_config {
        neo::gradient gradient = {};
        float repeats = 1.f;
        std::uint32_t duration_ms = 1000;

        gradient_fx_config() = default;

        void apply(gradient_fx &g_fx) const;

        [[nodiscard]] std::string to_string() const;
    };
}

namespace mlab {
    mlab::bin_stream &operator>>(mlab::bin_stream &s, neo::gradient_fx_config &g_fx_cfg);
}
namespace neo {
    gradient_fx::gradient_fx(neo::gradient g, std::chrono::milliseconds duration, float repeats) :
        _gradient{std::move(g)},
        _duration{duration},
        _repeats{repeats}
    {}

    std::chrono::milliseconds gradient_fx::duration() const {
        return _duration;
    }

    float gradient_fx::repeats() const {
        return _repeats;
    }

    void gradient_fx::set_gradient(neo::gradient g) {
        std::scoped_lock lock{_gradient_mutex};
        _gradient = std::move(g);
    }

    neo::gradient gradient_fx::get_gradient() const {
        std::scoped_lock lock{_gradient_mutex};
        return _gradient;
    }

    void gradient_fx::set_duration(std::chrono::milliseconds d) {
        _duration = d;
    }

    void gradient_fx::set_repeats(float n) {
        _repeats = n;
    }

}

#endif //PICOSKATE_NEOPIXEL_GRADIENT_FX_HPP
