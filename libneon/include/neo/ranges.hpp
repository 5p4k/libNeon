//
// Created by spak on 5/1/23.
//

#ifndef LIBNEON_RANGES_HPP
#define LIBNEON_RANGES_HPP

#include <ranges>
#include <neo/math.hpp>

namespace neo::ranges {

    template <class T, class F>
    struct is_pair_with_first : public std::false_type {};

    template <class F, class X>
    struct is_pair_with_first<std::pair<F, X>, F> : public std::true_type {};

    template <class T> concept has_position_key = is_pair_with_first<std::remove_cvref_t<T>, float>::value;

    /**
     * @note Until we have C++23, mimic `range_adaptor_closure`.
     */
    template <class Derived>
    struct adaptor {
        template <class Self, class Range>
            requires std::derived_from<std::remove_cvref_t<Self>, adaptor> and std::is_invocable_v<Self, Range>
        friend constexpr auto operator|(Range &&r, Self &&self) {
            return std::forward<Self>(self)(std::forward<Range>(r));
        }
    };

    template <class Iterator>
    struct unit_enumerate_iterator {
        Iterator inner;
        Iterator begin;
        std::size_t size;

        using iterator_category = std::iterator_traits<Iterator>::iterator_category;
        using difference_type   = std::iterator_traits<Iterator>::difference_type;
        using value_type        = std::pair<float, typename std::iterator_traits<Iterator>::value_type>;
        using pointer           = void;
        using reference         = void;

        constexpr value_type operator*() const {
            return {float(std::ranges::distance(begin, inner) / float(size - 1)), *inner};
        }

        constexpr unit_enumerate_iterator &operator++() { ++inner; return *this; }
        constexpr unit_enumerate_iterator  operator++(int) { auto copy = *this; ++*this; return copy; }

        constexpr unit_enumerate_iterator &operator--() { --inner; return *this; }
        constexpr unit_enumerate_iterator  operator--(int) { auto copy = *this; --*this; return copy; }

        constexpr unit_enumerate_iterator &operator+=(difference_type x) { inner += x; return *this; }
        constexpr unit_enumerate_iterator &operator-=(difference_type x) { inner -= x; return *this; }

        constexpr decltype(auto) operator[](difference_type n) const { auto copy = *this; copy += n; return *copy; }

        template <class Jterator> constexpr bool operator==(unit_enumerate_iterator<Jterator> const &i) const { return inner == i.inner; }
        template <class Jterator> constexpr bool operator!=(unit_enumerate_iterator<Jterator> const &i) const { return inner != i.inner; }

        constexpr bool operator<(unit_enumerate_iterator const &i) const { return inner < i.inner; }
        constexpr bool operator>(unit_enumerate_iterator const &i) const { return inner > i.inner; }
        constexpr bool operator<=(unit_enumerate_iterator const &i) const { return inner <= i.inner; }
        constexpr bool operator>=(unit_enumerate_iterator const &i) const { return inner >= i.inner; }
        constexpr auto operator<=>(unit_enumerate_iterator const &i) const { return inner <=> i.inner; }

        friend constexpr unit_enumerate_iterator operator+(unit_enumerate_iterator const &i, difference_type n) { return {i.inner + n, i.begin, i.size}; }
        friend constexpr unit_enumerate_iterator operator+(difference_type n, unit_enumerate_iterator const &i) { return {n + i.inner, i.begin, i.size}; }
        friend constexpr unit_enumerate_iterator operator-(unit_enumerate_iterator const &i, difference_type n) { return {i.inner - n, i.begin, i.size}; }
        friend constexpr difference_type operator-(unit_enumerate_iterator const &x, unit_enumerate_iterator const &y) { return x.inner - y.inner;}
    };

    template <std::ranges::sized_range R>
    struct unit_enumerate_view : public std::ranges::view_interface<unit_enumerate_view<R>> {
        using iterator = unit_enumerate_iterator<std::ranges::iterator_t<R>>;

        R inner = {};

        constexpr unit_enumerate_view() = default;
        constexpr explicit unit_enumerate_view(R rg) : inner{std::forward<R>(rg)} {}

        auto begin() { return iterator{std::ranges::begin(inner), std::ranges::begin(inner), std::ranges::size(inner)}; }
        auto end() { return iterator{std::ranges::end(inner), std::ranges::begin(inner), std::ranges::size(inner)}; }
    };

    struct unit_enumerate_t : public adaptor<unit_enumerate_t> {
        template <class R>
        constexpr auto operator()(R &&r) const {
            return unit_enumerate_view<std::remove_cvref_t<R>>{std::forward<R>(r)};
        }
    };

    constexpr auto unit_enumerate = unit_enumerate_t{};

    struct normalize_t : public adaptor<normalize_t> {
        template <class R>
        constexpr auto operator()(R &&r) const {
            if constexpr (has_position_key<std::ranges::range_value_t<std::remove_cvref_t<R>>>) {
                using kvp_t = std::remove_cvref_t<std::ranges::range_value_t<R>>;
                const auto lo = std::ranges::min(r, {}, &kvp_t::first).first;
                const auto hi = std::ranges::max(r, {}, &kvp_t::first).first;
                return std::ranges::transform_view(std::forward<R>(r), [=](auto &&kvp) {
                    return kvp_t{(kvp.first - lo) / (hi - lo), kvp.second};
                });
            } else {
                return operator()(unit_enumerate_view<R>{std::forward<R>(r)});
            }
        }
    };

    constexpr auto normalize = normalize_t{};

    struct rotate : public adaptor<rotate> {
        float dt;
        constexpr explicit rotate(float dt_) : dt{dt_} {}

        template <class R>
        constexpr auto operator()(R &&r) const requires std::ranges::viewable_range<R> and std::ranges::sized_range<R> {
            if constexpr (has_position_key<std::ranges::range_value_t<R>>) {
                using kvp_t = std::remove_cvref_t<std::ranges::range_value_t<R>>;
                return std::ranges::transform_view(std::forward<R>(r), [=, this](auto &&kvp) {
                    return kvp_t{kvp.first + dt, kvp.second};
                });
            } else {
                return operator()(unit_enumerate_view<R>{std::forward<R>(r)});
            }
        }
    };


}

#endif//LIBNEON_RANGES_HPP
