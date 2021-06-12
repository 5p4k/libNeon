//
// Created by spak on 6/7/21.
//

#ifndef PICOSKATE_NEOPIXEL_TIMER_HPP
#define PICOSKATE_NEOPIXEL_TIMER_HPP

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <string>
#include <chrono>
#include <functional>

namespace neo {

    class generic_timer {
        std::string _name;
        TimerHandle_t _timer;
        std::function<void(generic_timer &)> _cbk;

        static void callback(TimerHandle_t hdl);

    public:
        generic_timer();

        generic_timer(std::string name_, std::chrono::milliseconds period, std::function<void(generic_timer &)> cbk);

        generic_timer(generic_timer const &) = delete;

        generic_timer &operator=(generic_timer const &) = delete;

        generic_timer &operator=(generic_timer &&t) noexcept;

        generic_timer(generic_timer &&t) noexcept;

        virtual ~generic_timer();

        [[nodiscard]] inline std::string const &name() const;

        [[nodiscard]] std::chrono::milliseconds period() const;

        [[nodiscard]] bool is_active() const;

        void invoke_callback();

        virtual void start();

        virtual void stop();

        /**
         * @note Will @ref start the timer.
         */
        virtual void reset();

        /**
         * @note Will @ref start the timer.
         */
        virtual void reset(std::chrono::milliseconds new_period);

        inline explicit operator bool() const;
    };

    class steady_timer : private generic_timer {
        std::chrono::time_point<std::chrono::steady_clock> _last_start;
        std::chrono::milliseconds _previous_laps_duration;

        static void elapsed_callback(generic_timer &gen_timer);
        [[nodiscard]] std::chrono::milliseconds elapsed_since_last_start() const;
    public:
        
        using generic_timer::operator bool;
        using generic_timer::is_active;
        using generic_timer::name;
        using generic_timer::period;

        steady_timer();

        steady_timer(std::string name_, std::chrono::milliseconds period,
                     std::function<void(std::chrono::milliseconds)> cbk);

        steady_timer(steady_timer const &) = delete;
        steady_timer(steady_timer &&) noexcept = default;

        steady_timer &operator=(steady_timer const &) = delete;
        steady_timer &operator=(steady_timer &&) noexcept = default;

        [[nodiscard]] std::chrono::milliseconds elapsed() const;

        void start() override;

        void stop() override;

        /**
         * @note Will @ref start the timer.
         */
        void reset() override;

        /**
         * @note Will @ref start the timer.
         */
        void reset(std::chrono::milliseconds new_period) override;
    };

}

namespace neo {
    const std::string &generic_timer::name() const {
        return _name;
    }

    generic_timer::operator bool() const {
        return is_active();
    }
}

#endif //PICOSKATE_NEOPIXEL_TIMER_HPP
