//
// Created by spak on 10/6/21.
//

#include "neopixel_any_fx.hpp"

namespace mlab {

    mlab::bin_stream &operator>>(mlab::bin_stream &s, neo::any_fx_config &fx_cfg) {
        neo::fx_type type = neo::fx_type::solid;
        s >> type;
        if (not s.bad()) {
            switch (type) {
                case neo::fx_type::solid: {
                    neo::solid_fx_config s_cfg{};
                    s >> s_cfg;
                    fx_cfg = neo::any_fx_config_data<neo::fx_type::solid>{std::forward<neo::solid_fx_config>(s_cfg)};
                }
                    break;
                case neo::fx_type::gradient: {
                    neo::gradient_fx_config g_cfg{};
                    s >> g_cfg;
                    fx_cfg = neo::any_fx_config_data<neo::fx_type::gradient>{std::forward<neo::gradient_fx_config>(g_cfg)};
                }
                    break;
                case neo::fx_type::matrix: {
                    neo::matrix_fx_config m_cfg{};
                    s >> m_cfg;
                    fx_cfg = neo::any_fx_config_data<neo::fx_type::matrix>{std::forward<neo::matrix_fx_config>(m_cfg)};
                }
                    break;
            }
        }
        return s;
    }
}