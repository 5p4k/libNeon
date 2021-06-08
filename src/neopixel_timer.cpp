//
// Created by spak on 6/7/21.
//

#include "neopixel_timer.hpp"
#include <esp_log.h>


namespace neo {
    namespace {
        using namespace std::chrono_literals;
    }

    static constexpr auto timer_timeout = pdMS_TO_TICKS(100);

    generic_timer::generic_timer() : _name{}, _timer{nullptr}, _cbk{nullptr} {}

    generic_timer &generic_timer::operator=(generic_timer &&t) noexcept {
        if (t.is_active() or is_active()) {
            ESP_LOGW("TIMER", "Moving active generic_timer, let us hope it does not fire now");
        }
        // Change the generic_timer id to point to this
        if (t._timer) {
            vTimerSetTimerID(t._timer, this);
        }
        if (_timer) {
            vTimerSetTimerID(_timer, &t);
        }
        std::swap(t._name, _name);
        std::swap(t._timer, _timer);
        std::swap(t._cbk, _cbk);
        return *this;
    }

    generic_timer::generic_timer(generic_timer &&t) noexcept: generic_timer{} {
        *this = std::move(t);
    }

    generic_timer::generic_timer(std::string name_, std::chrono::milliseconds period, std::function<void(generic_timer &)> cbk,
                                 bool start) :
            _name{std::move(name_)},
            _timer{nullptr},
            _cbk{std::move(cbk)} {
        _timer = xTimerCreate(name().c_str(), pdMS_TO_TICKS(period.count()), pdTRUE, this, &callback);
        if (_timer == nullptr) {
            ESP_LOGE("TIMER", "Could not create generic_timer \"%s\".", name().c_str());
        } else if (start) {
            this->start();
        }
    }

    void generic_timer::start() {
        if (_timer != nullptr) {
            if (xTimerStart(_timer, timer_timeout) != pdPASS) {
                ESP_LOGE("TIMER", "Could not start generic_timer \"%s\".", name().c_str());
            }
        }
    }

    void generic_timer::invoke_callback() {
        if (_cbk != nullptr) {
            _cbk(*this);
        }
    }

    void generic_timer::callback(TimerHandle_t hdl) {
        if (auto *instance = reinterpret_cast<generic_timer *>(pvTimerGetTimerID(hdl)); instance != nullptr) {
            instance->invoke_callback();
        } else {
            ESP_LOGE("TIMER", "Null generic_timer instance.");
        }
    }

    void generic_timer::stop() {
        if (_timer != nullptr) {
            if (xTimerStop(_timer, timer_timeout) != pdPASS) {
                ESP_LOGE("TIMER", "Could not stop generic_timer \"%s\".", name().c_str());
            }
        }
    }

    void generic_timer::reset() {
        if (_timer != nullptr) {
            if (xTimerReset(_timer, timer_timeout) != pdPASS) {
                ESP_LOGE("TIMER", "Could not reset generic_timer \"%s\".", name().c_str());
            }
        }
    }

    void generic_timer::reset(std::chrono::milliseconds new_period) {
        if (_timer != nullptr) {
            if (xTimerChangePeriod(_timer, pdMS_TO_TICKS(new_period.count()), timer_timeout) != pdPASS) {
                ESP_LOGE("TIMER", "Could not change period of generic_timer \"%s\".", name().c_str());
            }
        }
    }

    std::chrono::milliseconds generic_timer::period() const {
        if (_timer) {
            return std::chrono::milliseconds{pdTICKS_TO_MS(xTimerGetPeriod(_timer))};
        }
        return {};
    }

    bool generic_timer::is_active() const {
        return _timer != nullptr and xTimerIsTimerActive(_timer);
    }

    generic_timer::~generic_timer() {
        if (_timer) {
            xTimerDelete(_timer, pdMS_TO_TICKS(timer_timeout));
        }
    }

    steady_timer::steady_timer() : generic_timer{}, _last_start{}, _previous_laps_duration{} {}

    void steady_timer::elapsed_callback(generic_timer &gen_timer) {


    }

    steady_timer::steady_timer(std::string name_, std::chrono::milliseconds period,
                               std::function<void(std::chrono::milliseconds)> cbk, bool start) :
            generic_timer{std::move(name_), period,
                          [elapsed_cbk = std::move(cbk)](generic_timer &gt) {
                              elapsed_cbk(reinterpret_cast<steady_timer *>(&gt)->elapsed());
                          }, start},
            _last_start{std::chrono::steady_clock::now()},
            _previous_laps_duration{0ms} {}

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

    void steady_timer::reset(std::chrono::milliseconds new_period) {
        _previous_laps_duration = 0ms;
        _last_start = std::chrono::steady_clock::now();
        generic_timer::reset(new_period);
    }

}