//
// Created by spak on 6/4/21.
//

#include "neopixel_color.hpp"
#include <cmath>
#include <vector>

namespace neo {

    namespace {
        std::uint8_t round_float_to_byte(float f) {
            return std::uint8_t(std::round(std::clamp(f, 0.f, 255.f)));
        }

        std::uint8_t unit_float_to_byte(float f) {
            return round_float_to_byte(255.f * f);
        }

        std::array<std::uint8_t, 3> unit_float_to_byte(std::array<float, 3> f) {
            return {unit_float_to_byte(f[0]), unit_float_to_byte(f[1]), unit_float_to_byte(f[2])};
        }
    }

    std::string rgb::to_string() const {
        // Do not use stringstream, it requires tons of memory
        static thread_local std::vector<char> buffer;
        auto attempt_snprintf = [&](std::vector<char> *buffer) -> std::size_t {
            return std::snprintf(buffer != nullptr ? buffer->data() : nullptr,
                                 buffer != nullptr ? buffer->size() : 0,
                                 "#%02x%02x%02x",
                                 r, g, b);
        };
        buffer.clear();
        buffer.resize(attempt_snprintf(nullptr) + 1 /* null terminator */, '\0');
        attempt_snprintf(&buffer);
        return std::string{buffer.data()};
    }

    hsv rgb::to_hsv() const {
        static constexpr auto hue_scale = 1.f / 6.f;
        const std::uint8_t value = std::max(r, std::max(g, b));
        const std::uint8_t min_c = std::min(r, std::min(g, b));
        const std::uint8_t chroma = value - min_c;

        const float hue = [&]() -> float {
            if (chroma == 0) {
                return 0.f;
            } else if (value == r) {
                return hue_scale * float(g - b) / float(chroma);
            } else if (value == g) {
                return hue_scale * (2.f + float(b - r) / float(chroma));
            } else if (value == b) {
                return hue_scale * (4.f + float(r - g) / float(chroma));
            }
            return 0.f;
        }();

        const float saturation = (value == 0 ? 0.f : float(chroma) / float(value));

        return {std::fmod(hue + 1.f, 1.f), saturation, float(value) / 255.f};
    }

    rgb hsv::to_rgb() const {
        const float s_ = std::clamp(s, 0.f, 1.f);
        const float v_ = std::clamp(v, 0.f, 1.f);
        if (s_ < std::numeric_limits<float>::epsilon()) {
            const std::uint8_t gray = unit_float_to_byte(v_);
            return {gray, gray, gray};
        }
        // Remap in 0...6
        const float h_ = 6.f * std::clamp(h - std::floor(h), 0.f, 1.f);
        const auto hue_floor = std::floor(h_);
        const auto hue_block = unsigned(hue_floor) % 6;
        const float m = v_ * (1.f - s_);
        const float x = v_ * (1.f - s_ * (hue_block % 2 == 0 ? 1.f - h_ + hue_floor : h_ - hue_floor));
        switch (hue_block) {
            case 0:
                return unit_float_to_byte({v_, x, m});
            case 1:
                return unit_float_to_byte({x, v_, m});
            case 2:
                return unit_float_to_byte({m, v_, x});
            case 3:
                return unit_float_to_byte({m, x, v_});
            case 4:
                return unit_float_to_byte({x, m, v_});
            default:
                return unit_float_to_byte({v_, m, x});
        }
    }

    hsv &hsv::clamp() {
        h = std::clamp(h - std::floor(h), 0.f, 1.f);
        s = std::clamp(s, 0.f, 1.f);
        v = std::clamp(v, 0.f, 1.f);
        return *this;
    }

    hsv &hsv::update(maybe_update<float> h_, maybe_update<float> s_, maybe_update<float> v_) {
        h_.set(h);
        s_.set(s);
        v_.set(v);
        return *this;
    }

    rgb &rgb::update(maybe_update<std::uint8_t> r_, maybe_update<std::uint8_t> g_, maybe_update<std::uint8_t> b_) {
        r_.set(r);
        g_.set(g);
        b_.set(b);
        return *this;
    }

    rgb &rgb::shift(maybe_update<signed> dr, maybe_update<signed> dg, maybe_update<signed> db) {
        dr.add(r);
        dg.add(g);
        db.add(b);
        return *this;
    }

    hsv &hsv::shift(maybe_update<float> dh, maybe_update<float> ds, maybe_update<float> dv) {
        dh.add(h);
        ds.add(s);
        dv.add(v);
        clamp();
        return *this;
    }

    rgb &rgb::blend(const rgb &target, float factor) {
        factor = std::clamp(factor, 0.f, 1.f);
        r = round_float_to_byte(float(r) * (1.f - factor) + float(target.r) * factor);
        g = round_float_to_byte(float(g) * (1.f - factor) + float(target.g) * factor);
        b = round_float_to_byte(float(b) * (1.f - factor) + float(target.b) * factor);
        return *this;
    }

}