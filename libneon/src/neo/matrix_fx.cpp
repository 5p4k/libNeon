//
// Created by spak on 10/6/21.
//

#include <algorithm>
#include <mlab/mutex.hpp>
#include <neo/matrix_fx.hpp>

namespace neo {

    matrix_fx::matrix_fx(std::vector<neo::rgb> matrix, std::size_t width, std::chrono::milliseconds duration_x,
                         std::chrono::milliseconds duration_y, float repeats_x)
        : _width{0},
          _height{0},
          _duration_x{duration_x},
          _duration_y{duration_y},
          _repeats_x{repeats_x} {
        set_matrix(std::move(matrix), width);
    }

    matrix_fx::matrix_fx(matrix_fx &&other) noexcept : matrix_fx{} {
        *this = std::move(other);
    }

    matrix_fx &matrix_fx::operator=(matrix_fx &&other) noexcept {
        std::scoped_lock lock{_matrix_mutex, other._matrix_mutex};
        auto swap_hold = other.swap(*this);
        std::swap(_matrix, other._matrix);
        std::swap(_width, other._width);
        std::swap(_height, other._height);
        std::swap(_duration_x, other._duration_x);
        std::swap(_duration_y, other._duration_y);
        std::swap(_repeats_x, other._repeats_x);
        return *this;
    }

    rgb matrix_fx::sample(float x, float y, blending_method method) const {
        std::scoped_lock lock{_matrix_mutex};
        if (not std::isfinite(x) or not std::isfinite(y) or width() == 0 or height() == 0) {
            return neo::rgb{0, 0, 0};
        }
        // Make sure it's in 0..1 range, handle also negative dts correctly
        x -= std::floor(x);
        y -= std::floor(y);
        assert(-std::numeric_limits<float>::epsilon() < x and x < std::nextafter(1.f, 2.f));
        assert(-std::numeric_limits<float>::epsilon() < y and y < std::nextafter(1.f, 2.f));
        // Remap to width/height
        x *= float(width() - 1);
        y *= float(height() - 1);
        const std::size_t xidx_low = std::clamp(std::size_t(std::floor(x)), std::size_t(0), width() - 1);
        const std::size_t xidx_high = std::clamp(std::size_t(std::ceil(x)), std::size_t(0), width() - 1);
        const std::size_t yidx_low = std::clamp(std::size_t(std::floor(y)), std::size_t(0), height() - 1);
        const std::size_t yidx_high = std::clamp(std::size_t(std::ceil(y)), std::size_t(0), height() - 1);
        // Get blending factor
        const float dx = std::clamp(x - float(xidx_low), 0.f, 1.f);
        const float dy = std::clamp(y - float(yidx_low), 0.f, 1.f);
        // Compute color indices
        const std::size_t llc_idx = width() * yidx_low + xidx_low;
        const std::size_t lrc_idx = width() * yidx_low + xidx_high;
        const std::size_t ulc_idx = width() * yidx_high + xidx_low;
        const std::size_t urc_idx = width() * yidx_high + xidx_high;
        // Blend twice
        return method(
                method(_matrix.at(llc_idx), _matrix.at(lrc_idx), dx),
                method(_matrix.at(ulc_idx), _matrix.at(urc_idx), dx),
                dy);
    }

    void matrix_fx::set_matrix(std::vector<rgb> m, std::size_t w) {
        if (m.size() % w != 0) {
            ESP_LOGW("NEO", "Matrix effect size %d cannot be expressed as W x H, %d x %d.",
                     m.size(), w, height());
        }
        std::scoped_lock lock{_matrix_mutex};
        _width = w;
        _height = w > 0 ? m.size() / w : 0;
        std::swap(_matrix, m);
    }

    void matrix_fx::resample(std::size_t new_width, std::size_t new_height, blending_method method) {
        std::vector<neo::rgb> new_matrix;
        new_matrix.resize(new_width * new_height, neo::rgb{0, 0, 0});
        std::scoped_lock lock{_matrix_mutex};
        std::size_t i = 0;
        for (std::size_t y = 0; y < new_height; ++y) {
            for (std::size_t x = 0; x < new_width; ++x) {
                new_matrix[i++] = sample(float(x) / float(new_width - 1), float(y) / float(new_height - 1), method);
            }
        }
        std::swap(new_matrix, _matrix);
        std::swap(new_width, _width);
        std::swap(new_height, _height);
    }

    void matrix_fx::tile(float overlap_x, float overlap_y) {
        std::scoped_lock lock{_matrix_mutex};
        std::vector<neo::rgb> new_matrix = _matrix;
        // Blend with overlapping areas
        const std::size_t overlap_w = std::clamp(std::size_t(overlap_x * float(width())), std::size_t(0), width() - 1);
        const std::size_t overlap_h = std::clamp(std::size_t(overlap_y * float(height())), std::size_t(0),
                                                 height() - 1);

        // First horizontal blending
        for (std::size_t x = 0; x < overlap_w; ++x) {
            const float blend_factor = float(x) / float(overlap_w - 1);
            for (std::size_t y = 0; y < height(); ++y) {
                const neo::rgb blend_from = _matrix.at((width() + x - overlap_w) + y * width());
                neo::rgb &blend_to = new_matrix.at(x + y * width());
                blend_to = blend_linear(blend_from, blend_to, blend_factor);
            }
        }

        // Then vertical
        for (std::size_t y = 0; y < overlap_h; ++y) {
            const float blend_factor = float(y) / float(overlap_h - 1);
            for (std::size_t x = 0; x < width(); ++x) {
                const neo::rgb blend_from = _matrix.at(x + (height() + y - overlap_h) * width());
                neo::rgb &blend_to = new_matrix.at(x + y * width());
                blend_to = blend_linear(blend_from, blend_to, blend_factor);
            }
        }

        std::swap(_matrix, new_matrix);
    }

