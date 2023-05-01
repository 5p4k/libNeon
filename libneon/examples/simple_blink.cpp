#include <neo/alarm.hpp>
#include <neo/encoder.hpp>
#include <neo/gradient.hpp>

static constexpr gpio_num_t strip_gpio_pin = GPIO_NUM_13;
static constexpr std::size_t strip_num_leds = 24;

using namespace std::chrono_literals;
using namespace neo::literals;

extern "C" void app_main() {
    neo::led_encoder encoder{neo::encoding::ws2812b, neo::make_rmt_config(strip_gpio_pin)};

    auto animate = [&, i = std::size_t{0}, buffer = std::vector<neo::rgb>{strip_num_leds}](neo::alarm &a) mutable {
        buffer[i++ % strip_num_leds] = 0x0_rgb;
        buffer[i % strip_num_leds] = 0xaaaaaa_rgb;
        ESP_ERROR_CHECK(encoder.transmit(std::begin(buffer), std::end(buffer)));
    };

    neo::alarm alarm{1_fps, animate};
    alarm.start();

    vTaskSuspend(nullptr);
}
