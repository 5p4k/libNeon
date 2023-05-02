//
// Created by spak on 6/5/21.
//

#include <cassert>
#include <cmath>
#include <iomanip>
#include <neo/gradient.hpp>
#include <neo/math.hpp>

namespace neo {

    namespace {
        struct safe_less {
            bool operator()(float l, float r) const {
                return std::abs(l - r) > std::numeric_limits<float>::epsilon() and l < r;
            }

            bool operator()(fixed_gradient_entry const &l, float r) const {
                return (*this)(l.position(), r);
            }

            bool operator()(float l, fixed_gradient_entry const &r) const {
                return (*this)(l, r.position());
            }

            bool operator()(fixed_gradient_entry const &l, fixed_gradient_entry const &r) const {
                return (*this)(l.position(), r.position());
            }

            bool operator()(gradient_entry const &l, float r) const {
                return (*this)(l.position(), r);
            }

            bool operator()(float l, gradient_entry const &r) const {
                return (*this)(l, r.position());
            }

            bool operator()(gradient_entry const &l, gradient_entry const &r) const {
                return (*this)(l.position(), r.position());
            }
        };
    }// namespace

    gradient::const_iterator gradient::lower_bound(float t) const {
        return std::lower_bound(std::begin(*this), std::end(*this), t, safe_less{});
    }

    gradient::const_iterator gradient::upper_bound(float t) const {
        return std::upper_bound(std::begin(*this), std::end(*this), t, safe_less{});
    }

    std::pair<gradient::iterator, bool> gradient::emplace(gradient_entry entry) {
        auto convert_it = [&](auto entries_it) {
            return std::begin(*this) + std::distance(std::begin(_entries), entries_it);
        };

        auto it = std::upper_bound(std::begin(_entries), std::end(_entries), entry.position(), safe_less{});
        if (it != std::begin(_entries)) {
            // Make sure there is no duplicate
            auto prev_it = std::prev(it);
            if (not safe_less{}(prev_it->position(), entry.position())) {
                assert(not safe_less{}(prev_it->position(), entry.position()) and not safe_less{}(entry.position(), prev_it->position()));
                return {convert_it(prev_it), false};
            }
        }
        it = _entries.insert(it, entry);
        assert(std::is_sorted(std::begin(_entries), std::end(_entries), safe_less{}));
        return {convert_it(it), true};
    }

    void gradient::normalize() {
        if (empty()) {
            return;
        } else if (size() == 1) {
            _entries.front().set_position(0.f);
        } else if (size() == 2) {
            _entries.front().set_position(0.f);
            _entries.back().set_position(1.f);
        } else {
            const float min = _entries.front().position();
            const float delta = _entries.back().position() - min;
            float last_time = std::numeric_limits<float>::lowest();
            for (auto &entry : _entries) {
                // Make sure we can actually guarantee monotonicity
                last_time = std::clamp((entry.position() - min) / delta,
                                       std::nextafter(last_time, 2.f),
                                       1.f);
                entry.set_position(last_time);
            }
            assert(std::is_sorted(std::begin(_entries), std::end(_entries), safe_less{}));
        }
    }


    srgb blend_linear(srgb l, srgb r, float t) {
        return l.blend(r, t);
    }

    srgb blend_round_down(srgb l, srgb, float) {
        return l;
    }

    srgb blend_round_up(srgb, srgb r, float) {
        return r;
    }

    srgb blend_nearest_neighbor(srgb l, srgb r, float t) {
        return safe_less{}(t, 0.5f) ? l : r;
    }

    std::string fixed_gradient_entry::to_string() const {
        std::string buffer;
        const auto color_str = color().to_string();
        auto attempt_snprintf = [&](std::string *buffer) -> std::size_t {
            return std::snprintf(buffer != nullptr ? buffer->data() : nullptr,
                                 buffer != nullptr ? buffer->size() : 0,
                                 "%0.2f: %s",
                                 position(), color_str.c_str());
        };
        buffer.clear();
        buffer.resize(attempt_snprintf(nullptr) + 1 /* null terminator */, '\0');
        attempt_snprintf(&buffer);
        buffer.resize(buffer.size() - 1);// Remove null terminator
        return buffer;
    }

    std::string gradient::to_string() const {
        // Make a copy and normalize
        gradient copy = *this;
        copy.normalize();
        std::string s;
        s.reserve(15 * copy.size() + 2);
        s += '[';
        for (std::size_t i = 0; i < copy.size(); ++i) {
            s += copy[i].to_string();
            if (i < copy.size() - 1) {
                s += ", ";
            }
        }
        s += ']';
        return s;
    }

    srgb gradient::sample(float progress, float offset, float repeat, blending_method method) const {
        return sample(modclamp((progress + offset) * repeat), method);
    }

    srgb gradient::sample(float t, blending_method method) const {
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
        t = std::clamp((t - lb->position()) / (ub->position() - lb->position()), 0.f, 1.f);
        return method(lb->color(), ub->color(), t);
    }

    gradient::gradient(std::vector<gradient_entry> entries) : _entries{std::move(entries)} {
        std::sort(std::begin(entries), std::end(entries), safe_less{});
        normalize();
    }
    gradient::gradient(std::vector<fixed_gradient_entry> const &entries) {
        _entries.reserve(entries.size());
        for (auto const &entry : entries) {
            _entries.emplace_back(entry.position(), entry.color());
        }
        std::sort(std::begin(_entries), std::end(_entries), safe_less{});
        normalize();
    }
    gradient::gradient(std::vector<srgb> const &colors) {
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
}// namespace neo

namespace mlab {

    namespace {
        [[nodiscard]] std::uint8_t unit_float_to_byte(float f) {
            return std::uint8_t(255.f * std::clamp(f, 0.f, 1.f));
        }
        [[nodiscard]] float byte_to_unit_float(std::uint8_t b) {
            return float(b) / 255.f;
        }
    }// namespace

    bin_stream &operator>>(bin_stream &i, neo::gradient_entry &ge) {
        std::uint8_t t = 0;
        i >> ge._color >> t;
        ge.set_position(byte_to_unit_float(t));
        return i;
    }

    bin_data &operator<<(bin_data &o, neo::fixed_gradient_entry const &fge) {
        return o << fge.color() << unit_float_to_byte(fge.position());
    }


    bin_data &operator<<(bin_data &o, neo::gradient const &g) {
        o << std::uint8_t(g.size());
        for (const auto entry : g) {
            o << entry;
        }
        return o;
    }

    bin_stream &operator>>(bin_stream &i, neo::gradient &g) {
        std::uint8_t n_entries = 0;
        i >> n_entries;
        if (not i.bad() and i.remaining() >= n_entries * 4) {
            std::vector<neo::gradient_entry> entries;
            entries.resize(n_entries);
            for (auto &entry : entries) {
                i >> entry;
            }
            g = neo::gradient(std::move(entries));
        } else {
            i.set_bad();
        }
        return i;
    }
}// namespace mlab