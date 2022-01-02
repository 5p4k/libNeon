//
// Created by spak on 5/28/21.
//

#ifndef NEO_STRIP_HPP
#define NEO_STRIP_HPP

#include <cmath>
#include <limits>
#include <neo/color.hpp>
#include <neo/gamma.hpp>
#include <neo/rmt.hpp>
#include <vector>

namespace neo {

    template <class OutputIt>
    OutputIt populate(OutputIt it, std::uint8_t b, rmt_item32_s zero, rmt_item32_s one);

    template <class OutputIt, class T, class = std::enable_if_t<not std::is_polymorphic_v<T>>>
    OutputIt populate(OutputIt it, T const &t, rmt_item32_s zero, rmt_item32_s one);

    template <class Led>
    class ref;

    template <class Led>
    class cref;

    class transmittable_rgb_strip {
    public:
        [[nodiscard]] virtual esp_err_t update(std::vector<rgb> const &colors, rmt_channel_t channel, bool wait_tx_done) = 0;
        [[nodiscard]] virtual esp_err_t update(rgb color, rmt_channel_t channel, bool wait_tx_done) = 0;
        [[nodiscard]] virtual std::size_t size() const = 0;

        virtual ~transmittable_rgb_strip() = default;
    };

    template <class Led>
    class strip final : public transmittable_rgb_strip {
        rmt_item32_s _zero;
        rmt_item32_s _one;
        float _gamma;
        gamma_table const *_gamma_table;
        std::vector<Led> _pixels;
        std::vector<rmt_item32_s> _rmt_buffer;

    public:
        static constexpr float default_gamma = 2.8f;
        static constexpr float no_gamma = std::numeric_limits<float>::quiet_NaN();

        static_assert(std::has_unique_object_representations_v<Led>, "The Led class will be transmitted directly");

        explicit strip(std::pair<rmt_item32_s, rmt_item32_s> zero_one, std::size_t size = 0, float gamma = default_gamma);

        strip(rmt_manager const &manager, controller chip, std::size_t size = 0, bool inverted = false, float gamma = default_gamma);

        [[nodiscard]] esp_err_t update(std::vector<rgb> const &colors, rmt_channel_t channel, bool wait_tx_done) override;
        [[nodiscard]] esp_err_t update(rgb color, rmt_channel_t channel, bool wait_tx_done) override;

        [[nodiscard]] esp_err_t transmit(rmt_channel_t channel, bool wait_tx_done) const;

        template <class Neopix, class CRTPIt>
        class iterator_base;

        class const_iterator;

        class iterator;

        [[nodiscard]] inline rmt_item32_s zero() const;

        [[nodiscard]] inline rmt_item32_s one() const;

        [[nodiscard]] inline std::size_t size() const override;

        [[nodiscard]] inline bool empty() const;

        [[nodiscard]] inline float gamma() const;

        [[nodiscard]] inline gamma_table const *get_gamma_table() const;

        inline void set_gamma(float gamma);

        void clear();

        inline void resize(std::size_t new_size);
        void resize(std::size_t new_size, Led led);

        [[nodiscard]] inline ref<Led> operator[](std::size_t i);

        [[nodiscard]] inline cref<Led> operator[](std::size_t i) const;

        [[nodiscard]] inline const_iterator begin() const;

        [[nodiscard]] inline const_iterator cbegin() const;

        [[nodiscard]] inline const_iterator end() const;

        [[nodiscard]] inline const_iterator cend() const;

        [[nodiscard]] inline iterator begin();

        [[nodiscard]] inline iterator end();
    };


    template <class Led>
    template <class Neopix, class CRTPIt>
    class strip<Led>::iterator_base {
    protected:
        Neopix *_neopix = nullptr;
        std::size_t _i = std::numeric_limits<std::size_t>::max();

    public:
        iterator_base() = default;

        iterator_base(Neopix &neopix, std::size_t i) : _neopix{&neopix}, _i{i} {}

        [[nodiscard]] inline decltype(auto) operator*() const;

        inline CRTPIt &operator++();

        inline const CRTPIt operator++(int);

        inline CRTPIt &operator--();

        inline const CRTPIt operator--(int);

        [[nodiscard]] inline bool operator==(CRTPIt const &other) const;

        [[nodiscard]] inline bool operator!=(CRTPIt const &other) const;

        inline CRTPIt &operator+=(std::size_t n);

        inline CRTPIt &operator-=(std::size_t n);
    };


    template <class Led>
    class strip<Led>::const_iterator : public iterator_base<strip const, const_iterator> {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = decltype(std::declval<strip const &>()[0]);
        using difference_type = void;
        using pointer = void;
        using reference = void;

        using iterator_base<strip const, const_iterator>::iterator_base;
    };


    template <class Led>
    class strip<Led>::iterator : public iterator_base<strip, iterator> {
    protected:
        using iterator_base<strip, iterator>::_neopix;
        using iterator_base<strip, iterator>::_i;

    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = decltype(std::declval<strip &>()[0]);
        using difference_type = void;
        using pointer = void;
        using reference = void;

