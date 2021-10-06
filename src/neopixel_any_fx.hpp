//
// Created by spak on 10/6/21.
//

#ifndef PICOSKATE_NEOPIXEL_ANY_FX_HPP
#define PICOSKATE_NEOPIXEL_ANY_FX_HPP

#include <cstdint>
#include "bin_data.hpp"
#include "neopixel_gradient_fx.hpp"
#include "neopixel_matrix_fx.hpp"
#include "neopixel_solid_fx.hpp"
#include "any_of.hpp"

namespace neo {

    enum struct fx_type : std::uint8_t {
        solid = 0,
        gradient = 1,
        matrix = 2
    };


    template <fx_type>
    struct any_fx_config_data {};

    template <>
    struct any_fx_config_data<fx_type::solid> : public solid_fx_config {
        any_fx_config_data(solid_fx_config cfg) : solid_fx_config{std::forward<solid_fx_config>(cfg)} {}
    };


    template <>
    struct any_fx_config_data<fx_type::gradient> : public gradient_fx_config {
        any_fx_config_data(gradient_fx_config cfg) : gradient_fx_config{std::forward<gradient_fx_config>(cfg)} {}
    };


    template <>
    struct any_fx_config_data<fx_type::matrix> : public matrix_fx_config {
        any_fx_config_data(matrix_fx_config cfg) : matrix_fx_config{std::forward<matrix_fx_config>(cfg)} {}
    };

    class any_fx_config : public mlab::any_of<fx_type, any_fx_config_data> {
    public:
        using mlab::any_of<fx_type, any_fx_config_data>::any_of;
    };


    template <fx_type>
    struct any_fx {};

    template <>
    struct any_fx<fx_type::solid> {
        solid_fx fx{};
        any_fx() = default;
        any_fx(solid_fx fx_) : fx{std::move(fx_)} {}
    };


    template <>
    struct any_fx<fx_type::gradient> {
        gradient_fx fx{};
        any_fx() = default;
        any_fx(gradient_fx fx_) : fx{std::move(fx_)} {}
    };


    template <>
    struct any_fx<fx_type::matrix> {
        matrix_fx fx{};
        any_fx() = default;
        any_fx(matrix_fx fx_) : fx{std::move(fx_)} {}
    };


}

namespace mlab {
    mlab::bin_stream &operator>>(mlab::bin_stream &s, neo::any_fx_config &fx_cfg);
}

#endif //PICOSKATE_NEOPIXEL_ANY_FX_HPP
