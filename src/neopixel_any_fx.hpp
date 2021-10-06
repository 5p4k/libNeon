//
// Created by spak on 10/6/21.
//

#ifndef PICOSKATE_NEOPIXEL_ANY_FX_HPP
#define PICOSKATE_NEOPIXEL_ANY_FX_HPP

#include "any_of.hpp"
#include "bin_data.hpp"
#include "neopixel_gradient_fx.hpp"
#include "neopixel_matrix_fx.hpp"
#include "neopixel_solid_fx.hpp"

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
        using solid_fx_config::solid_fx_config;
    };


    template <>
    struct any_fx_config_data<fx_type::gradient> : public gradient_fx_config {
        using gradient_fx_config::gradient_fx_config;
    };


    template <>
    struct any_fx_config_data<fx_type::matrix> : public matrix_fx_config {
        using matrix_fx_config::matrix_fx_config;
    };

    class any_fx_config : public mlab::any_of<fx_type, any_fx_config_data> {
    public:
        using mlab::any_of<fx_type, any_fx_config_data>::any_of;
    };

}

namespace mlab {
    mlab::bin_stream &operator>>(mlab::bin_stream &s, neo::any_fx_config &fx_cfg);
}

#endif //PICOSKATE_NEOPIXEL_ANY_FX_HPP
