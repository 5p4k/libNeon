# libNeon
*Neopixel driver library using RMT, effects included. No dependencies.*

[![pipeline status](https://git.mittelab.org/5p4k/libneon/badges/master/pipeline.svg)](https://git.mittelab.org/5p4k/libneon/-/commits/master)

Main repo: https://git.mittelab.org/5p4k/libneon  
Mirror repo: https://github.com/LizardM4/libNeon  
Public issues: https://github.com/LizardM4/libNeon/issues

## Tell me how to use it quickly
Look at the examples, they should make everything clear. Customizing the code should be pretty easy too.


Unfortunately there is still no documentation and no unit testing for the library (although I've used it extensively),
but down here below I'll try to summarize the concepts and do a little tutorial.

## Basics

Supports WS2812b, WS2812, WS2811 out of the box, but if you know the timings and the channel order, you can use it with
any of them. You must wire power and ground separately, the library manages the data pin.

Color data is sent via an *encoder*, which controls the specified GPIO pin using [RMT](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/rmt.html).
This allows to reach pretty high framerates very smoothly (60fps is usually no problem), compared to
bitbanging manually the GPIO.

The encoder knows how to turn bytes into pulses that the LED strip can understand. It does not need to know how many
pixels are present.

```c++
#include <neo/encoder.hpp>

// ...

neo::led_encoder encoder{neo::encoding::ws2812b, neo::make_rmt_config(GPIO_NUM_13)};
//                       your led model ^^^^^^^         the gpio pin  ^^^^^^^^^^^
```

You can send raw data (the values for each channel) directly like this:

```c++
// Make two pixels black
const std::vector<std::uint8_t> channels = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
encoder.transmit_raw(channels);
```

but it's better to specify the colors and let the encoder do the rest:
```c++
#include <neo/color.hpp>
using namespace neo::literals;

// ...

constexpr auto black = 0x000000_rgb;  // You can use hex notation with _rgb suffix
constexpr auto red   = 0xff0000_rgb;
const std::vector<neo::srgb> colors = {black, red};
encoder.trasmit(std::begin(colors), std::end(colors), neo::srgb_linear_channel_extractor());
// This can be omitted (see below for more details):  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

## Colors

Colors are in the [sRGB](https://en.wikipedia.org/wiki/SRGB) colorspace.
Nothing special to know except that the colorspace has been chosen deliberately and that `neo::srgb::blend` will
correctly convert the channel data to linear before blending, and then reconvert back, as shown as "linear" [here](https://bottosson.github.io/posts/colorwrong/).
This results in much better looking gradients.

Also, the channel values are not supposed to be sent directly (unless somehow your LED supports raw sRGB).
For this reason, you should always pass a *channel extractor* to the `neo::encoder::transmit` method:

```c++
encoder.trasmit(std::begin(colors), std::end(colors), neo::srgb_linear_channel_extractor());

// Or you can adjust the gamma manually, that might improve the apperance
encoder.transmit(std::begin(colors), std::end(colors), neo::srgb_gamma_channel_extractor(1.2f));
```

### Other color representations
There exists support for the [HSV](https://en.wikipedia.org/wiki/HSL_and_HSV) representation of the RGB color space,
through `neo::hsv`. You can convert to HSV using `neo::srgb::to_hsv` and back to sRGB with `neo::hsv::to_rgb`.
However not much more than that is available, as RGB remains the main color representation. However, it's not fully
excluded that it will be supported as a fully-fledged color representation in the future.  

## Gradients

Gradients are sorted collections of `neo::gradient_entry`, which is just a position (float in the 0..1 range) and a
color (sRGB). If your entries are not sorted or are not in the 0..1 range, you can use `neo::gradient_normalize` to
fix that. To sample the gradient, and get back one color for each LED, you use `neo::gradient_sample`.

```c++
#include <neo/gradient.hpp>

// ...

const std::vector<neo::gradient_entry> bwb = 
        {{0.0f, 0x0_rgb}, {0.5f, 0xffffff_rgb}, {1.0f, 0x0_rgb}};

// Unnecessary, in this case:
neo::gradient_normalize(std::begin(bwb), std::end(bwb));

// Sample 24 colors:
colors.resize(24);
neo::gradient_sample(std::begin(bwb), std::end(bwb), colors.size(), std::begin(colors));

// Send:
encoder.trasmit(std::begin(colors), std::end(colors), neo::srgb_linear_channel_extractor());
```

If you want a gradient of equally spaced colors, you can spare yourself typing all the floats by using
`neo::gradient_make_uniform_from_colors`:

```c++
const std::vector<neo::gradient_entry> same_as_bwb =
        neo::gradient_make_uniform_from_colors({0x0_rgb, 0xffffff_rgb, 0x0_rgb});
```

You can repeat the gradient multiple times (even a fractional number of them) by passing a number higher than 1 to the
*scale* parameter of `neo::gradient_sample` (think of it as a *repeat* parameter), and you can rotate the gradient
in the 0..1 range (like shifting, but since it carries on to the other side, it's more of a rotation) by specifying
a value other than 0 for the *rotate* parameter. This creates nice effects for example on the NeoPixel rings.
See the `spinner_fx.cpp` example for this.

### Other blending functions
As a final note on gradients, the blending function can be customized in case you need a step gradient. Te blending
function is usually the last argument, and there are several to choose from, where the default is `neo::blend_linearly`,
which calls `neo::srgb::blend` (and thus blends in the linear sRGB space, mentioned above). Other defined blending
functions are
 - `neo::blend_linear` default sRGB blending
 - `neo::blend_lerp` blends the raw sRGB values (yields weird gradients)
 - `neo::blend_round_down` sets the color to the previous entry in the gradient
 - `neo::blend_round_up` sets the color to the next entry in the gradient
 - `neo::blend_nearest_neighbor` rounds to the closest entry in the gradient
 - You can however pass a reference to any function matching the signature  
   `neo::srgb my_blend_fn(neo::srgb l, neo::srgb r, float factor);`.

## Timers and alarms

To get anything animates, you have to transmit new colors every so often. `neo::timer` and `neo::alarm` (which is
basically a periodically firing event) wrap around [ESP-IDF's general purpose timer](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gptimer.html).

The most important method of `neo::timer` is `neo::timer::cycle_time`, which you can use to get a number in the range
0..1 to that represents the progress in a cycle of a specified duration. That is for timing cyclic effects. For example,
`timer.cycle_time(4s)` will return a number that goes from 0 to 1 every 4 seconds, and resets from 0 afterward.

The alarm instead, takes a `void(neo::alarm &)` callable that is the action to be performed every time the alarm fires.
In our case, rendering a new frame.

```c++
#include <neo/encoder.hpp>
#include <neo/gradient.hpp>
#include <neo/alarm.hpp>
#include <chrono>

using namespace std::chrono_literals;
using namespace neo::literals;

// ...
neo::encoder encoder{/* ... */};
// ...

const auto bwb = neo::gradient_make_uniform_from_colors({0x0_rgb, 0xffffff_rgb, 0x0_rgb});
std::vector<neo::srgb> colors;
colors.resize(24);

// Declare a lambda that can capture the encoder and the `bwb` gradient we defined before.
// The lambda must have a signature like `void my_function(neo::alarm &a);`.
auto render_frame = [&](neo::alarm &a) {
    // Rotate the gradient every 4s. Sample 24 leds
    neo::gradient_sample(std::begin(bwb), std::end(bwb), colors.size(), std::begin(colors),
                         a.cycle_time(4s) /* this is the 'rotate' parm */);
    encoder.transmit(std::begin(colors), std::end(colors), neo::srgb_linear_channel_extractor());
};

neo::alarm alarm{30_fps, render_frame};
alarm.start();

// Nice, you got yourself a spinner!
vTaskSuspend(nullptr); // <- very important!
```

**Important**: in the `void app_main()` function, *after* the timer is started, you call `vTaskSuspend(nullptr)`.
This function is a [FreeRTOS](https://www.freertos.org/a00130.html) method that suspends the current task. If you do not
do so, the function will exit, the alarm destroyed, and see no effect. You can also call something like
`std::this_thread::sleep_for(100s)`.

## Pre-crafted effects

Some basic effects are ready to use, namely:
 - `neo::solix_fx` solid color (used only for blending)
 - `neo::gradient_fx` animates the *rotation* of a gradient

They can be used as building blocks for composite effects:
 - `neo::pulse_fx` bounces back and forth between two other effects
 - `neo::transition_fx` allows to smoothly change from one effect to another
 - `neo::blend_fx` blends two effects together using a fixed (not animated) factor

All effects have a convenient `make_callback(neo::led_encoder &encoder, std::size_t num_leds)` function that can be used
directly as an argument to `neo::alarm`, as follows:

```c++
#include <neo/fx.hpp>

// ...

// Reuse gradient `bwb` and `encoder` from before
const auto rotate_fx = neo::wrap(neo::gradient_fx{bwb, 5s});

// This replaces all the code above:
neo::alarm alarm{30_fps, rainbow_fx->make_callback(encoder, 24)};
alarm.start();

vTaskSuspend(nullptr);
```

The only catch is that `neo::led_encoder` must stay alive as much as the effect does, so it should not go out of scope.

If you need to set a different extractor, you can pass it as a last argument to the templated overload of
`make_callback`.

**Important:** `make_callback` uses the method `shared_from_this()` of `std::enable_shared_from_this`. This means you
**must** wrap the effect into a `std::shared_ptr` **before** calling `make_callback`, otherwise you will trigger a
segmentation fault.

### Some technical details
All effects inherit from `neo::fx_base`, and need to implement only one function:

```c++
void my_effect::populate(neo::alarm const &a, neo::color_range colors) override;
```

This function must populate `colors` and can use `a` to compute those colors. For example, `neo::gradient_fx` calls
`neo::gradient_sample` with `a.cycle_time(rotate_cycle_time)`, like we did above.
Composite effects call the respective `populate` method of their sub-effects and combine them. Composite effects must
thus manage their own buffer if they need intermediate storage.

Due to the fact that composite effects require other sub-effects to stay alive, and to the fact that `neo::fx_base` is
abstract, all effects **must be used through `std::shared_ptr`**, so that dependency can be tracked effectively, and
leaks avoided.

**Note 1:** since some templated arguments in the constructor can be only resolved when explicitly called, the function
`neo::wrap` is made available so that the syntax can be shortened a bit:

```c++
// Painfully long:
auto my_fx1 = std::make_shared<neo::gradient_fx>(std::vector<neo::srgb>{0x0_rgb, 0xff0000_rgb});
// Sad but a bit better:
auto my_fx2 = neo::wrap(neo::gradient_fx{{0x0_rgb, 0xff0000_rgb}});
```

**Note 2:** to avoid having to wrap every single effect, composite effects should always take template parameters
constrained on the concept `neo::fx_or_fx_ptr`, which matches a subclass of `neo::fx_base`, or a `std::shared_ptr` to
such class. It can then be piped into `neo::wrap` within the constructor to ensure it's cast to a
`std::shared_ptr<neo::fx_base>`:

```c++
// Painfully long:
auto my_fx1 = neo::wrap(neo::pulse_fx{neo::wrap(neo::solid_fx{0x0_rgb}), neo::wrap(neo::solid_fx{0x7fc0c2_rgb}), 4s});
// Mildly better:
auto my_fx2 = neo::wrap(neo::pulse_fx{neo::solid_fx{0x0_rgb}, neo::solid_fx{0x7fc0c2_rgb}, 4s});
```
 
### Helpers

When blending two colors with any function, it might be useful to employ `neo::broadcast_blend`. This is the somewhat
restricted C++ version of Python's `map` and `zip` function:

```c++

template <class FwdIt1, class FwdIt2, class OutIt>
OutIt broadcast_blend(FwdIt1 l_begin, FwdIt1 l_end, FwdIt2 r_begin, FwdIt2 r_end, OutIt out, float t, blend_fn_t blend_fn = blend_linear);
```

It takes two ranges, `L = [l_begin, l_end)` and `R = [r_begin, r_end)`, and collects in `out` the result of the call
`blend_fn(l, r, t)`, where `l` belongs to `L` (respectively, `r` to `R`). It basically broadcasts `blend_fn` onto
items from each range. It's probably easier to express it as

```python
def broadcast_blend(L: Iterable[sRGB],
                    R: Iterable[sRGB],
                    blend_fn: Callable[[sRGB, sRGB, float], sRGB],
                    t: float) -> Iterable[sRGB]:
    for l, r in zip(L, R):
        yield blend_fn(l, r, t)
```