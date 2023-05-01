//
// Created by spak on 4/28/23.
//

#ifndef LIBNEON_ENCODER_HPP
#define LIBNEON_ENCODER_HPP

#include <chrono>
#include <driver/gpio.h>
#include <driver/rmt_types.h>
#include <driver/rmt_tx.h>
#include <mlab/bin_data.hpp>
#include <neo/channel.hpp>


namespace neo {

    using namespace std::chrono_literals;

    constexpr rmt_tx_channel_config_t default_rmt_config{
            .gpio_num = -1,
            .clk_src = RMT_CLK_SRC_DEFAULT,
            .resolution_hz = 40'000'000,// 40MHz
            .mem_block_symbols = 1024,  // Recommended value when using dma
            .trans_queue_depth = 2,     // We should be safe with at most two transactions
            .flags = {.invert_out = false,
                      .with_dma = true,
                      .io_loop_back = false,
                      .io_od_mode = false}};


    struct encoding_spec {
        std::chrono::nanoseconds t0h;
        std::chrono::nanoseconds t0l;
        std::chrono::nanoseconds t1h;
        std::chrono::nanoseconds t1l;
        std::chrono::nanoseconds res;
        channel_sequence chn_seq;
    };

    struct encoding {
        channel_sequence chn_seq;
        rmt_bytes_encoder_config_t rmt_encoder_cfg;
        rmt_symbol_word_t rmt_reset_sym;

        constexpr encoding(std::chrono::nanoseconds t0h, std::chrono::nanoseconds t0l,
                           std::chrono::nanoseconds t1h, std::chrono::nanoseconds t1l,
                           channel_sequence chn_seq_,
                           std::chrono::nanoseconds res = 500ns,
                           std::uint32_t resolution_hz = default_rmt_config.resolution_hz,
                           bool msb_first = true)
            : chn_seq{chn_seq_},
              rmt_encoder_cfg{.bit0 = {.duration0 = std::uint32_t(t0h.count()) * resolution_hz / 1'000'000'000,
                                       .level0 = 1,
                                       .duration1 = std::uint32_t(t0l.count()) * resolution_hz / 1'000'000'000,
                                       .level1 = 0},
                              .bit1 = {.duration0 = std::uint32_t(t1h.count()) * resolution_hz / 1'000'000'000,
                                       .level0 = 1,
                                       .duration1 = std::uint32_t(t1l.count()) * resolution_hz / 1'000'000'000,
                                       .level1 = 0},
                              .flags = {.msb_first = msb_first}},
              rmt_reset_sym{.duration0 = std::uint32_t(res.count()) * resolution_hz / 1'000'000'000, .level0 = 0, .duration1 = 0, .level1 = 0} {}

        constexpr encoding(encoding_spec spec) : encoding{spec.t0h, spec.t0l, spec.t1h, spec.t1l, spec.chn_seq, spec.res} {}

        static constexpr encoding_spec ws2812b{400ns, 850ns, 800ns, 450ns, 50us, "grb"};
        static constexpr encoding_spec ws2812{350ns, 700ns, 800ns, 600ns, 50us, "grb"};
        static constexpr encoding_spec ws2811{500ns, 1200ns, 2000ns, 1300ns, 50us, "rgb"};
    };

    class led_encoder : private rmt_encoder_t {
        rmt_encoder_handle_t _bytes_encoder;
        rmt_encoder_handle_t _tail_encoder;
        rmt_symbol_word_t _reset_sym;
        channel_sequence _chn_seq;
        rmt_channel_handle_t _rmt_chn;
        mlab::bin_data _buffer;

        static std::size_t _encode(rmt_encoder_t *encoder, rmt_channel_handle_t tx_channel, const void *primary_data, std::size_t data_size, rmt_encode_state_t *ret_state);
        static esp_err_t _reset(rmt_encoder_t *encoder);
        static esp_err_t _del(rmt_encoder_t *);

        std::size_t encode(rmt_channel_handle_t tx_channel, const void *primary_data, std::size_t data_size, rmt_encode_state_t *ret_state);
        esp_err_t reset();

    public:
        led_encoder();

        led_encoder(led_encoder const &) = delete;
        led_encoder &operator=(led_encoder const &) = delete;

        led_encoder(led_encoder &&) noexcept = default;
        led_encoder &operator=(led_encoder &&) noexcept = default;

        explicit led_encoder(encoding enc, gpio_num_t gpio);

        esp_err_t transmit_raw(mlab::range<std::uint8_t const *> data);

        template <class ColorIterator>
        esp_err_t transmit(ColorIterator begin, ColorIterator end);

        ~led_encoder();
    };


}// namespace neo


namespace neo {


    template <class ColorIterator>
    esp_err_t led_encoder::transmit(ColorIterator begin, ColorIterator end) {
        _buffer.clear();
        _buffer.reserve(std::distance(begin, end));
        _chn_seq.extract(begin, end, std::back_inserter(_buffer));
        return transmit_raw(_buffer.data_view());
    }

}// namespace neo

#endif//LIBNEON_ENCODER_HPP
