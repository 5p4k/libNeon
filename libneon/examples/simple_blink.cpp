#include <neo/led.hpp>
#include <neo/rmt.hpp>
#include <neo/strip.hpp>

static constexpr rmt_channel_t rmt_channel = RMT_CHANNEL_0;
static constexpr gpio_num_t strip_gpio_pin = GPIO_NUM_19;
static constexpr std::size_t strip_num_leds = 24;

extern "C" void app_main() {
    neo::rmt_manager manager{neo::make_rmt_config(rmt_channel, strip_gpio_pin), true};
    neo::strip<neo::grb_led> strip{manager, neo::controller::ws2812_800khz, strip_num_leds};

    if (const auto err = strip.transmit(manager, true); err != ESP_OK) {
        ESP_LOGE("NEO", "Trasmit failed with status %s", esp_err_to_name(err));
    }
    while (true) {
        for (unsigned i = 0; i < strip_num_leds; ++i) {
            strip[(i + strip_num_leds - 1) % strip_num_leds] = {0, 0, 0};
            strip[i] = {255, 255, 255};
            ESP_LOGI("NEO", "Setting led %d to full brightness.", i);
            if (const auto err = strip.transmit(manager, true); err != ESP_OK) {
                ESP_LOGE("NEO", "Trasmit failed with status %s", esp_err_to_name(err));
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}
