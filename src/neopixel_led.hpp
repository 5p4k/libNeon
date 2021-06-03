//
// Created by spak on 6/3/21.
//

#ifndef PICOSKATE_NEOPIXEL_LED_HPP
#define PICOSKATE_NEOPIXEL_LED_HPP
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
        inline led(std::uint32_t rgb);
        inline led(std::array<std::uint8_t, 3> rgb);
        inline led(std::initializer_list<std::uint8_t> rgb);
        inline explicit led(std::uint8_t r, std::uint8_t g, std::uint8_t b);

        template <led_channel Channel>
        void set(std::uint8_t v);
        void set(led_channel chn, std::uint8_t v);

        template <led_channel Channel>
        [[nodiscard]] inline std::uint8_t get() const;
        [[nodiscard]] inline std::uint8_t get(led_channel chn) const;
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
    led<Channels...>::led(std::uint32_t rgb) : led{
        std::uint8_t(0xff & (rgb >> 16)),
        std::uint8_t(0xff & (rgb >> 8)),
        std::uint8_t(0xff & rgb)}
    {}


    template <led_channel ...Channels>
    led<Channels...>::led(std::array<std::uint8_t, 3> rgb) : led{rgb[0], rgb[1], rgb[2]} {}

    template <led_channel ...Channels>
    led<Channels...>::led(std::initializer_list<std::uint8_t> rgb) : led{} {
        auto it = std::begin(rgb);
        auto end = std::end(rgb);
        if (it != end) {
            set<led_channel::red>(*it++);
        }
        if (it != end) {
            set<led_channel::green>(*it++);
        }
        if (it != end) {
            set<led_channel::blue>(*it++);
        }
    }

    template <led_channel ...Channels>
    led<Channels...>::led(std::uint8_t r, std::uint8_t g, std::uint8_t b) : led{} {
        set<led_channel::red>(r);
        set<led_channel::green>(g);
        set<led_channel::blue>(b);
    }
}

#endif //PICOSKATE_NEOPIXEL_LED_HPP
