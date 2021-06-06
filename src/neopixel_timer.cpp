//
// Created by spak on 6/7/21.
//

#include "neopixel_timer.hpp"
#include <esp_log.h>


namespace neo {
    static constexpr auto timer_timeout = pdMS_TO_TICKS(100);

    timer::timer() : _name{}, _elapsed{}, _last_fire{std::numeric_limits<TickType_t>::max()}, _timer{nullptr} {}

    timer &timer::operator=(timer &&t) noexcept {
        if (t.is_active() or is_active()) {
            ESP_LOGW("TIMER", "Moving active timer, let us hope it does not fire now");
        }
        // Change the timer id to point to this
        if (t._timer) {
            vTimerSetTimerID(t._timer, this);
        }
        if (_timer) {
            vTimerSetTimerID(_timer, &t);
        }
        std::swap(t._name, _name);
        std::swap(t._elapsed, _elapsed);
        std::swap(t._timer, _timer);
        return *this;
    }

    timer::timer(timer &&t) noexcept : timer{} {
        *this = std::move(t);
    }

    timer::timer(std::string name_, std::chrono::milliseconds period, bool start) :
        _name{std::move(name_)},
        _elapsed{0},
        _last_fire{std::numeric_limits<TickType_t>::max()},
        _timer{nullptr}
    {
        _timer = xTimerCreate(name().c_str(), pdMS_TO_TICKS(period.count()), pdTRUE, this, &callback);
        if (_timer == nullptr) {
            ESP_LOGE("TIMER", "Could not create timer \"%s\".", name().c_str());
        } else if (start) {
            this->start();
        }
    }

    void timer::start() {
        if (_timer != nullptr) {
            if (xTimerStart(_timer, timer_timeout) != pdPASS) {
                ESP_LOGE("TIMER", "Could not start timer \"%s\".", name().c_str());
            }
        }
    }

    void timer::callback(TimerHandle_t hdl) {
        if (auto *instance = reinterpret_cast<timer *>(pvTimerGetTimerID(hdl)); instance != nullptr) {
            instance->callback();
        } else {
            ESP_LOGE("TIMER", "Null timer instance.");
        }
    }

    void timer::callback() {
        // Update the last fired event and add the extra elapsed time
        const auto now = xTaskGetTickCount();
        if (_last_fire != std::numeric_limits<TickType_t>::max()) {

        }
        _last_fire = xTaskGetTickCount();
#warning Use chrono instead
    }

    void timer::stop() {
        if (_timer != nullptr) {
            if (xTimerStop(_timer, timer_timeout) != pdPASS) {
                ESP_LOGE("TIMER", "Could not stop timer \"%s\".", name().c_str());
            }
        }
    }

    void timer::reset() {
        if (_timer != nullptr) {
            if (xTimerReset(_timer, timer_timeout) != pdPASS) {
                ESP_LOGE("TIMER", "Could not reset timer \"%s\".", name().c_str());
            }
        }
    }

    void timer::reset(std::chrono::milliseconds new_period) {
        if (_timer != nullptr) {
            const bool active = is_active();
            if (xTimerChangePeriod(_timer, pdMS_TO_TICKS(new_period.count()), timer_timeout) != pdPASS) {
                ESP_LOGE("TIMER", "Could not change period of timer \"%s\".", name().c_str());
            }
        }
    }

    std::chrono::milliseconds timer::period() const {
        if (_timer) {
            return std::chrono::milliseconds{pdTICKS_TO_MS(xTimerGetPeriod(_timer))};
        }
        return {};
    }

    bool timer::is_active() const {
        return _timer != nullptr and xTimerIsTimerActive(_timer);
    }

    timer::~timer() {
        if (_timer) {
            xTimerDelete(_timer, pdMS_TO_TICKS(timer_timeout));
        }
    }
}