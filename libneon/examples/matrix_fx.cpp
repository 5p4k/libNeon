#include <neo/led.hpp>
#include <neo/matrix_fx.hpp>
#include <neo/rmt.hpp>
#include <neo/strip.hpp>
#include <neo/timer.hpp>

static constexpr rmt_channel_t rmt_channel = RMT_CHANNEL_0;
static constexpr gpio_num_t strip_gpio_pin = GPIO_NUM_19;
static constexpr std::size_t strip_num_leds = 24;

using namespace std::chrono_literals;
using namespace neo::literals;

extern "C" void app_main() {
    neo::rmt_manager manager{neo::make_rmt_config(rmt_channel, strip_gpio_pin), true};
    neo::strip<neo::grb_led> strip{manager, neo::controller::ws2812_800khz, strip_num_leds};
    const neo::matrix_fx fx{
            {0xff0000_rgb, 0xffff00_rgb, 0xff0000_rgb,
             0x0000ff_rgb, 0x00ffff_rgb, 0x0000ff_rgb,
             0xff0000_rgb, 0xffff00_rgb, 0xff0000_rgb},
            3,
            2s,
            2s};
    neo::steady_timer timer{16ms, fx.make_steady_timer_callback(strip, manager), 0};

    if (const auto err = strip.transmit(manager, true); err != ESP_OK) {
        ESP_LOGE("NEO", "Trasmit failed with status %s", esp_err_to_name(err));
    }

    timer.start();

    vTaskSuspend(nullptr);
}
