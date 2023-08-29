#include <neo/alarm.hpp>
#include <neo/encoder.hpp>
#include <neo/gradient.hpp>

static constexpr gpio_num_t strip_gpio_pin = GPIO_NUM_13;
static constexpr std::size_t strip_num_leds = 24;

using namespace std::chrono_literals;
using namespace neo::literals;

extern "C" void app_main() {
    neo::led_encoder encoder{neo::encoding::ws2812b, neo::make_rmt_config(strip_gpio_pin)};

    const auto rainbow = neo::gradient_make_uniform_from_colors(
            {0xff0000_rgb, 0xffff00_rgb, 0x00ff00_rgb, 0x00ffff_rgb, 0x0000ff_rgb, 0xff00ff_rgb, 0xff0000_rgb});

    std::array<neo::srgb, strip_num_leds> colors{};
    neo::gradient_sample(std::begin(rainbow), std::end(rainbow), colors.size(), std::begin(colors));

    ESP_ERROR_CHECK(encoder.transmit(std::begin(colors), std::end(colors), neo::srgb_linear_channel_extractor()));

    vTaskSuspend(nullptr);
}
