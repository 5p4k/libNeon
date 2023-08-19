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

    struct fx_base {
        virtual void populate(alarm const &a, mlab::range<srgb *> colors) = 0;

        virtual ~fx_base() = default;
    };

    struct solid_fx : fx_base {
        srgb color;

        void populate(alarm const &, mlab::range<srgb *> colors) override;
    };

    struct gradient_fx : fx_base {
        std::vector<gradient_entry> gradient = {};
        float scale = 1.f;
        std::chrono::milliseconds rotate_cycle_time = 0ms;

        void populate(alarm const &a, mlab::range<srgb *> colors) override;
    };

    struct pulse_fx : fx_base {
        std::shared_ptr<fx_base> lo;
        std::shared_ptr<fx_base> hi;
        std::chrono::milliseconds cycle_time;

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
        void populate(alarm const &a, mlab::range<srgb *> colors) override;

        void transition_to(alarm const &a, std::shared_ptr<fx_base> fx, std::chrono::milliseconds duration);
    };

}// namespace neo

#endif//LIBNEON_FX_HPP
