#ifndef BANYA_INTERVAL_TIMER_H
#define BANYA_INTERVAL_TIMER_H

#include <Arduino.h>
#include <functional>

namespace HAL {

/**
 * @brief Non-blocking interval timer for periodic tasks
 *
 * Provides a clean way to execute code at specified intervals
 * without blocking the main loop.
 *
 * @example
 * @code
 * IntervalTimer lcdTimer(5000); // 5 seconds
 *
 * void loop() {
 *     // Option 1: Manual check
 *     if (lcdTimer.check()) {
 *         if (!lcd.isConnected()) {
 *             lcd.begin();
 *         }
 *     }
 *
 *     // Option 2: With lambda (cleaner)
 *     lcdTimer.run([]() {
 *         if (!lcd.isConnected()) {
 *             lcd.begin();
 *         }
 *     });
 * }
 * @endcode
 */
class IntervalTimer {
private:
    unsigned long interval;
    unsigned long lastTrigger;

public:
    /**
     * @brief Construct a new Interval Timer
     * @param intervalMs Interval in milliseconds
     */
    explicit IntervalTimer(unsigned long intervalMs = 1000)
        : interval(intervalMs), lastTrigger(0) {}

    /**
     * @brief Check if interval has elapsed
     * @return true if interval has elapsed (and resets timer)
     * @return false if interval has not elapsed yet
     */
    bool check() {
        unsigned long now = millis();
        if (now - lastTrigger >= interval) {
            lastTrigger = now;
            return true;
        }
        return false;
    }

    /**
     * @brief Run a function if interval has elapsed
     * @param func Function to execute (lambda, std::function, etc.)
     * @return true if function was executed
     * @return false if interval has not elapsed yet
     */
    bool handleLoop(std::function<void()> func) {
        if (check()) {
            func();
            return true;
        }
        return false;
    }

    /**
     * @brief Check if interval has elapsed without resetting
     * @return true if interval has elapsed
     * @return false if interval has not elapsed yet
     */
    bool isReady() const {
        return millis() - lastTrigger >= interval;
    }

    /**
     * @brief Reset the timer (restart interval)
     */
    void reset() {
        lastTrigger = millis();
    }

    /**
     * @brief Force trigger the timer (as if interval elapsed)
     */
    void force() {
        lastTrigger = 0;
    }

    /**
     * @brief Set a new interval
     * @param intervalMs New interval in milliseconds
     */
    void setInterval(unsigned long intervalMs) {
        interval = intervalMs;
    }

    /**
     * @brief Get the current interval
     * @return Interval in milliseconds
     */
    unsigned long getInterval() const {
        return interval;
    }

    /**
     * @brief Get time remaining until next trigger
     * @return Milliseconds remaining (0 if ready)
     */
    unsigned long remaining() const {
        unsigned long elapsed = millis() - lastTrigger;
        if (elapsed >= interval) {
            return 0;
        }
        return interval - elapsed;
    }
};

} // namespace HAL

#endif // BANYA_INTERVAL_TIMER_H
