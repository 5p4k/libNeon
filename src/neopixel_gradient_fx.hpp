//
// Created by spak on 6/6/21.
//

#ifndef PICOSKATE_NEOPIXEL_GRADIENT_FX_HPP
#define PICOSKATE_NEOPIXEL_GRADIENT_FX_HPP

#include "neopixel_gradient.hpp"
#include "neopixel_strip.hpp"
#include <chrono>
#include <functional>
#include "mlab_unique_tracker.hpp"
#include "bin_data.hpp"

namespace neo {
    namespace {
        using namespace std::chrono_literals;
    }

    class gradient_fx : public mlab::uniquely_tracked {
        neo::gradient _gradient;
        std::chrono::milliseconds _duration;
        float _repeats;
    public:
        inline explicit gradient_fx(neo::gradient g, std::chrono::milliseconds duration = 1s, float repeats = 1.f);

        gradient_fx(gradient_fx &&g_fx) noexcept;
        gradient_fx &operator=(gradient_fx &&g_fx) noexcept;

        [[nodiscard]] inline neo::gradient const &gradient() const;
        inline void set_gradient(neo::gradient g);
        [[nodiscard]] inline float repeats() const;
        [[nodiscard]] inline std::chrono::milliseconds duration() const;

        inline void set_repeats(float n);
        inline void set_duration(std::chrono::milliseconds d);

        [[nodiscard]] std::vector<rgb> sample(std::size_t n_leds, std::chrono::milliseconds time_since_start, std::vector<rgb> recycle_buffer = {}, blending_method method = blend_linear) const;

        [[nodiscard]] std::function<void(std::chrono::milliseconds)> make_steady_timer_callback(transmittable_rgb_strip &strip, rmt_channel_t channel, blending_method method = blend_linear) const;
    };

    struct gradient_fx_config {
        neo::gradient gradient = {};
        float repeats = 1.f;
        std::uint32_t duration_ms = 1000;

        gradient_fx_config() = default;

        void apply(gradient_fx &g_fx) const;
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
        _gradient = std::move(g);
    }

    const neo::gradient & gradient_fx::gradient() const {
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