        using iterator_base<strip, iterator>::iterator_base;

        inline operator const_iterator() const;
    };

    template <class Led>
    class cref {
    protected:
        strip<Led> const &_strip;
        Led const &_cref;

    public:
        cref(strip<Led> const &neo, Led const &cref);

        [[nodiscard]] inline explicit operator Led() const;

        [[nodiscard]] rgb get_color() const;
    };


    template <class Led>
    class ref : public cref<Led> {
        std::vector<rmt_item32_s>::iterator _repr_it;
        using cref<Led>::_strip;
        using cref<Led>::_cref;

        [[nodiscard]] inline Led &get_ref() { return const_cast<Led &>(_cref); }

    public:
        explicit ref(strip<Led> const &neo, Led &ref, std::vector<rmt_item32_s>::iterator repr_it);

        using cref<Led>::operator Led;
        using cref<Led>::get_color;

        void set_color(rgb c);

        ref &operator=(Led v);
        inline ref &operator=(rgb c);
    };

}// namespace neo

namespace neo {

    template <class Led>
    strip<Led>::strip(rmt_manager const &manager, controller chip, std::size_t size, bool inverted, float gamma)
        : _zero{},
          _one{},
          _gamma{no_gamma},
          _gamma_table{nullptr} {
        auto [zero, one] = make_zero_one(manager, chip, inverted);
        _zero = zero;
        _one = one;
        set_gamma(gamma);
        if (size > 0) {
            resize(size);
        }
    }

    template <class Led>
    strip<Led>::strip(std::pair<rmt_item32_s, rmt_item32_s> zero_one, std::size_t size, float gamma)
        : _zero{zero_one.first},
          _one{zero_one.second},
          _gamma{no_gamma},
          _gamma_table{nullptr} {
        set_gamma(gamma);
        if (size > 0) {
            resize(size);
        }
    }


    template <class Led>
    float strip<Led>::gamma() const {
        return _gamma;
    }

    template <class Led>
    gamma_table const *strip<Led>::get_gamma_table() const {
        return _gamma_table;
    }

    template <class Led>
    void strip<Led>::set_gamma(float gamma) {
        _gamma = gamma;
        _gamma_table = std::isnan(gamma) ? nullptr : &get_cached_gamma_table(gamma);
    }

    template <class Led>
    std::size_t strip<Led>::size() const {
        return _pixels.size();
    }

    template <class Led>
    void strip<Led>::resize(std::size_t new_size) {
        resize(new_size, Led{});
    }

    template <class Led>
    void strip<Led>::resize(std::size_t new_size, Led led) {
        const auto old_size = size();
        _pixels.resize(new_size, led);
        _rmt_buffer.resize(new_size * 8 * sizeof(Led), zero());
        if (new_size > old_size) {
            for (std::size_t i = old_size; i < new_size; ++i) {
                (*this)[i] = led;
            }
        }
    }

    template <class Led>
    void strip<Led>::clear() {
        _pixels.clear();
        _rmt_buffer.clear();
    }

    template <class Led>
    template <class Neopix, class CRTPIt>
    decltype(auto) strip<Led>::iterator_base<Neopix, CRTPIt>::operator*() const {
        return (*_neopix)[_i];
    }


    template <class Led>
    template <class Neopix, class CRTPIt>
    CRTPIt &strip<Led>::iterator_base<Neopix, CRTPIt>::operator++() {
        ++_i;
        return *reinterpret_cast<CRTPIt *>(this);
    }

    template <class Led>
    template <class Neopix, class CRTPIt>
    const CRTPIt strip<Led>::iterator_base<Neopix, CRTPIt>::operator++(int) {
        CRTPIt copy = *reinterpret_cast<CRTPIt *>(this);
        ++(*this);
        return copy;
    }

    template <class Led>
    template <class Neopix, class CRTPIt>
    CRTPIt &strip<Led>::iterator_base<Neopix, CRTPIt>::operator--() {
        --_i;
        return *reinterpret_cast<CRTPIt *>(this);
    }

    template <class Led>
    template <class Neopix, class CRTPIt>
    const CRTPIt strip<Led>::iterator_base<Neopix, CRTPIt>::operator--(int) {
        CRTPIt copy = *reinterpret_cast<CRTPIt *>(this);
        --(*this);
        return copy;
    }


    template <class Led>
    template <class Neopix, class CRTPIt>
    bool strip<Led>::iterator_base<Neopix, CRTPIt>::operator==(CRTPIt const &other) const {
        auto const &other_base = static_cast<iterator_base<Neopix, CRTPIt> const &>(other);
        if (_neopix == other_base._neopix) {
            if (_neopix != nullptr) {
                return _i == other_base._i;
            }
            return true;
        }
        return false;
    }

    template <class Led>
    template <class Neopix, class CRTPIt>
    bool strip<Led>::iterator_base<Neopix, CRTPIt>::operator!=(CRTPIt const &other) const {
        return not operator==(other);
    }


