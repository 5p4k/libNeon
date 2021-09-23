//
// Created by spak on 6/3/21.
//

#ifndef PICOSKATE_NEOPIXEL_LED_HPP
#define PICOSKATE_NEOPIXEL_LED_HPP

#include "neopixel_color.hpp"
#include <array>
#include <algorithm>

namespace neo {

    enum struct led_channel {
        red,
        green,
        blue
    };

    template <led_channel Channel>
    struct channel_storage {
        std::uint8_t value = 0;
    };

    template <led_channel Channel, led_channel ...Channels>
    struct channel_pack : public channel_storage<Channel>, public channel_pack<Channels...> {
    };

    template <led_channel Channel>
    struct channel_pack<Channel> : public channel_storage<Channel> {};

    template <led_channel ...Channels>
    class led : private channel_pack<Channels...> {
    public:
        led() = default;

        inline led(rgb c, gamma_table const *table);

        template <led_channel Channel>
        void set(std::uint8_t v);
        void set(led_channel chn, std::uint8_t v);

        template <led_channel Channel>
        [[nodiscard]] inline std::uint8_t get() const;
        [[nodiscard]] inline std::uint8_t get(led_channel chn) const;

        void set_color(rgb c, gamma_table const *table);

        /**
         * @note Very slow because it does inverse gamma table lookup. Also not guaranteed to invert @ref set_color,
         * because a discretized gamma function is not bijective.
         */
        [[nodiscard]] rgb get_color(gamma_table const *table) const;
    };

    using grb_led = led<led_channel::green, led_channel::red, led_channel::blue>;
    using rgb_led = led<led_channel::red, led_channel::green, led_channel::blue>;

}

namespace neo {

    template <led_channel ...Channels>
    template <led_channel Channel>
    void led<Channels...>::set(std::uint8_t v) {
        channel_storage<Channel>::value = v;
    }

    template <led_channel ...Channels>
    template <led_channel Channel>
    std::uint8_t led<Channels...>::get() const {
        return channel_storage<Channel>::value;
    }

    template <led_channel ...Channels>
    void led<Channels...>::set(led_channel chn, std::uint8_t v) {
        switch (chn) {
            case led_channel::red:
                set<led_channel::red>(v);
                break;
            case led_channel::green:
                set<led_channel::green>(v);
                break;
            case led_channel::blue:
                set<led_channel::blue>(v);
                break;
            default:
                break;
        }
    }

    template <led_channel ...Channels>
    std::uint8_t led<Channels...>::get(led_channel chn) const {
        switch (chn) {
            case led_channel::red:
                return get<led_channel::red>();
            case led_channel::green:
                return get<led_channel::green>();
            case led_channel::blue:
                return get<led_channel::blue>();
            default:
                return 0;
        }
    }

    template <led_channel ...Channels>
    void led<Channels...>::set_color(rgb c, const gamma_table *table) {
        if (table != nullptr) {
            set<led_channel::red>((*table)[c.r]);
            set<led_channel::green>((*table)[c.g]);
            set<led_channel::blue>((*table)[c.b]);
        } else {
            set<led_channel::red>(c.r);
            set<led_channel::green>(c.g);
            set<led_channel::blue>(c.b);
        }
    }

    template <led_channel ...Channels>
    led<Channels...>::led(rgb c, gamma_table const *table) : led{} {
        set_color(c, table);
    }

    template <led_channel ...Channels>
    rgb led<Channels...>::get_color(gamma_table const *table) const {
        if (table != nullptr) {
            return {table->reverse_lookup(get<led_channel::red>()),
                    table->reverse_lookup(get<led_channel::green>()),
                    table->reverse_lookup(get<led_channel::blue>())};
        } else {
            return {get<led_channel::red>(),
                    get<led_channel::green>(),
                    get<led_channel::blue>()};
        }
    }

}

#endif //PICOSKATE_NEOPIXEL_LED_HPP
