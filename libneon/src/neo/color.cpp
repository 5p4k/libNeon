//
// Created by spak on 6/4/21.
//

#include <cmath>
#include <neo/color.hpp>
#include <vector>

namespace neo {

    namespace {
        std::array<std::uint8_t, 3> f3_linear_to_srgb(std::array<float, 3> f) {
            return {linear_to_srgb(f[0]), linear_to_srgb(f[1]), linear_to_srgb(f[2])};
        }
    }// namespace

    float srgb_to_linear(std::uint8_t v) {
        const float f = float(v) / 255.f;
        return f <= 0.04045f ? f / 12.92f : std::pow((f + 0.055f) / 1.055f, 2.4f);
    }

    std::uint8_t linear_to_srgb(float v) {
        v = v <= 0.0031308f ? v * 12.92f : 1.055f * std::pow(v, 1.f / 2.4f) - 0.055f;
        return std::uint8_t(std::round(std::clamp(v * 255.f, 0.f, 255.f)));
    }


    std::string rgb::to_string() const {
        // Do not use stringstream, it requires tons of memory
        std::string buffer;
        auto attempt_snprintf = [&](std::string *buffer) -> std::size_t {
            return std::snprintf(buffer != nullptr ? buffer->data() : nullptr,
                                 buffer != nullptr ? buffer->size() : 0,
                                 "#%02x%02x%02x",
                                 r, g, b);
        };
        buffer.clear();
        buffer.resize(attempt_snprintf(nullptr) + 1 /* null terminator */, '\0');
        attempt_snprintf(&buffer);
        buffer.resize(buffer.size() - 1);// Remove null terminator
        return buffer;
    }


    std::array<float, 3> rgb::to_linear_rgb() const {
        return {srgb_to_linear(r), srgb_to_linear(g), srgb_to_linear(b)};
    }

    rgb rgb::from_linear_rgb(std::array<float, 3> const &linear_rgb) {
        return {linear_to_srgb(linear_rgb[0]), linear_to_srgb(linear_rgb[1]), linear_to_srgb(linear_rgb[2])};
    }

    hsv rgb::to_hsv() const {
        static constexpr auto hue_scale = 1.f / 6.f;
        const auto [lin_r, lin_g, lin_b] = to_linear_rgb();

        const float value = std::max(lin_r, std::max(lin_g, lin_b));
        const float min_c = std::min(lin_r, std::min(lin_g, lin_b));
        const float chroma = value - min_c;

        // Note: explicitly copy structured bindings
        const float hue = [&, lin_r = lin_r, lin_g = lin_g, lin_b = lin_b]() -> float {
            if (chroma < std::numeric_limits<float>::epsilon()) {
                return 0.f;
            } else if (value == lin_r) {
                return hue_scale * float(lin_g - lin_b) / float(chroma);
            } else if (value == lin_g) {
                return hue_scale * (2.f + float(lin_b - lin_r) / float(chroma));
            } else if (value == lin_b) {
                return hue_scale * (4.f + float(lin_r - lin_g) / float(chroma));
            }
            return 0.f;
        }();

        const float saturation = (value < std::numeric_limits<float>::epsilon() ? 0.f : chroma / value);

        return {std::fmod(hue + 1.f, 1.f), saturation, value};
    }

    rgb hsv::to_rgb() const {
        const float s_ = std::clamp(s, 0.f, 1.f);
        const float v_ = std::clamp(v, 0.f, 1.f);
        if (s_ < std::numeric_limits<float>::epsilon()) {
            const std::uint8_t gray = linear_to_srgb(v_);
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
                return f3_linear_to_srgb({v_, x, m});
            case 1:
                return f3_linear_to_srgb({x, v_, m});
            case 2:
                return f3_linear_to_srgb({m, v_, x});
            case 3:
                return f3_linear_to_srgb({m, x, v_});
            case 4:
                return f3_linear_to_srgb({x, m, v_});
            default:
                return f3_linear_to_srgb({v_, m, x});
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

    rgb &rgb::blend(rgb target, float factor) {
        factor = std::clamp(factor, 0.f, 1.f);
        r = linear_to_srgb(srgb_to_linear(r) * (1.f - factor) + srgb_to_linear(target.r) * factor);
        g = linear_to_srgb(srgb_to_linear(g) * (1.f - factor) + srgb_to_linear(target.g) * factor);
        b = linear_to_srgb(srgb_to_linear(b) * (1.f - factor) + srgb_to_linear(target.b) * factor);
        return *this;
    }

}// namespace neo

namespace mlab {

    bin_data &operator<<(bin_data &o, neo::rgb c) {
        return o << c.r << c.g << c.b;
    }

    bin_stream &operator>>(bin_stream &i, neo::rgb &c) {
        return i >> c.r >> c.g >> c.b;
    }
}// namespace mlab