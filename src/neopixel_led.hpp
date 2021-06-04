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

        /**
         *
         * @tparam Arg0 Dirty trick to match only >= 1 parameter packs, so that we are sure to be default constructible
         * @tparam Args `(Arg0, Args...)` together can construct @ref hsv or @ref rgb
         */
        template <class Arg0, class ...Args>
        inline led(Arg0 && arg0, Args &&...args);

        template <led_channel Channel>
        void set(std::uint8_t v);
        void set(led_channel chn, std::uint8_t v);

        template <led_channel Channel>
        [[nodiscard]] inline std::uint8_t get() const;
        [[nodiscard]] inline std::uint8_t get(led_channel chn) const;

        [[nodiscard]] inline rgb rgb_color() const;
        [[nodiscard]] inline hsv hsv_color() const;
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
    rgb led<Channels...>::rgb_color() const {
        return {get<led_channel::red>(), get<led_channel::green>(), get<led_channel::blue>()};
    }

    template <led_channel ...Channels>
    hsv led<Channels...>::hsv_color() const {
        return rgb_color().to_hsv();
    }


    template <led_channel ...Channels>
    template <class Arg0, class ...Args>
    led<Channels...>::led(Arg0 &&arg0, Args &&...args) : led{} {
        if constexpr(std::is_constructible_v<rgb, Arg0, Args...>) {
            rgb color(std::forward<Arg0>(arg0), std::forward<Args>(args)...);
            set<led_channel::red>(color.r);
            set<led_channel::green>(color.g);
            set<led_channel::blue>(color.b);
        } else if constexpr(std::is_constructible_v<hsv, Arg0, Args...>) {
            rgb color = hsv(std::forward<Arg0>(arg0), std::forward<Args>(args)...).to_rgb();
            set<led_channel::red>(color.r);
            set<led_channel::green>(color.g);
            set<led_channel::blue>(color.b);
        } else {
            static_assert(std::is_constructible_v<rgb, Args...> or std::is_constructible_v<hsv, Args...>,
                          "I cannot construct neither neo::rgb nor neo::hsv with these arguments.");
        }
    }
}

#endif //PICOSKATE_NEOPIXEL_LED_HPP
