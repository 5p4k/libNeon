//
// Created by spak on 5/1/23.
//

#ifndef LIBNEON_RANGES_HPP
#define LIBNEON_RANGES_HPP

#include <ranges>
#include <neo/math.hpp>
#include <neo/traits.hpp>
#include <neo/gradient.hpp>

namespace neo::ranges {

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

    struct sample : public adaptor<sample> {
        unsigned count;
        blend_fn_t blend_fn;
        constexpr explicit sample(unsigned count_, blend_fn_t blend_fn_ = blend_linear) : count{count_}, blend_fn{blend_fn_} {}

        /**
         * @todo Make constexpr with C++23
         */
        template <class R>
        auto operator()(R &&r) const requires std::ranges::viewable_range<R> and std::ranges::sized_range<R> {
            if constexpr (has_position_key<std::ranges::range_value_t<R>>) {
                auto norm_r = r | normalize;
                std::vector<std::pair<float, srgb>> gradient_normalized{std::begin(norm_r), std::end(norm_r)};
                return gradient_sample_normalized(std::move(gradient_normalized), count, blend_fn);
            } else {
                return operator()(unit_enumerate_view<R>{std::forward<R>(r)});
            }
        }
    };

    template <class Fn, class ...Args> requires std::is_invocable_v<Fn, srgb, srgb, Args...>
    struct broadcast_fn {
        Fn fn{};
        std::tuple<Args...> args{};

        using value_type = std::result_of_t<Fn(Args...)>;

        broadcast_fn() = default;

        explicit broadcast_fn(Fn fn_, Args ...args_) : fn{std::move(fn_)}, args{std::move(args_)...} {}

    private:
        template <std::size_t ...Is>
        auto invoke(std::index_sequence<Is...>, srgb c1, srgb c2) {
            return fn(c1, c2, std::get<Is>(args)...);
        }
    public:
        auto operator()(srgb c1, srgb c2) {
            return invoke(std::index_sequence_for<Args...>{}, c1, c2);
        }
    };

    template <class It1, class It2, class Fn, class ...Args>
    struct broadcast_view_iterator {
        broadcast_fn<Fn, Args...> *fn;
        It1 it1;
        It2 it2;

        using iterator_category = std::forward_iterator_tag;
        using difference_type   = void;
        using value_type        = broadcast_fn<Fn, Args...>::value_type;
        using pointer           = void;
        using reference         = void;

        constexpr value_type operator*() const {
            return (*fn)(*it1, *it2);
        }

        constexpr broadcast_view_iterator &operator++() { ++it1, ++it2; return *this; }
        constexpr broadcast_view_iterator  operator++(int) { auto copy = *this; ++*this; return copy; }

        template <class Jt1, class Jt2> constexpr bool operator==(broadcast_view_iterator<Jt1, Jt2, Fn, Args...> const &i) const { return fn == i.fn and it1 == i.it1 and it2 == i.it2; }
        template <class Jt1, class Jt2> constexpr bool operator!=(broadcast_view_iterator<Jt1, Jt2, Fn, Args...> const &i) const { return fn != i.fn or it1 != i.it1 or it2 != i.it2; }
    };

    template <class R1, class R2, class Fn, class ...Args>
    requires std::ranges::viewable_range<R1> and std::ranges::sized_range<R1> and std::ranges::viewable_range<R2> and std::ranges::sized_range<R2>
    struct broadcast_view {
        broadcast_fn<Fn, Args...> fn;
        R1 r1;
        R2 r2;

        broadcast_view(Fn fn_, R1 &&r1_, R2 &&r2_, Args... args_) : fn{std::move(fn_), std::move(args_)...}, r1{std::forward<R1>(r1)}, r2{std::forward<R1>(r2)}
        {
            assert(std::ranges::size(r1_) == std::ranges::size(r2_));
        }

        auto begin() { return broadcast_view_iterator{&fn, std::begin(r1), std::begin(r2)}; }
        auto end() { return broadcast_view_iterator{&fn, std::end(r1), std::end(r2)}; }
    };


}

#endif//LIBNEON_RANGES_HPP
