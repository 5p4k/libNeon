//
// Created by spak on 6/7/21.
//

#include "neopixel_timer.hpp"
#include <esp_log.h>


namespace neo {

    namespace {
        constexpr std::array<std::array<const char *, TIMER_MAX>, TIMER_GROUP_MAX> timer_desc{{
              {{"NEO Timer 0:0", "NEO Timer 0:1"}},
              {{"NEO Timer 1:0", "NEO Timer 1:1"}}
        }};
    }

    generic_timer::generic_timer(std::chrono::milliseconds period,
                                 std::function<void(generic_timer &)> cbk, BaseType_t core_affinity,
                                 timer_idx_t timer_idx,
                                 timer_group_t timer_group) :
            _cfg{timer_config_default},
            _group{timer_group},
            _idx{timer_idx},
            _core_affinity{core_affinity},
            _cbk{std::move(cbk)},
            _cbk_task{nullptr},
            _period{period},
            _active{false}
    {
        ESP_LOGD("TIMER", "Creating timer %d:%d with period %lld", group(), index(), period.count());
        ESP_ERROR_CHECK(timer_init(group(), index(), &_cfg));
        ESP_ERROR_CHECK(timer_set_counter_value(group(), index(), 0));
        ESP_ERROR_CHECK(timer_set_alarm_value(group(), index(), get_alarm_value(this->period())));
        setup_callbacks();
    }

    generic_timer &generic_timer::operator=(generic_timer &&t) noexcept {
        // Signal that a move operation is in place. This RAII object upon destruction will swap the content of
        // the corresponding trackers.
        auto hold_move = t.swap(*this);
        std::swap(_cfg, t._cfg);
        std::swap(_group, t._group);
        std::swap(_idx, t._idx);
        std::swap(_core_affinity, t._core_affinity);
        std::swap(_cbk, t._cbk);
        std::swap(_cbk_task, t._cbk_task);
        std::swap(_period, t._period);
        std::swap(_active, t._active);
        return *this;
    }

    generic_timer::generic_timer(generic_timer &&t) noexcept: generic_timer{} {
        *this = std::move(t);
    }

    void generic_timer::cbk_task_body(void *tracker) {
        if (auto *instance = mlab::uniquely_tracked::track<generic_timer>(tracker); instance != nullptr) {
            ESP_LOGD("TIMER", "Timer %d:%d running on core %d.", instance->group(), instance->index(), xPortGetCoreID());
            while (instance->_cbk_task != nullptr /* it's a me :D */) {
                if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) != 0) {
                    instance->_cbk(*instance);
                }
            }
        } else {
            ESP_LOGE("TIMER", "Null generic_timer instance in callback body.");
        }
    }

    void generic_timer::setup_callbacks() {
        if (_cbk_task != nullptr) {
            return;
        }
        // Setup first the callback to the timer
        ESP_ERROR_CHECK(timer_isr_callback_add(group(), index(), &isr_callback, this, 0));
        // Setup a pinned task that will call the callback, leaving the ISR callback nice and free
        const auto res = xTaskCreatePinnedToCore(
                &cbk_task_body,
                timer_desc[group()][index()],
                CONFIG_ESP_TIMER_TASK_STACK_SIZE,
                this,
                3 | portPRIVILEGE_BIT,
                &_cbk_task,
                core_affinity());
        if (res != pdPASS) {
            ESP_LOGE("TIMER", "Unable to create pinned timer task, error %d.", res);
            _cbk_task = nullptr;
            return;
        }
    }

    void generic_timer::teardown_callbacks() {
        if (_cbk_task == nullptr) {
            return;
        }
        // Stop first the callback
        ESP_ERROR_CHECK(timer_isr_callback_remove(group(), index()));
        // And now delete the task
        vTaskDelete(_cbk_task);
        _cbk_task = nullptr;
    }

    bool generic_timer::is_valid_timer() const {
        return group() < TIMER_GROUP_MAX and index() < TIMER_MAX;
    }

    void generic_timer::start() {
        if (is_valid_timer()) {
            ESP_LOGD("TIMER", "Starting timer %d:%d", group(), index());
            ESP_ERROR_CHECK(timer_start(group(), index()));
            _active = true;
        }
    }

    bool generic_timer::isr_callback(void *tracker) {
        if (auto *instance = mlab::uniquely_tracked::track<generic_timer>(tracker); instance != nullptr) {
            BaseType_t high_task_awoken = pdFALSE;
            vTaskNotifyGiveFromISR(instance->_cbk_task, &high_task_awoken);
            return high_task_awoken == pdTRUE; // Return whether we need to yield at the end of ISR
        }
        return pdFALSE;
    }

    std::uint64_t generic_timer::get_alarm_value(std::chrono::milliseconds period) {
        return period.count() * std::uint64_t(generic_timer_base_frequency / 1'000);
    }

    void generic_timer::stop() {
        if (is_valid_timer()) {
            ESP_LOGD("TIMER", "Stopping timer %d:%d", group(), index());
            ESP_ERROR_CHECK(timer_pause(group(), index()));
            _active = false;
        }
    }

    void generic_timer::reset() {
        if (is_valid_timer()) {
            ESP_LOGD("TIMER", "Resetting timer %d:%d", group(), index());
            ESP_ERROR_CHECK(timer_set_counter_value(group(), index(), 0));
        }
    }

    generic_timer::~generic_timer() {
        if (is_valid_timer()) {
            ESP_LOGD("TIMER", "Destroying timer %d:%d", group(), index());
            teardown_callbacks();
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