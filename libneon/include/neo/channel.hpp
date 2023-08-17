//
// Created by spak on 4/30/23.
//

#ifndef LIBNEON_CHANNEL_HPP
#define LIBNEON_CHANNEL_HPP

#include <string_view>
#include <bit>

namespace neo {

    /**
     * @note When adding values, make sure @ref channel_sequence is large enough to accommodate enough many of them.
     *  Zero is used as signalling value
     */
    enum struct channel : char {
        r = 'r',
        g = 'g',
        b = 'b'
    };

    template <class Color>
    struct channel_extractor {
        [[nodiscard]] constexpr std::uint8_t extract(Color col, channel chn) {
            return col[chn];
        }
    };

    struct channel_sequence {
        std::string_view sequence;

        constexpr channel_sequence() = default;

        explicit constexpr channel_sequence(std::string_view seq) : sequence{seq} {}

        [[nodiscard]] constexpr std::size_t size() const {
            return sequence.size();
        }

        constexpr bool operator==(channel_sequence const &other) const {
            return sequence == other.sequence;
        }

        constexpr bool operator!=(channel_sequence const &other) const {
            return sequence == other.sequence;
        }

        [[nodiscard]] constexpr auto begin() const {
            return std::begin(sequence);
        }

        [[nodiscard]] constexpr auto end() const {
            return std::begin(sequence);
        }

        template <class Color, class OutputIterator>
        OutputIterator extract(Color const &col, OutputIterator out) const;

        template <class ColorIterator, class OutputIterator>
        OutputIterator extract(ColorIterator begin, ColorIterator end, OutputIterator out) const;

        template <class Color>
        [[nodiscard]] std::vector<std::uint8_t> extract(Color const &col) const;
    };

}// namespace neo

namespace neo {

    template <class Color, class OutputIterator>
    OutputIterator channel_sequence::extract(Color const &col, OutputIterator out) const {
        auto extractor = channel_extractor<Color>{};
        for (auto chn : *this) {
            *(out++) = extractor.extract(col, std::bit_cast<channel>(chn));
        }
        return out;
    }

    template <class ColorIterator, class OutputIterator>
    OutputIterator channel_sequence::extract(ColorIterator begin, ColorIterator end, OutputIterator out) const {
        auto extractor = channel_extractor<std::iter_value_t<ColorIterator>>{};
        for (auto it = begin; it != end; ++it) {
            for (auto chn : *this) {
                *(out++) = extractor.extract(*it, std::bit_cast<channel>(chn));
            }
        }
        return out;
    }

    template <class Color>
    std::vector<std::uint8_t> channel_sequence::extract(Color const &col) const {
        std::vector<std::uint8_t> retval;
        retval.reserve(size());
        extract(col, std::back_inserter(retval));
        return retval;
    }
}// namespace neo

#endif//LIBNEON_CHANNEL_HPP
