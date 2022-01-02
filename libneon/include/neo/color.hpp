//
// Created by spak on 6/4/21.
//

#ifndef NEO_COLOR_HPP
#define NEO_COLOR_HPP

#include <algorithm>
#include <array>
#include <cstdint>
#include <mlab/bin_data.hpp>

namespace neo {
    struct hsv;

    struct keep_t {
    };

    static constexpr keep_t keep{};

    template <class T>
    struct maybe_update {
        const bool update;
        T value;

        maybe_update(keep_t);

        maybe_update(T value_);

        void set(T &target) const;

        template <class U>
        void add(U &target) const;
    };

    [[nodiscard]] float srgb_to_linear(std::uint8_t v);
    [[nodiscard]] std::uint8_t linear_to_srgb(float v);

    /**
     * The values are expressed in sRGB color space.
     */
    struct rgb {
        std::uint8_t r = 0;
        std::uint8_t g = 0;
        std::uint8_t b = 0;

        rgb() = default;

        inline rgb(std::uint32_t rgb_);

        inline rgb(std::array<std::uint8_t, 3> rgb_);

        inline rgb(std::uint8_t r_, std::uint8_t g_, std::uint8_t b_);

        rgb &update(maybe_update<std::uint8_t> r_, maybe_update<std::uint8_t> g_, maybe_update<std::uint8_t> b_);
        [[nodiscard]] inline rgb update(maybe_update<std::uint8_t> r_, maybe_update<std::uint8_t> g_, maybe_update<std::uint8_t> b_) const;

        rgb &shift(maybe_update<signed> dr, maybe_update<signed> dg, maybe_update<signed> db);
        [[nodiscard]] inline rgb shift(maybe_update<signed> dr, maybe_update<signed> dg, maybe_update<signed> db) const;

        inline rgb &shift(rgb delta, bool negate = false);
        [[nodiscard]] inline rgb shift(rgb delta, bool negate = false) const;

        rgb &blend(rgb target, float factor);
        [[nodiscard]] inline rgb blend(rgb target, float factor) const;

        [[nodiscard]] hsv to_hsv() const;

        [[nodiscard]] std::array<float, 3> to_linear_rgb() const;
        [[nodiscard]] static rgb from_linear_rgb(std::array<float, 3> const &linear_rgb);

        [[nodiscard]] std::string to_string() const;
    };

    struct hsv {
        float h = 0.f;
        float s = 0.f;
        float v = 0.f;

        hsv() = default;

        inline hsv(std::array<float, 3> hsv_);

        inline hsv(float h_, float s_, float v_);

        hsv &clamp();

        hsv &update(maybe_update<float> h_, maybe_update<float> s_, maybe_update<float> v_);
        [[nodiscard]] inline hsv update(maybe_update<float> h_, maybe_update<float> s_, maybe_update<float> v_) const;

        hsv &shift(maybe_update<float> dh, maybe_update<float> ds, maybe_update<float> dv);
        [[nodiscard]] inline hsv shift(maybe_update<float> dh, maybe_update<float> ds, maybe_update<float> dv) const;

        inline hsv &shift(hsv delta, bool negate = false);
        [[nodiscard]] inline hsv shift(hsv delta, bool negate = false) const;

        [[nodiscard]] rgb to_rgb() const;
    };

}// namespace neo

namespace neo {

    rgb::rgb(std::uint8_t r_, std::uint8_t g_, std::uint8_t b_) : r{r_}, g{g_}, b{b_} {}

    rgb::rgb(std::array<std::uint8_t, 3> rgb_) : rgb{rgb_[0], rgb_[1], rgb_[2]} {}

    rgb::rgb(std::uint32_t rgb_) : rgb{
                                           std::uint8_t(0xff & (rgb_ >> 16)),
                                           std::uint8_t(0xff & (rgb_ >> 8)),
                                           std::uint8_t(0xff & rgb_)} {}


    hsv::hsv(float h_, float s_, float v_) : h{h_}, s{s_}, v{v_} {}

    hsv::hsv(std::array<float, 3> hsv_) : hsv{hsv_[0], hsv_[1], hsv_[2]} {}


    template <class T>
    maybe_update<T>::maybe_update(keep_t) : update{false}, value{} {}

    template <class T>
    maybe_update<T>::maybe_update(T value_) : update{true}, value{value_} {}

    template <class T>
    void maybe_update<T>::set(T &target) const {
        if (update) {
            target = value;
        }
    }

    template <class T>
    template <class U>
    void maybe_update<T>::add(U &target) const {
        if (update) {
            if constexpr (not std::is_signed_v<U> and std::is_signed_v<T>) {
                target = U(std::clamp(T(target + value),
                                      T(std::numeric_limits<U>::lowest()),
                                      T(std::numeric_limits<U>::max())));
            } else {
                target += value;
            }
        }
    }

    rgb &rgb::shift(rgb delta, bool negate) {
        return negate ? shift(-delta.r, -delta.g, -delta.b) : shift(delta.r, delta.g, delta.b);
    }

    hsv &hsv::shift(hsv delta, bool negate) {
        return negate ? shift(-delta.h, -delta.s, -delta.v) : shift(delta.h, delta.s, delta.v);
    }

    rgb rgb::update(maybe_update<std::uint8_t> r_, maybe_update<std::uint8_t> g_, maybe_update<std::uint8_t> b_) const {
        rgb retval = *this;
        retval.update(r_, g_, b_);
        return retval;
    }

    rgb rgb::shift(maybe_update<signed> dr, maybe_update<signed> dg, maybe_update<signed> db) const {
        rgb retval = *this;
        retval.shift(dr, dg, db);
        return retval;
    }

    rgb rgb::shift(rgb delta, bool negate) const {
        rgb retval = *this;
        retval.shift(delta, negate);
        return retval;
    }

    hsv hsv::update(maybe_update<float> h_, maybe_update<float> s_, maybe_update<float> v_) const {
        hsv retval = *this;
        retval.update(h_, s_, v_);
        return retval;
    }

    hsv hsv::shift(maybe_update<float> dh, maybe_update<float> ds, maybe_update<float> dv) const {
        hsv retval = *this;
        retval.shift(dh, ds, dv);
        return retval;
    }

    hsv hsv::shift(hsv delta, bool negate) const {
        hsv retval = *this;
        retval.shift(delta, negate);
        return retval;
    }

    rgb rgb::blend(rgb target, float factor) const {
        rgb retval = *this;
        retval.blend(target, factor);
        return retval;
    }

}// namespace neo

namespace mlab {
    bin_data &operator<<(bin_data &o, neo::rgb c);
    bin_stream &operator>>(bin_stream &i, neo::rgb &c);
}// namespace mlab

#endif//NEO_COLOR_HPP
