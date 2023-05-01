//
// Created by spak on 6/3/21.
//

#include <neo/rmt_legacy.hpp>

#include <chrono>
#include <esp_log.h>

#define RMT_NEOPX_TAG "RMT-NEOPX"


namespace neo {

    namespace timings {
        using namespace std::chrono_literals;
        using nanosec = std::chrono::nanoseconds;

        static constexpr auto ws2812_0h = 400ns;
        static constexpr auto ws2812_0l = 850ns;
        static constexpr auto ws2812_1h = 800ns;
        static constexpr auto ws2812_1l = 450ns;

        static constexpr auto ws2811_0h = 500ns;
        static constexpr auto ws2811_0l = 2000ns;
        static constexpr auto ws2811_1h = 1200ns;
        static constexpr auto ws2811_1l = 1300ns;

        [[nodiscard]] rmt_item32_t make_rmt_item_bit(nanosec h, nanosec l, std::uint32_t hz, bool inverted) {
            return rmt_item32_t{{{.duration0 = std::uint32_t(double(h.count()) * hz * 1.e-9),
                                  .level0 = inverted ? 0u : 1u,
                                  .duration1 = std::uint32_t(double(l.count()) * hz * 1.e-9),
                                  .level1 = inverted ? 1u : 0u}}};
        }
    }// namespace timings

    static constexpr auto rmt_config_neopixel_default = rmt_config_t{
            .rmt_mode = RMT_MODE_TX,
            .channel = RMT_CHANNEL_MAX,// Replace with actual channel
            .gpio_num = GPIO_NUM_MAX,  // Replace with actual GPIO
            .clk_div = 2,
            .mem_block_num = 1,
            .flags = 0,
            .tx_config = {
                    .carrier_freq_hz = 38000,
                    .carrier_level = RMT_CARRIER_LEVEL_HIGH,
                    .idle_level = RMT_IDLE_LEVEL_LOW,
                    .carrier_duty_percent = 33,
                    .carrier_en = false,
                    .loop_en = false,
                    .idle_output_en = true,
            }};

    const char *to_string(controller c) {
        switch (c) {
            case controller::ws2811_400khz:
                return "WS2811 (400kHz)";
            case controller::ws2812_800khz:
                return "WS2812 (800kHz)";
            default:
                return "UNKNOWN";
        }
    }

    rmt_config_t make_rmt_config(rmt_channel_t channel, gpio_num_t gpio) {
        rmt_config_t retval = rmt_config_neopixel_default;
        retval.channel = channel;
        retval.gpio_num = gpio;
        return retval;
    }

    std::pair<rmt_item32_t, rmt_item32_t> make_zero_one(rmt_manager const &manager, controller chip, bool inverted) {
        const auto clock_hz = manager.get_clock_hertz();
        switch (chip) {
            case controller::ws2811_400khz:
                return {timings::make_rmt_item_bit(timings::ws2811_0h, timings::ws2811_0l, clock_hz, inverted),
                        timings::make_rmt_item_bit(timings::ws2811_1h, timings::ws2811_1l, clock_hz, inverted)};
            case controller::ws2812_800khz:
                return {timings::make_rmt_item_bit(timings::ws2812_0h, timings::ws2812_0l, clock_hz, inverted),
                        timings::make_rmt_item_bit(timings::ws2812_1h, timings::ws2812_1l, clock_hz, inverted)};
            default:
                return {{},
                        {}};
        }
    }


    rmt_manager::rmt_manager(rmt_config_t config, bool manage_driver) : _channel{config.channel},
                                                                        _manage_driver{manage_driver} {
        if (_manage_driver) {
            if (const auto err = rmt_driver_install(config.channel, 0, 0); err != ESP_OK) {
                ESP_LOGE(RMT_NEOPX_TAG, "rmt_driver_install: failed with status %s", esp_err_to_name(err));
                _manage_driver = false;
                _channel = RMT_CHANNEL_MAX;
                return;
            }
        }
        // Attempt at configuration
        if (const auto err = rmt_config(&config); err != ESP_OK) {
            ESP_LOGE(RMT_NEOPX_TAG, "rmt_config: failed with status %s", esp_err_to_name(err));
            _manage_driver = false;
            _channel = RMT_CHANNEL_MAX;
            return;
        }
    }
    rmt_manager::rmt_manager(rmt_manager &&other) noexcept {
        *this = std::move(other);
    }

    rmt_manager &rmt_manager::operator=(rmt_manager &&other) noexcept {
        std::swap(_channel, other._channel);
        std::swap(_manage_driver, other._manage_driver);
        return *this;
    }


    std::uint32_t rmt_manager::get_clock_hertz() const {
        std::uint32_t retval = 0;
        if (const auto err = rmt_get_counter_clock(*this, &retval); err != ESP_OK) {
            ESP_LOGE(RMT_NEOPX_TAG, "rmt_get_counter_clock: failed with status %s", esp_err_to_name(err));
        }
        return retval;
    }

    rmt_manager::~rmt_manager() {
        if (_manage_driver and *this < RMT_CHANNEL_MAX) {
            rmt_driver_uninstall(*this);
            _manage_driver = false;
            _channel = RMT_CHANNEL_MAX;
        }
    }

}// namespace neo