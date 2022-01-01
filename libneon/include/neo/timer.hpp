//
// Created by spak on 6/7/21.
//

#ifndef PICOSKATE_NEOPIXEL_TIMER_HPP
#define PICOSKATE_NEOPIXEL_TIMER_HPP

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <driver/timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <string>
#include <chrono>
#include <functional>
#include <mlab/unique_tracker.hpp>

namespace neo {

    namespace {
        using namespace std::chrono_literals;
    }

    class generic_timer : public mlab::uniquely_tracked {

        /**
         * We have the timer in ms, but we cannot specify a sufficiently large divider
         * to get 1KHz, so we operate at 2KHz.
         */
        static constexpr std::uint32_t generic_timer_base_frequency = 2'000;

        static constexpr timer_config_t timer_config_default{
                .alarm_en = TIMER_ALARM_EN,
                .counter_en = TIMER_PAUSE,
                .intr_type = TIMER_INTR_LEVEL,
                .counter_dir = TIMER_COUNT_UP,
                .auto_reload = TIMER_AUTORELOAD_EN,
                .divider = TIMER_BASE_CLK / generic_timer_base_frequency
        };

        timer_config_t _cfg = timer_config_default;
        timer_group_t _group = TIMER_GROUP_MAX;
        timer_idx_t _idx = TIMER_MAX;
        BaseType_t _core_affinity = tskNO_AFFINITY;
        std::function<void(generic_timer &)> _cbk = nullptr;
        TaskHandle_t _cbk_task = nullptr;
        std::chrono::milliseconds _period = 0ms;
        bool _active = false;

        static bool IRAM_ATTR isr_callback(void *tracker);

        static void cbk_task_body(void *tracker);

        [[nodiscard]] static std::uint64_t get_alarm_value(std::chrono::milliseconds period);

        [[nodiscard]] inline timer_group_t group() const;

        [[nodiscard]] inline timer_idx_t index() const;

        [[nodiscard]] inline BaseType_t core_affinity() const;

        [[nodiscard]] inline bool is_valid_timer() const;

        void setup_callbacks();

        void teardown_callbacks();

    public:
        generic_timer() = default;

        generic_timer(std::chrono::milliseconds period, std::function<void(generic_timer &)> cbk,
                      BaseType_t core_affinity = tskNO_AFFINITY, timer_idx_t timer_idx = TIMER_0,
                      timer_group_t timer_group = TIMER_GROUP_0);

        generic_timer(generic_timer const &) = delete;

        generic_timer &operator=(generic_timer const &) = delete;

        generic_timer &operator=(generic_timer &&t) noexcept;

        generic_timer(generic_timer &&t) noexcept;

        virtual ~generic_timer();

        [[nodiscard]] inline std::chrono::milliseconds period() const;

        [[nodiscard]] inline bool is_active() const;

        virtual void start();

        virtual void stop();

        virtual void reset();

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
        using generic_timer::period;

        steady_timer();

        steady_timer(std::chrono::milliseconds period, std::function<void(std::chrono::milliseconds)> cbk,
                     BaseType_t core_affinity = tskNO_AFFINITY, timer_idx_t timer_idx = TIMER_0,
                     timer_group_t timer_group = TIMER_GROUP_0);

        steady_timer(steady_timer const &) = delete;

        steady_timer(steady_timer &&) noexcept = default;

        steady_timer &operator=(steady_timer const &) = delete;

        steady_timer &operator=(steady_timer &&) noexcept = default;

        [[nodiscard]] std::chrono::milliseconds elapsed() const;

        void start() override;

        void stop() override;

        void reset() override;

    };

}

namespace neo {

    timer_group_t generic_timer::group() const {
        return _group;
    }

    timer_idx_t generic_timer::index() const {
        return _idx;
    }

    BaseType_t generic_timer::core_affinity() const {
        return _core_affinity;
    }

    std::chrono::milliseconds generic_timer::period() const {
        return _period;
    }

    bool generic_timer::is_active() const {
        return _active;
    }

    generic_timer::operator bool() const {
        return is_active();
    }
}

#endif //PICOSKATE_NEOPIXEL_TIMER_HPP
