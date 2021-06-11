//
// Created by spak on 6/5/21.
//

#include "neopixel_gradient.hpp"
#include <cassert>
#include <cmath>

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

    std::pair<gradient::iterator, bool> gradient::emplace(gradient_entry entry) {
        auto convert_it = [&] (auto entries_it) {
            return std::begin(*this) + std::distance(std::begin(_entries), entries_it);
        };

        auto it = std::upper_bound(std::begin(_entries), std::end(_entries), entry.time(), safe_less{});
        if (it != std::begin(_entries)) {
            // Make sure there is no duplicate
            auto prev_it = std::prev(it);
            if (not safe_less{}(prev_it->time(), entry.time())) {
                assert(not safe_less{}(prev_it->time(), entry.time()) and not safe_less{}(entry.time(), prev_it->time()));
                return {convert_it(prev_it), false};
            }
        }
        it = _entries.insert(it, entry);
        assert(std::is_sorted(std::begin(_entries), std::end(_entries), safe_less{}));
        return {convert_it(it), true};
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
            float last_time = std::numeric_limits<float>::lowest();
            for (auto &entry : _entries) {
                // Make sure we can actually guarantee monotonicity
                last_time = std::clamp((entry.time() - min) / delta,
                                       std::nextafter(last_time, 2.f),
                                       1.f);
                entry.set_time(last_time);
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
        if (not std::isfinite(t)) {
            t = 0.f;
        }
        if (empty()) {
            return {};
        }
        const auto ub = upper_bound(t);
        if (ub == std::begin(*this)) {
            return front().color();
        } else if (ub == std::end(*this)) {
            return back().color();
        }
        const auto lb = std::prev(ub);
        t = std::clamp((t - lb->time()) / (ub->time() - lb->time()), 0.f, 1.f);
        return method(lb->color(), ub->color(), t);
    }

    gradient::gradient(std::vector<gradient_entry> entries) : _entries{std::move(entries)} {
        std::sort(std::begin(entries), std::end(entries), safe_less{});
        normalize();
    }
    gradient::gradient(std::vector<fixed_gradient_entry> const &entries) {
        _entries.reserve(entries.size());
        for (auto const &entry : entries) {
            _entries.emplace_back(entry.time(), entry.color());
        }
        std::sort(std::begin(_entries), std::end(_entries), safe_less{});
        normalize();
    }
    gradient::gradient(std::vector<rgb> const &colors) {
        _entries.reserve(colors.size());
        float t = 0.f;
        const float dt = 1.f / float(colors.size());
        for (auto c : colors) {
            _entries.emplace_back(t, c);
            t += dt;
        }
        normalize();
    }

    gradient::gradient(std::vector<hsv> const &colors) {
        _entries.reserve(colors.size());
        float t = 0.f;
        const float dt = 1.f / float(colors.size());
        for (auto c : colors) {
            _entries.emplace_back(t, c.to_rgb());
            t += dt;
        }
        normalize();
    }
}