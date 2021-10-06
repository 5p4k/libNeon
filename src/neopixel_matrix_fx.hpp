//
// Created by spak on 10/6/21.
//

#ifndef PICOSKATE_NEOPIXEL_MATRIX_FX_HPP
#define PICOSKATE_NEOPIXEL_MATRIX_FX_HPP

#include "neopixel_strip.hpp"
#include "neopixel_gradient.hpp"
#include "mlab_unique_tracker.hpp"
#include "bin_data.hpp"
#include <chrono>
#include <functional>
#include <mutex>

namespace neo {
    namespace {
        using namespace std::chrono_literals;
    }

    class matrix_fx : public mlab::uniquely_tracked {
        std::vector<neo::rgb> _matrix = {};
        std::size_t _width = 0;
        std::size_t _height = 0;
        std::chrono::milliseconds _duration_x = 0ms;
        std::chrono::milliseconds _duration_y = 0ms;
        float _repeats_x = 1.f;
        std::recursive_mutex _matrix_mutex = {};
    public:
        matrix_fx() = default;

        matrix_fx(std::vector<neo::rgb> matrix, std::size_t width, std::chrono::milliseconds duration_x = 1s,
                  std::chrono::milliseconds duration_y = 1s, float repeats_x = 1.f);

        matrix_fx(matrix_fx &&other) noexcept;

        matrix_fx &operator=(matrix_fx &&other) noexcept;

        [[nodiscard]] inline float repeats_x() const;

        [[nodiscard]] inline std::size_t width() const;

        [[nodiscard]] inline std::size_t height() const;

        [[nodiscard]] inline std::chrono::milliseconds duration_x() const;

        [[nodiscard]] inline std::chrono::milliseconds duration_y() const;

        inline void set_repeats_x(float n);

        inline void set_duration_x(std::chrono::milliseconds d);

        inline void set_duration_y(std::chrono::milliseconds d);

        [[nodiscard]] inline std::vector<rgb> const &matrix() const;

        void set_matrix(std::vector<rgb> m, std::size_t w);

        void resample(std::size_t new_width, std::size_t new_height, blending_method method = blend_linear);

        void tile(float overlap_x, float overlap_y);

        [[nodiscard]] rgb sample(float x, float y, blending_method method = blend_linear) const;

        [[nodiscard]] std::vector<rgb>
        sample(std::size_t n_leds, std::chrono::milliseconds time_since_start, std::vector<rgb> recycle_buffer = {},
               blending_method method = blend_linear) const;

        [[nodiscard]] std::function<void(std::chrono::milliseconds)>
        make_steady_timer_callback(transmittable_rgb_strip &strip, rmt_channel_t channel,
                                   blending_method method = blend_linear) const;
    };

    struct matrix_fx_config {
        std::vector<neo::rgb> matrix = {};
        std::uint8_t width = 0;
        float repeats_x = 1.f;
        std::uint32_t duration_x_ms = 0;
        std::uint32_t duration_y_ms = 5000;


        matrix_fx_config() = default;

        void apply(matrix_fx &m_fx, float tile_x = 0.1f, float tile_y = 0.1f) const;
    };

}

namespace mlab {
    mlab::bin_stream &operator>>(mlab::bin_stream &s, neo::matrix_fx_config &m_fx_cfg);
}

namespace neo {

    std::chrono::milliseconds matrix_fx::duration_y() const {
        return _duration_y;
    }

    std::chrono::milliseconds matrix_fx::duration_x() const {
        return _duration_x;
    }

    float matrix_fx::repeats_x() const {
        return _repeats_x;
    }

    std::size_t matrix_fx::width() const {
        std::scoped_lock lock{_matrix_mutex};
        return _width;
    }

    std::size_t matrix_fx::height() const {
        std::scoped_lock lock{_matrix_mutex};
        return _height;
    }

    void matrix_fx::set_duration_x(std::chrono::milliseconds d) {
        _duration_x = d;
    }

    void matrix_fx::set_duration_y(std::chrono::milliseconds d) {
        _duration_y = d;
    }

    void matrix_fx::set_repeats_x(float n) {
        _repeats_x = n;
    }

    std::vector<rgb> const &matrix_fx::matrix() const {
        std::scoped_lock lock{_matrix_mutex};
        return _matrix;
    }
}

#endif //PICOSKATE_NEOPIXEL_MATRIX_FX_HPP
