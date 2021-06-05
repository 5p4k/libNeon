//
// Created by spak on 6/5/21.
//

#include "neopixel_gradient.hpp"
#include <cassert>

namespace neo {

    namespace {
        struct safe_less {
            bool operator()(float l, float r) const {
                return std::abs(l - r) > std::numeric_limits<float>::epsilon() and l < r;
            }

            bool operator()(fixed_gradient_entry const &l, float r) const {
                return (*this)(l.time(), r);
            }

            bool operator()(float l, fixed_gradient_entry const &r) const {
                return (*this)(l, r.time());
            }

            bool operator()(fixed_gradient_entry const &l, fixed_gradient_entry const &r) const {
                return (*this)(l.time(), r.time());
            }

            bool operator()(gradient_entry const &l, float r) const {
                return (*this)(l.time(), r);
            }

            bool operator()(float l, gradient_entry const &r) const {
                return (*this)(l, r.time());
            }

            bool operator()(gradient_entry const &l, gradient_entry const &r) const {
                return (*this)(l.time(), r.time());
            }
        };
    }

    gradient::const_iterator gradient::lower_bound(float t) const {
        return std::lower_bound(std::begin(*this), std::end(*this), t, safe_less{});
    }

    gradient::const_iterator gradient::upper_bound(float t) const {
        return std::upper_bound(std::begin(*this), std::end(*this), t, safe_less{});
    }

    gradient::iterator gradient::emplace(gradient_entry entry) {
        auto it = std::upper_bound(std::begin(_entries), std::end(_entries), entry.time(), safe_less{});
        it = _entries.insert(it, entry);
        return std::begin(*this) + std::distance(std::begin(_entries), it);
    }

    void  gradient::normalize() {
        if (empty()) {
            return;
        } else if (size() == 1) {
            _entries.front().set_time(0.f);
        } else if (size() == 2) {
            _entries.front().set_time(0.f);
            _entries.back().set_time(1.f);
        } else {
            const float min = _entries.front().time();
            const float delta = _entries.back().time() - min;
            for (auto &entry : _entries) {
                entry.set_time(std::clamp((entry.time() - min) / delta, 0.f, 1.f));
            }
            assert(std::is_sorted(std::begin(_entries), std::end(_entries), safe_less{}));
        }
    }


    rgb blend_linear(rgb const &l, rgb const &r, float t) {
        return l.blend(r, t);
    }

    rgb blend_round_down(rgb const &l, rgb const &, float) {
        return l;
    }

    rgb blend_round_up(rgb const &, rgb const &r, float) {
        return r;
    }

    rgb blend_nearest_neighbor(rgb const &l, rgb const &r, float t) {
        return safe_less{}(t, 0.5f) ? l : r;
    }

    rgb gradient::sample(float t, blending_method method) const {
        if (empty()) {
            return {};
        } else if (size() == 1) {
            return front().color();
        } else if (safe_less{}(t, front().time())) {
            return front().color();
        } else if (safe_less{}(back().time(), t)) {
            return back().color();
        } else {
            const auto lb = lower_bound(t);
            if (lb == std::end(*this)) {
                return back().color();
            }
            const auto ub = std::next(lb);
            if (ub == std::end(*this)) {
                return back().color();
            }
            t = std::clamp((t - lb->time()) / (ub->time() - lb->time()), 0.f, 1.f);
            return method(lb->color(), ub->color(), t);
        }
    }
}