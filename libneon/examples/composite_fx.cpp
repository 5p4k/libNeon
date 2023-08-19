#include <esp_random.h>
#include <neo/alarm.hpp>
#include <neo/encoder.hpp>
#include <neo/fx.hpp>
#include <neo/gradient.hpp>
#include <thread>

static constexpr gpio_num_t strip_gpio_pin = GPIO_NUM_13;
static constexpr std::size_t strip_num_leds = 24;

using namespace std::chrono_literals;
using namespace neo::literals;

extern "C" [[noreturn]] void app_main() {
    const std::array<std::shared_ptr<neo::fx_base>, 4> all_fx = {
            // Rainbow:
            neo::wrap(neo::gradient_fx{{0xff0000_rgb, 0xffff00_rgb, 0x00ff00_rgb, 0x00ffff_rgb, 0x0000ff_rgb, 0xff00ff_rgb, 0xff0000_rgb}, 5s}),
            // Spinner:
            neo::wrap(neo::gradient_fx{{0x0_rgb, 0xffffff_rgb}, 2s, 2.f}),
            // Pulse red-yellow:
            neo::wrap(neo::pulse_fx{neo::solid_fx{0xff0000_rgb}, neo::solid_fx{0xffff00_rgb}, 2s}),
            // Pulse blue:
            neo::wrap(neo::pulse_fx{neo::solid_fx{0x0000ff_rgb}, neo::solid_fx{0x000000_rgb}, 2s})};

    auto fx_transition = std::make_shared<neo::transition_fx>();
    auto fx_mask = neo::wrap(neo::blend_fx{fx_transition, neo::solid_fx{0x0_rgb}, 0.75f});

    neo::led_encoder encoder{neo::encoding::ws2812b, neo::make_rmt_config(strip_gpio_pin)};
    neo::alarm alarm{30_fps, fx_mask->make_callback(encoder, strip_num_leds)};
    alarm.start();

    std::this_thread::sleep_for(1s);

    for (std::size_t i = 0; true; i = (i + 1) % all_fx.size()) {
        ESP_LOGI("NEO", "Switching to fx no. %d", i);
        fx_transition->transition_to(alarm, all_fx[i], 2s);
        std::this_thread::sleep_for(10s);
    }
}
