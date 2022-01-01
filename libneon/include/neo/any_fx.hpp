//
// Created by spak on 10/6/21.
//

#ifndef PICOSKATE_NEOPIXEL_ANY_FX_HPP
#define PICOSKATE_NEOPIXEL_ANY_FX_HPP

#include <cstdint>
#include <mlab/bin_data.hpp>
#include <neo/gradient_fx.hpp>
#include <neo/matrix_fx.hpp>
#include <neo/solid_fx.hpp>
#include <mlab/any_of.hpp>

namespace neo {

    enum struct fx_type : std::uint8_t {
        solid = 0,
        gradient = 1,
        matrix = 2
    };


    template <fx_type>
    struct any_fx_config_data {};

    class any_fx : public mlab::uniquely_tracked {
        std::atomic<fx_type> _type = fx_type::solid;
        solid_fx _s_fx{};
        gradient_fx _g_fx{};
        matrix_fx _m_fx{};

        friend class any_fx_config;
    public:

        any_fx() = default;

        any_fx(any_fx &&other) noexcept;
        any_fx &operator=(any_fx &&other) noexcept;

        [[nodiscard]] inline fx_type type() const;
        inline void set_type(fx_type t);

        [[nodiscard]] inline solid_fx const &s_fx() const;
        [[nodiscard]] inline gradient_fx const &g_fx() const;
        [[nodiscard]] inline matrix_fx const &m_fx() const;

        [[nodiscard]] std::function<void(std::chrono::milliseconds)> make_steady_timer_callback(
                transmittable_rgb_strip &strip, rmt_channel_t channel, blending_method method = blend_linear) const;

        std::vector<rgb> render_frame(transmittable_rgb_strip &strip, rmt_channel_t channel,
                                      std::chrono::milliseconds elapsed, std::vector<rgb> recycle_buffer = {},
                                      blending_method method = blend_linear) const;
    };


    class any_fx_config : public mlab::any_of<fx_type, any_fx_config_data> {
    public:
        using mlab::any_of<fx_type, any_fx_config_data>::any_of;
        inline any_fx_config();

        void apply(any_fx &fx) const;

        [[nodiscard]] std::string to_string() const;
    };


    template <>
    struct any_fx_config_data<fx_type::solid> : public solid_fx_config {
        explicit any_fx_config_data(solid_fx_config cfg) : solid_fx_config{std::forward<solid_fx_config>(cfg)} {}
    };


    template <>
    struct any_fx_config_data<fx_type::gradient> : public gradient_fx_config {
        explicit any_fx_config_data(gradient_fx_config cfg) : gradient_fx_config{std::forward<gradient_fx_config>(cfg)} {}
    };


    template <>
    struct any_fx_config_data<fx_type::matrix> : public matrix_fx_config {
        explicit any_fx_config_data(matrix_fx_config cfg) : matrix_fx_config{std::forward<matrix_fx_config>(cfg)} {}
    };

    solid_fx const &any_fx::s_fx() const {
        return _s_fx;
    }
    gradient_fx const &any_fx::g_fx() const {
        return _g_fx;
    }
    matrix_fx const &any_fx::m_fx() const {
        return _m_fx;
    }

    fx_type any_fx::type() const {
        return _type;
    }
    void any_fx::set_type(fx_type t) {
        _type = t;
    }
    any_fx_config::any_fx_config() :
        mlab::any_of<fx_type, any_fx_config_data>{any_fx_config_data<fx_type::solid>{solid_fx_config{}}}
    {}
}

namespace mlab {
    mlab::bin_stream &operator>>(mlab::bin_stream &s, neo::any_fx_config &fx_cfg);
}

#endif //PICOSKATE_NEOPIXEL_ANY_FX_HPP
