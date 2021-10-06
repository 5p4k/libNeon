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

    [[nodiscard]] const char *to_string(controller c);

    class rmt_manager;

    [[nodiscard]] rmt_config_t make_rmt_config(rmt_channel_t channel, gpio_num_t gpio);

    [[nodiscard]] std::pair<rmt_item32_s, rmt_item32_s> make_zero_one(rmt_manager const &manager, controller chip);

    class rmt_manager {
        rmt_channel_t _channel = RMT_CHANNEL_MAX;
        bool _manage_driver = false;
    public:
        rmt_manager() = default;
        rmt_manager(rmt_config_t config, bool manage_driver);
        rmt_manager(rmt_manager const &) = delete;
        rmt_manager &operator=(rmt_manager const &) = delete;
        rmt_manager(rmt_manager &&other) noexcept;
        rmt_manager &operator=(rmt_manager &&other) noexcept;

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
