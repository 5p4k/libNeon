//
// Created by spak on 8/19/23.
//

#ifndef LIBNEON_FX_HPP
#define LIBNEON_FX_HPP

#include <deque>
#include <neo/alarm.hpp>
#include <neo/color.hpp>
#include <neo/gradient.hpp>

namespace neo {

    class led_encoder;

    struct fx_base : public std::enable_shared_from_this<fx_base> {
        virtual void populate(alarm const &a, mlab::range<srgb *> colors) = 0;

        [[nodiscard]] std::function<void(alarm &)> make_callback(led_encoder &encoder, std::size_t num_leds);

        virtual ~fx_base() = default;
    };

    template <class T>
    [[nodiscard]] std::shared_ptr<T> wrap(T &&t);

    struct solid_fx : fx_base {
        srgb color = {};

        solid_fx() = default;
        inline explicit solid_fx(srgb color_);


        void populate(alarm const &, mlab::range<srgb *> colors) override;
    };

    struct gradient_fx : fx_base {
        std::vector<gradient_entry> gradient = {};
        std::chrono::milliseconds rotate_cycle_time = 0ms;
        float scale = 1.f;

        gradient_fx() = default;
        inline explicit gradient_fx(std::vector<gradient_entry> gradient_, std::chrono::milliseconds rotate_cycle_time_ = 2s, float scale_ = 1.f);
        inline explicit gradient_fx(std::vector<srgb> gradient_, std::chrono::milliseconds rotate_cycle_time_ = 2s, float scale_ = 1.f);

        void populate(alarm const &a, mlab::range<srgb *> colors) override;
    };

    struct pulse_fx : fx_base {
        std::shared_ptr<fx_base> lo = {};
        std::shared_ptr<fx_base> hi = {};
        std::chrono::milliseconds cycle_time = 0ms;

        pulse_fx() = default;
        inline pulse_fx(std::shared_ptr<fx_base> lo_, std::shared_ptr<fx_base> hi_, std::chrono::milliseconds cycle_time_ = 2s);

        template <class Fx1, class Fx2>
            requires std::is_base_of_v<fx_base, Fx1> and std::is_base_of_v<fx_base, Fx2>
        pulse_fx(Fx1 lo_, Fx2 hi_, std::chrono::milliseconds cycle_time_ = 2s);

        void populate(alarm const &a, mlab::range<srgb *> colors) override;

    private:
        std::vector<srgb> _buffer;
    };

    class transition_fx : public fx_base {
        struct transition {
            std::chrono::milliseconds activation_time;
            std::chrono::milliseconds transition_duration;
            std::shared_ptr<fx_base> fx;

            [[nodiscard]] bool is_complete(std::chrono::milliseconds t) const;
            [[nodiscard]] float compute_blend_factor(std::chrono::milliseconds t) const;
        };

        std::deque<transition> _active_transitions;
        std::vector<srgb> _buffer;

        void pop_expired(std::chrono::milliseconds t);

    public:
        transition_fx() = default;
        void populate(alarm const &a, mlab::range<srgb *> colors) override;

        void transition_to(alarm const &a, std::shared_ptr<fx_base> fx, std::chrono::milliseconds duration);
    };

}// namespace neo

namespace neo {
    gradient_fx::gradient_fx(std::vector<gradient_entry> gradient_, std::chrono::milliseconds rotate_cycle_time_, float scale_)
        : gradient{std::move(gradient_)},
          rotate_cycle_time{rotate_cycle_time_},
          scale{scale_} {}

    gradient_fx::gradient_fx(std::vector<srgb> gradient_, std::chrono::milliseconds rotate_cycle_time_, float scale_)
        : gradient{neo::gradient_make_uniform_from_colors(std::move(gradient_))},
          rotate_cycle_time{rotate_cycle_time_},
          scale{scale_} {}

    solid_fx::solid_fx(neo::srgb color_) : color{color_} {}

    pulse_fx::pulse_fx(std::shared_ptr<fx_base> lo_, std::shared_ptr<fx_base> hi_, std::chrono::milliseconds cycle_time_)
        : lo{std::move(lo_)}, hi{std::move(hi_)}, cycle_time{cycle_time_} {}

    template <class Fx1, class Fx2>
        requires std::is_base_of_v<fx_base, Fx1> and std::is_base_of_v<fx_base, Fx2>
    pulse_fx::pulse_fx(Fx1 lo_, Fx2 hi_, std::chrono::milliseconds cycle_time_)
        : lo{wrap(std::move(lo_))}, hi{wrap(std::move(hi_))}, cycle_time{cycle_time_} {}

    template <class T>
    std::shared_ptr<T> wrap(T &&t) {
        return std::make_shared<T>(std::forward<T>(t));
    }
}// namespace neo

#endif//LIBNEON_FX_HPP
