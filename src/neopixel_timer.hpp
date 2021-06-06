//
// Created by spak on 6/7/21.
//

#ifndef PICOSKATE_NEOPIXEL_TIMER_HPP
#define PICOSKATE_NEOPIXEL_TIMER_HPP

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <string>
#include <chrono>

namespace neo {
    class timer {
        std::string _name;
        std::chrono::milliseconds _elapsed;
        TickType_t _last_fire;
        TimerHandle_t _timer;

        static void callback(TimerHandle_t hdl);
        void callback();
    public:
        timer();
        timer(std::string name_, std::chrono::milliseconds period, bool start = false);
        timer(timer const &) = delete;
        timer &operator=(timer const &) = delete;
        timer &operator=(timer &&t) noexcept;
        timer(timer &&t) noexcept;
        ~timer();

        [[nodiscard]] inline std::string const &name() const;
        [[nodiscard]] std::chrono::milliseconds period() const;
        [[nodiscard]] bool is_active() const;

        void start();
        void stop();
        void reset();
        void reset(std::chrono::milliseconds new_period);

        inline explicit operator bool() const;
    };
}

namespace neo {
    const std::string & timer::name() const {
        return _name;
    }

    timer::operator bool() const {
        return is_active();
    }
}

#endif //PICOSKATE_NEOPIXEL_TIMER_HPP
