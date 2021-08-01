//
// Created by spak on 6/7/21.
//

#include "neopixel_timer.hpp"
#include <esp_log.h>


namespace neo {

    generic_timer::generic_timer(std::chrono::milliseconds period,
                                 std::function<void(generic_timer &)> cbk, BaseType_t core_affinity,
                                 timer_idx_t timer_idx,
                                 timer_group_t timer_group) :
            _cfg{timer_config_default},
            _group{timer_group},
            _idx{timer_idx},
            _core_affinity{core_affinity},
            _cbk{std::move(cbk)},
            _semaphore{nullptr},
            _cbk_task{nullptr},
            _period{period},
            _active{false} {
        ESP_ERROR_CHECK(timer_init(group(), index(), &_cfg));
        ESP_ERROR_CHECK(timer_set_counter_value(group(), index(), 0));
        ESP_ERROR_CHECK(timer_set_alarm_value(group(), index(), get_alarm_value(this->period())));
        kickoff_cbk_task();
    }

    generic_timer &generic_timer::operator=(generic_timer &&t) noexcept {
        if (is_valid_timer()) {
            delete_cbk_task();
        }
        if (t.is_valid_timer()) {
            t.delete_cbk_task();
        }
        std::swap(_cfg, t._cfg);
        std::swap(_group, t._group);
        std::swap(_idx, t._idx);
        std::swap(_core_affinity, t._core_affinity);
        std::swap(_cbk, t._cbk);
        std::swap(_semaphore, t._semaphore);
        std::swap(_cbk_task, t._cbk_task);
        std::swap(_period, t._period);
        std::swap(_active, t._active);
        if (t.is_valid_timer()) {
            t.kickoff_cbk_task();
        }
        if (is_valid_timer()) {
            kickoff_cbk_task();
        }
        return *this;
    }

    generic_timer::generic_timer(generic_timer &&t) noexcept: generic_timer{} {
        *this = std::move(t);
    }

    void generic_timer::cbk_task_body(void *arg) {
        static constexpr auto ms_0p1_in_ticks = configTICK_RATE_HZ / 10'000;
        if (auto *instance = reinterpret_cast<generic_timer *>(arg); instance != nullptr) {
            ESP_LOGD("TIMER", "Timer running on core %d.",  xPortGetCoreID());
            while (instance->_cbk_task != nullptr /* it's a me :D */) {
                if (xSemaphoreTake(instance->_semaphore, ms_0p1_in_ticks) == pdTRUE) {
                    instance->_cbk(*instance);
                }
            }
        } else {
            ESP_LOGE("TIMER", "Null generic_timer instance in callback body.");
        }
    }

    void generic_timer::kickoff_cbk_task() {
        if (_cbk_task != nullptr) {
            return;
        }
        _semaphore = xSemaphoreCreateBinary();
        configASSERT(_semaphore);
        const auto res = xTaskCreatePinnedToCore(
                &cbk_task_body,
                "Generic timer task",
                CONFIG_ESP_TIMER_TASK_STACK_SIZE,
                this,
                (configMAX_PRIORITIES - 5) | portPRIVILEGE_BIT,
                &_cbk_task,
                core_affinity());
        if (res != pdPASS) {
            ESP_LOGE("TIMER", "Unable to create pinned timer task, error %d.", res);
            _cbk_task = nullptr;
            return;
        }
    }

    void generic_timer::delete_cbk_task() {
        if (_cbk_task == nullptr) {
            return;
        }
        vTaskDelete(_cbk_task);
        _cbk_task = nullptr;
        vPortFree(_semaphore);
        _semaphore = nullptr;
    }

    bool generic_timer::is_valid_timer() const {
        return group() < TIMER_GROUP_MAX and index() < TIMER_MAX;
    }

    void generic_timer::start() {
        if (is_valid_timer()) {
            ESP_ERROR_CHECK(timer_start(group(), index()));
            _active = true;
        }
    }

    bool generic_timer::isr_callback(void *arg) {
        if (auto *instance = reinterpret_cast<generic_timer *>(arg); instance != nullptr) {
            BaseType_t high_task_awoken = pdFALSE;
            xSemaphoreGiveFromISR(instance->_semaphore, &high_task_awoken);
            return high_task_awoken == pdTRUE; // Return whether we need to yield at the end of ISR
        } else {
            ESP_LOGE("TIMER", "Null generic_timer instance.");
            return pdFALSE;
        }
    }

    std::uint64_t generic_timer::get_alarm_value(std::chrono::milliseconds period) {
        return period.count() * std::uint64_t(generic_timer_base_frequency / 1'000);
    }

    void generic_timer::stop() {
        if (is_valid_timer()) {
            ESP_ERROR_CHECK(timer_pause(group(), index()));
            _active = false;
        }
    }

    void generic_timer::reset() {
        if (is_valid_timer()) {
            ESP_ERROR_CHECK(timer_set_counter_value(group(), index(), 0));
        }
    }

    generic_timer::~generic_timer() {
        if (is_valid_timer()) {
            delete_cbk_task();
            ESP_ERROR_CHECK(timer_deinit(group(), index()));
        }
    }

    steady_timer::steady_timer() : generic_timer{}, _last_start{}, _previous_laps_duration{} {}

    void steady_timer::elapsed_callback(generic_timer &gen_timer) {


    }

    steady_timer::steady_timer(std::chrono::milliseconds period, std::function<void(std::chrono::milliseconds)> cbk,
                               BaseType_t core_affinity, timer_idx_t timer_idx, timer_group_t timer_group) :
            generic_timer{period,
                          [elapsed_cbk = std::move(cbk)](generic_timer &gt) {
                              elapsed_cbk(reinterpret_cast<steady_timer *>(&gt)->elapsed());
                          },
                          core_affinity, timer_idx, timer_group},
            _last_start{std::chrono::steady_clock::now()},
            _previous_laps_duration{0ms}
    {}

    void steady_timer::start() {
        if (not is_active()) {
            _last_start = std::chrono::steady_clock::now();
            generic_timer::start();
        }
    }

    std::chrono::milliseconds steady_timer::elapsed_since_last_start() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - _last_start);
    }

    std::chrono::milliseconds steady_timer::elapsed() const {
        return is_active() ? _previous_laps_duration + elapsed_since_last_start() : _previous_laps_duration;
    }

    void steady_timer::stop() {
        if (is_active()) {
            _previous_laps_duration = elapsed();
            generic_timer::stop();
        }
    }

    void steady_timer::reset() {
        _previous_laps_duration = 0ms;
        _last_start = std::chrono::steady_clock::now();
        generic_timer::reset();
    }

}