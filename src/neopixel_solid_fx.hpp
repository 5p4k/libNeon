//
// Created by spak on 10/6/21.
//

#ifndef PICOSKATE_NEOPIXEL_SOLID_FX_HPP
#define PICOSKATE_NEOPIXEL_SOLID_FX_HPP

#include "mlab_unique_tracker.hpp"
#include "neopixel_strip.hpp"
#include <mutex>
#include "bin_data.hpp"

namespace neo {

    class solid_fx : public mlab::uniquely_tracked {
        neo::rgb _color;
        mutable std::recursive_mutex _color_mutex;
    public:
        inline explicit solid_fx(neo::rgb color = neo::rgb{0, 0, 0});

        solid_fx(solid_fx &&other) noexcept;

        solid_fx &operator=(solid_fx &&other) noexcept;

        [[nodiscard]] inline rgb color() const;

        inline void set_color(rgb c);

        [[nodiscard]] std::function<void(std::chrono::milliseconds)>
        make_steady_timer_callback(transmittable_rgb_strip &strip, rmt_channel_t channel) const;

        void render_frame(transmittable_rgb_strip &strip, rmt_channel_t channel) const;
    };


    struct solid_fx_config {
        neo::rgb color = neo::rgb{0, 0, 0};

        solid_fx_config() = default;

        inline void apply(solid_fx &s_fx) const;
    };
}


namespace mlab {
    mlab::bin_stream &operator>>(mlab::bin_stream &s, neo::solid_fx_config &m_fx_cfg);
}


namespace neo {

    solid_fx::solid_fx(neo::rgb color) : _color{color} {}

    neo::rgb solid_fx::color() const {
        std::scoped_lock lock{_color_mutex};
        return _color;
    }

    void solid_fx::set_color(neo::rgb c) {
        std::scoped_lock lock{_color_mutex};
        _color = c;
    }

    void solid_fx_config::apply(solid_fx &s_fx) const {
        s_fx.set_color(color);
    }
}

#endif //PICOSKATE_NEOPIXEL_SOLID_FX_HPP