//
// Created by spak on 8/14/23.
//

#ifndef LIBNEON_TRAITS_HPP
#define LIBNEON_TRAITS_HPP

#include <type_traits>

namespace neo {

    template <class T, class F>
    struct is_pair_with_first : public std::false_type {};

    template <class F, class X>
    struct is_pair_with_first<std::pair<F, X>, F> : public std::true_type {};

    template <class T> concept has_position_key = is_pair_with_first<std::remove_cvref_t<T>, float>::value;

}
#endif//LIBNEON_TRAITS_HPP
