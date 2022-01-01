//
// Created by spak on 10/6/21.
//

#include <neo/any_fx.hpp>

namespace neo {

    any_fx::any_fx(any_fx &&other) noexcept : any_fx{} {
        *this = std::move(other);
    }

    any_fx &any_fx::operator=(any_fx &&other) noexcept {
        auto hold_swap = other.swap(*this);
        std::swap(_s_fx, other._s_fx);
        std::swap(_g_fx, other._g_fx);
        std::swap(_m_fx, other._m_fx);
        const fx_type other_type = other._type;
        other._type = _type.load();
        _type = other_type;
        return *this;
    }

    std::function<void(std::chrono::milliseconds)> any_fx::make_steady_timer_callback(
            transmittable_rgb_strip &strip, rmt_channel_t channel, blending_method method) const
    {
        return [buffer = std::vector<rgb>{}, &strip, channel, method, tracker = tracker()]
                (std::chrono::milliseconds elapsed) mutable
        {
            // Do not capture this, to enable movement of the object
            if (auto *fx = mlab::uniquely_tracked::track<any_fx>(tracker); fx != nullptr) {
                buffer = fx->render_frame(strip, channel, elapsed, std::move(buffer), method);
            } else {
                ESP_LOGE("NEO", "Unable to track any fx object.");
            }
        };
    }

    std::vector<rgb> any_fx::render_frame(transmittable_rgb_strip &strip, rmt_channel_t channel,
                                  std::chrono::milliseconds elapsed, std::vector<rgb> recycle_buffer,
                                  blending_method method) const
    {
        switch (type()) {
            case fx_type::solid:
                s_fx().render_frame(strip, channel);
                break;
            case fx_type::gradient:
                recycle_buffer = g_fx().render_frame(strip, channel, elapsed, std::move(recycle_buffer), method);
                break;
            case fx_type::matrix:
                recycle_buffer = m_fx().render_frame(strip, channel, elapsed, std::move(recycle_buffer), method);
                break;
        }
        return recycle_buffer;
    }

    std::string any_fx_config::to_string() const {
        switch (type()) {
            case fx_type::solid:
                return get<fx_type::solid>().to_string();
            case fx_type::gradient:
                return get<fx_type::gradient>().to_string();
            case fx_type::matrix:
                return get<fx_type::matrix>().to_string();
        }
        return "UNKNOWN";
    }

    void any_fx_config::apply(any_fx &fx) const {
        switch (type()) {
            case fx_type::solid:
                get<fx_type::solid>().apply(fx._s_fx);
                break;
            case fx_type::gradient:
                get<fx_type::gradient>().apply(fx._g_fx);
                break;
            case fx_type::matrix:
                get<fx_type::matrix>().apply(fx._m_fx);
                break;
        }
        fx.set_type(type());
    }
}

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