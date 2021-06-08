//
// Created by spak on 6/5/21.
//

#ifndef PICOSKATE_NEOPIXEL_GRADIENT_HPP
#define PICOSKATE_NEOPIXEL_GRADIENT_HPP

#include "neopixel_color.hpp"
#include <vector>

namespace neo {
    using blending_method = rgb (&)(rgb const &l, rgb const &r, float t);

    [[maybe_unused]] rgb blend_linear(rgb const &l, rgb const &r, float t);
    [[maybe_unused]] rgb blend_round_down(rgb const &l, rgb const &, float);
    [[maybe_unused]] rgb blend_round_up(rgb const &, rgb const &r, float);
    [[maybe_unused]] rgb blend_nearest_neighbor(rgb const &l, rgb const &r, float t);

    class fixed_gradient_entry {
    protected:
        float _time = 0.f;
        rgb _color = rgb{};

        fixed_gradient_entry &operator=(fixed_gradient_entry const &) = default;

        fixed_gradient_entry &operator=(fixed_gradient_entry &&) noexcept = default;

    public:
        [[nodiscard]] inline float time() const;

        [[nodiscard]] inline rgb const &color() const;

        inline void set_color(rgb color);

        fixed_gradient_entry() = default;

        fixed_gradient_entry(fixed_gradient_entry const &) = default;

        fixed_gradient_entry(fixed_gradient_entry &&) noexcept = default;

        inline fixed_gradient_entry(float t, rgb c);

        inline fixed_gradient_entry &operator=(rgb c);
    };

    class gradient_entry : protected fixed_gradient_entry {
        friend class gradient;

    public:
        using fixed_gradient_entry::fixed_gradient_entry;
        using fixed_gradient_entry::operator=;
        using fixed_gradient_entry::time;
        using fixed_gradient_entry::color;
        using fixed_gradient_entry::set_color;

        inline void set_time(float t);

        gradient_entry() = default;

        gradient_entry(gradient_entry const &) = default;

        gradient_entry(gradient_entry &&) noexcept = default;

        gradient_entry &operator=(gradient_entry const &) = default;

        gradient_entry &operator=(gradient_entry &&) noexcept = default;
    };

    class gradient {
        std::vector<gradient_entry> _entries;
    public:
        using iterator = gradient_entry *;
        using const_iterator = fixed_gradient_entry const *;

        gradient() = default;
        explicit gradient(std::vector<gradient_entry> entries);
        explicit gradient(std::vector<fixed_gradient_entry> const &entries);
        explicit gradient(std::vector<rgb> const &colors);
        explicit gradient(std::vector<hsv> const &colors);

        [[nodiscard]] inline std::size_t size() const;

        [[nodiscard]] inline bool empty() const;

        [[nodiscard]] inline const_iterator begin() const;

        [[nodiscard]] inline const_iterator end() const;

        [[nodiscard]] inline iterator begin();

        [[nodiscard]] inline iterator end();

        [[nodiscard]] inline fixed_gradient_entry &front();

        [[nodiscard]] inline fixed_gradient_entry &back();

        [[nodiscard]] inline fixed_gradient_entry const &front() const;

        [[nodiscard]] inline fixed_gradient_entry const &back() const;

        inline std::pair<iterator, bool> emplace(fixed_gradient_entry entry);

        std::pair<iterator, bool> emplace(gradient_entry entry);

        [[nodiscard]] const_iterator lower_bound(float t) const;

        [[nodiscard]] const_iterator upper_bound(float t) const;

        void normalize();

        [[nodiscard]] inline fixed_gradient_entry const &operator[](std::size_t i) const;
        [[nodiscard]] inline fixed_gradient_entry &operator[](std::size_t i);

        rgb sample(float t, blending_method = blend_linear) const;
    };
}


namespace neo {
    bool gradient::empty() const {
        return _entries.empty();
    }

    std::size_t gradient::size() const {
        return _entries.size();
    }


    gradient::const_iterator gradient::begin() const {
        return _entries.data();
    }

    gradient::const_iterator gradient::end() const {
        return begin() + size();
    }

    gradient::iterator gradient::begin() {
        return _entries.data();
    }

    gradient::iterator gradient::end() {
        return begin() + size();
    }

    fixed_gradient_entry &gradient::front() {
        return _entries.front();
    }

    fixed_gradient_entry &gradient::back() {
        return _entries.back();
    }

    fixed_gradient_entry const &gradient::front() const {
        return _entries.front();
    }

    fixed_gradient_entry const &gradient::back() const {
        return _entries.back();
    }

    void fixed_gradient_entry::set_color(rgb color) {
        _color = color;
    }

    fixed_gradient_entry::fixed_gradient_entry(float t, rgb c) : _time{t}, _color{c} {}

    float fixed_gradient_entry::time() const {
        return _time;
    }

    const rgb &fixed_gradient_entry::color() const {
        return _color;
    }

    fixed_gradient_entry & fixed_gradient_entry::operator=(rgb c) {
        set_color(c);
        return *this;
    }

    void gradient_entry::set_time(float t) {
        _time = t;
    }

    std::pair<gradient::iterator, bool> gradient::emplace(fixed_gradient_entry entry) {
        return emplace(gradient_entry{entry.time(), entry.color()});
    }

    fixed_gradient_entry const &gradient::operator[](std::size_t i) const {
        return _entries.at(i);
    }
    fixed_gradient_entry &gradient::operator[](std::size_t i) {
        return _entries.at(i);
    }
}
#endif //PICOSKATE_NEOPIXEL_GRADIENT_HPP
