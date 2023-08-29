#include <neo/alarm.hpp>
#include <neo/encoder.hpp>
#include <neo/fx.hpp>
#include <neo/gradient.hpp>

static constexpr gpio_num_t strip_gpio_pin = GPIO_NUM_13;
static constexpr std::size_t strip_num_leds = 24;

using namespace std::chrono_literals;
using namespace neo::literals;

extern "C" void app_main() {
    neo::led_encoder encoder{neo::encoding::ws2812b, neo::make_rmt_config(strip_gpio_pin)};

    const auto rainbow_fx = neo::wrap(neo::gradient_fx{
            {0xff0000_rgb, 0xffff00_rgb, 0x00ff00_rgb, 0x00ffff_rgb, 0x0000ff_rgb, 0xff00ff_rgb, 0xff0000_rgb},
            5s});

    neo::alarm alarm{30_fps, rainbow_fx->make_callback(encoder, strip_num_leds)};
    alarm.start();

    vTaskSuspend(nullptr);
}
