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

    const auto spinner_fx = neo::wrap(neo::gradient_fx{{{0.0, 0x0_rgb}, {0.1, 0x9999999_rgb}, {0.2, 0x444444_rgb}, {0.9, 0x0_rgb}}, 5s, 2.});

    neo::alarm alarm{30_fps, spinner_fx->make_callback(encoder, strip_num_leds)};
    alarm.start();

    vTaskSuspend(nullptr);
}
