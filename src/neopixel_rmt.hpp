//
// Created by spak on 5/28/21.
//

#ifndef PICOSKATE_NEOPIXEL_RMT_HPP
#define PICOSKATE_NEOPIXEL_RMT_HPP

#include <cstdint>
#include <driver/rmt.h>
#include <utility>

namespace neo {

    enum struct controller {
        ws2811_400khz,
        ws2812_800khz
    };

    class rmt_manager;

    [[nodiscard]] rmt_config_t make_rmt_config(rmt_channel_t channel, gpio_num_t gpio);

    [[nodiscard]] std::pair<rmt_item32_s, rmt_item32_s> make_zero_one(rmt_manager const &manager, controller chip);

    class rmt_manager {
        rmt_channel_t _channel;
        bool _manage_driver;
    public:
        rmt_manager(rmt_config_t config, bool manage_driver);

        [[nodiscard]] inline operator rmt_channel_t() const;

        [[nodiscard]] std::uint32_t get_clock_hertz() const;

        ~rmt_manager();
    };

}

namespace neo {

    rmt_manager::operator rmt_channel_t() const {
        return _channel;
    }

}

#endif //PICOSKATE_NEOPIXEL_RMT_HPP