    template <class Led>
    template <class Neopix, class CRTPIt>
    CRTPIt &strip<Led>::iterator_base<Neopix, CRTPIt>::operator+=(std::size_t n) {
        _i += n;
        return *this;
    }


    template <class Led>
    template <class Neopix, class CRTPIt>
    CRTPIt &strip<Led>::iterator_base<Neopix, CRTPIt>::operator-=(std::size_t n) {
        _i -= n;
        return *this;
    }

    template <class Led>
    strip<Led>::iterator::operator const_iterator() const {
        if (_neopix == nullptr) {
            return const_iterator{};
        }
        return const_iterator{*_neopix, _i};
    }

    template <class Led>
    typename strip<Led>::const_iterator strip<Led>::begin() const {
        return {*this, 0};
    }

    template <class Led>
    typename strip<Led>::const_iterator strip<Led>::cbegin() const {
        return {*this, 0};
    }


    template <class Led>
    typename strip<Led>::const_iterator strip<Led>::end() const {
        return {*this, size() - 1};
    }


    template <class Led>
    typename strip<Led>::const_iterator strip<Led>::cend() const {
        return {*this, size() - 1};
    }


    template <class Led>
    typename strip<Led>::iterator strip<Led>::begin() {
        return {*this, 0};
    }


    template <class Led>
    typename strip<Led>::iterator strip<Led>::end() {
        return {*this, size() - 1};
    }

    template <class Led>
    ref<Led> strip<Led>::operator[](std::size_t i) {
        using diff_t = typename decltype(_rmt_buffer)::difference_type;
        return ref{*this, _pixels.at(i), std::begin(_rmt_buffer) + diff_t(i * 8 * sizeof(Led))};
    }

    template <class Led>
    cref<Led> strip<Led>::operator[](std::size_t i) const {
        return cref{*this, _pixels.at(i)};
    }

    template <class Led>
    bool strip<Led>::empty() const {
        return _pixels.empty();
    }

    template <class Led>
    cref<Led>::operator Led() const {
        return _cref;
    }

    template <class Led>
    rgb cref<Led>::get_color() const {
        return _cref.get_color(_strip.get_gamma_table());
    }

    template <class Led>
    cref<Led>::cref(strip<Led> const &neo, Led const &cref) : _strip{neo}, _cref{cref} {}

    template <class Led>
    ref<Led>::ref(strip<Led> const &neo, Led &ref, std::vector<rmt_item32_s>::iterator repr_it)
        : cref<Led>{neo, ref},
          _repr_it{repr_it} {}


    template <class Led>
    void ref<Led>::set_color(rgb c) {
        get_ref().set_color(c, _strip.get_gamma_table());
        // Make sure to copy the byte representation
        populate(_repr_it, Led(*this), _strip.zero(), _strip.one());
    }

    template <class Led>
    ref<Led> &ref<Led>::operator=(rgb c) {
        set_color(c);
        return *this;
    }

    template <class Led>
    ref<Led> &ref<Led>::operator=(Led v) {
        get_ref() = v;
        // Make sure to copy the byte representation
        populate(_repr_it, Led(*this), _strip.zero(), _strip.one());
        return *this;
    }

    template <class OutputIt>
    OutputIt populate(OutputIt it, std::uint8_t b, rmt_item32_s zero, rmt_item32_s one) {
        for (std::uint8_t mask = 1 << 7; mask != 0; mask >>= 1) {
            *(it++) = (b & mask) != 0 ? one : zero;
        }
        return it;
    }

    template <class OutputIt, class T, class>
    OutputIt populate(OutputIt it, T const &t, rmt_item32_s zero, rmt_item32_s one) {
        auto const *byte_rep = reinterpret_cast<std::uint8_t const *>(&t);
        for (std::size_t i = 0; i < sizeof(T); ++i, ++byte_rep) {
            it = populate(it, *byte_rep, zero, one);
        }
        return it;
    }


    template <class Led>
    rmt_item32_s strip<Led>::zero() const {
        return _zero;
    }

    template <class Led>
    rmt_item32_s strip<Led>::one() const {
        return _one;
    }

    template <class Led>
    esp_err_t strip<Led>::transmit(rmt_channel_t channel, bool wait_tx_done) const {
        return rmt_write_items(channel, _rmt_buffer.data(), _rmt_buffer.size(), wait_tx_done);
    }

    template <class Led>
    esp_err_t strip<Led>::update(std::vector<rgb> const &colors, rmt_channel_t channel, bool wait_tx_done) {
        if (size() != colors.size()) {
            resize(colors.size(), Led{});
        }
        std::copy(std::begin(colors), std::end(colors), std::begin(*this));
        return transmit(channel, wait_tx_done);
    }

    template <class Led>
    esp_err_t strip<Led>::update(rgb color, rmt_channel_t channel, bool wait_tx_done) {
        for (auto led : *this) {
            led = color;
        }
        return transmit(channel, wait_tx_done);
    }

}// namespace neo

#endif//NEO_STRIP_HPP