    std::vector<rgb> matrix_fx::sample(std::size_t n_leds, std::chrono::milliseconds time_since_start,
                                       std::vector<rgb> recycle_buffer, blending_method method) const {
        recycle_buffer.resize(n_leds);

        // Get the fractional cycle time at the first led
        // TODO Fix this also in gradient_fx
        const float tx0 = duration_x().count() > 0
                                  ? float(time_since_start.count() % duration_x().count()) / float(duration_x().count())
                                  : 0.f;
        const float ty = duration_y().count() > 0
                                 ? float(time_since_start.count() % duration_y().count()) / float(duration_y().count())
                                 : 0.f;
        // Compute the time increment for each led
        const float dtx = n_leds > 0
                                  ? repeats_x() / float(n_leds)
                                  : 0.f;
        // We are sampling a horizontal strip so y gets special treatment and does not get repeated

        for (std::size_t i = 0; i < recycle_buffer.size(); ++i) {
            // Compute the correct time for this led
            const float tx = tx0 + float(i) * dtx;
            recycle_buffer[i] = sample(tx, ty, method);
        }

        return recycle_buffer;
    }

    std::string matrix_fx_config::to_string() const {
        std::string buffer;
        auto attempt_snprintf = [&](std::string *buffer) -> std::size_t {
            return std::snprintf(buffer != nullptr ? buffer->data() : nullptr,
                                 buffer != nullptr ? buffer->size() : 0,
                                 "matrix %d×%d, %01.1f×, x %d ms, y %d ms",
                                 width, width > 0 ? matrix.size() / width : 0, repeats_x, duration_x_ms, duration_y_ms);
        };
        buffer.clear();
        buffer.resize(attempt_snprintf(nullptr) + 1 /* null terminator */, '\0');
        attempt_snprintf(&buffer);
        buffer.resize(buffer.size() - 1);// Remove null terminator
        return buffer;
    }

    std::vector<rgb> matrix_fx::render_frame(transmittable_rgb_strip &strip, rmt_channel_t channel,
                                             std::chrono::milliseconds elapsed, std::vector<rgb> recycle_buffer,
                                             blending_method method) const {
        // Quickly try to lock, if fails, just drop frame
        mlab::scoped_try_lock lock{_matrix_mutex};
        if (lock) {
            // Use lambda initialization syntax and mutability to always recycle the buffer
            recycle_buffer = sample(strip.size(), elapsed, std::move(recycle_buffer), method);
            ESP_ERROR_CHECK_WITHOUT_ABORT(strip.update(recycle_buffer, channel, false));
        } else {
            ESP_LOGW("NEO", "Dropping frame due to locked matrix fx operation");
        }
        return recycle_buffer;
    }

    std::function<void(std::chrono::milliseconds)> matrix_fx::make_steady_timer_callback(
            transmittable_rgb_strip &strip, rmt_channel_t channel, blending_method method) const {
        return [buffer = std::vector<rgb>{}, &strip, channel, method, tracker = tracker()](std::chrono::milliseconds elapsed) mutable {
            // Do not capture this, to enable movement of the object
            if (auto *m_fx = mlab::uniquely_tracked::track<matrix_fx>(tracker); m_fx != nullptr) {
                buffer = m_fx->render_frame(strip, channel, elapsed, std::move(buffer), method);
            } else {
                ESP_LOGE("NEO", "Unable to track matrix fx object.");
            }
        };
    }


    void matrix_fx_config::apply(matrix_fx &m_fx, float tile_x, float tile_y) const {
        m_fx.set_matrix(matrix, width);
        m_fx.tile(tile_x, tile_y);
        m_fx.set_repeats_x(repeats_x);
        m_fx.set_duration_x(std::chrono::milliseconds{duration_x_ms});
        m_fx.set_duration_y(std::chrono::milliseconds{duration_y_ms});
    }

}// namespace neo

namespace mlab {
    mlab::bin_stream &operator>>(mlab::bin_stream &s, neo::matrix_fx_config &m_fx_cfg) {
        std::uint16_t matrix_size = 0;
        s >> lsb16 >> matrix_size;
        ESP_LOGI("NEO", "Matrix size: %d", matrix_size);
        if (s.remaining() < matrix_size * 3 + 13) {
            ESP_LOGW("NEO", "Not enough data to parse a matrix of size %d.", matrix_size);
            s.set_bad();
            return s;
        }
        // Safety check: too large? Want 50% margin, each color is 3 bytes
        const std::size_t avail_mem = xPortGetFreeHeapSize();
        if (avail_mem < matrix_size * 6) {
            ESP_LOGE("NEO", "Cannot parse a matrix fx of size %d, not enough memory at 50%% margin (%d)",
                     matrix_size, avail_mem);
            s.set_bad();
            return s;
        }
        m_fx_cfg.matrix.resize(matrix_size);
        for (auto &c : m_fx_cfg.matrix) {
            s >> c;
        }
        s >> m_fx_cfg.width;
        s >> lsb16 >> m_fx_cfg.repeats_x;
        s >> lsb32 >> m_fx_cfg.duration_x_ms;
        s >> lsb32 >> m_fx_cfg.duration_y_ms;
        return s;
    }
}// namespace mlab