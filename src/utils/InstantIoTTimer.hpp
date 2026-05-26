#pragma once
/**
 * ============================================================
 * ⏱️ InstantIoTTimer.hpp - Non-blocking timer manager
 * ============================================================
 *
 * Usage:
 *   InstantTimer timers;
 *
 *   void setup() {
 *       timers.every(1000, []{ Serial.println("Every second"); });
 *       timers.once(5000, []{ Serial.println("Once only"); });
 *       timers.times(500, []{ Serial.println("3 times"); }, 3);
 *   }
 *
 *   void loop() {
 *       timers.run();
 *   }
 *
 * ============================================================
 */

#include <Arduino.h>

#ifndef INSTANT_TIMERS_MAX
  #define INSTANT_TIMERS_MAX 16
#endif

#ifndef INSTANT_DEBUG
  #define INSTANT_DEBUG 0
#endif

class InstantTimer {
public:
    using Fn = void (*)();

    // ============================================================
    // 🎯 FLUENT API FOR SCHEDULING
    // ============================================================

    /**
     * Runs a function at regular intervals (infinite)
     * @param ms Interval in milliseconds
     * @param fn Function to call
     * @return Timer ID (-1 on error)
     */
    int every(uint32_t ms, Fn fn) {
        return schedule(ms, fn, 0, true);
    }

    /**
     * Runs a function once after a delay
     * @param ms Delay in milliseconds
     * @param fn Function to call
     * @return Timer ID (-1 on error)
     */
    int once(uint32_t ms, Fn fn) {
        return schedule(ms, fn, 1, true);
    }

    /**
     * Runs a function N times at regular intervals
     * @param ms Interval in milliseconds
     * @param fn Function to call
     * @param n Number of repetitions
     * @return Timer ID (-1 on error)
     */
    int times(uint32_t ms, Fn fn, uint16_t n) {
        return schedule(ms, fn, n, true);
    }

    // ============================================================
    // 🔧 TIMER CONTROL
    // ============================================================

    /**
     * Enables or disables a timer
     */
    bool enable(int id, bool en = true) {
        if (!valid(id) || !tasks_[id].inUse) {
#if INSTANT_DEBUG
            Serial.print(F("[Timer] ⚠️ Enable impossible: id="));
            Serial.print(id);
            Serial.println(F(" invalid"));
#endif
            return false;
        }
        tasks_[id].active = en;
#if INSTANT_DEBUG
        Serial.print(F("[Timer] "));
        Serial.print(en ? F("✅ Enabled") : F("⏸️ Disabled"));
        Serial.print(F(" timer id="));
        Serial.println(id);
#endif
        return true;
    }

    /**
     * Disables a timer (shortcut)
     */
    bool disable(int id) {
        return enable(id, false);
    }

    /**
     * Cancels and releases a timer
     */
    bool cancel(int id) {
        if (!valid(id) || !tasks_[id].inUse) {
#if INSTANT_DEBUG
            Serial.print(F("[Timer] ⚠️ Cancel impossible: id="));
            Serial.print(id);
            Serial.println(F(" invalid"));
#endif
            return false;
        }
#if INSTANT_DEBUG
        Serial.print(F("[Timer] ❌ Timer id="));
        Serial.print(id);
        Serial.println(F(" cancelled"));
#endif
        tasks_[id] = Task{};
        return true;
    }

    /**
     * Changes the interval of an existing timer
     */
    bool changeInterval(int id, uint32_t ms) {
        if (!valid(id) || !tasks_[id].inUse) {
#if INSTANT_DEBUG
            Serial.print(F("[Timer] ⚠️ ChangeInterval impossible: id="));
            Serial.print(id);
            Serial.println(F(" invalid"));
#endif
            return false;
        }
        tasks_[id].interval = ms;
#if INSTANT_DEBUG
        Serial.print(F("[Timer] ⏱️ Timer id="));
        Serial.print(id);
        Serial.print(F(" → new interval="));
        Serial.print(ms);
        Serial.println(F(" ms"));
#endif
        return true;
    }

    /**
     * Forces immediate execution of a timer
     */
    bool executeNow(int id) {
        if (!valid(id) || !tasks_[id].inUse) {
#if INSTANT_DEBUG
            Serial.print(F("[Timer] ⚠️ ExecuteNow impossible: id="));
            Serial.print(id);
            Serial.println(F(" invalid"));
#endif
            return false;
        }
        tasks_[id].nextAt = millis();
#if INSTANT_DEBUG
        Serial.print(F("[Timer] ⚡ Forced immediate execution: id="));
        Serial.println(id);
#endif
        return true;
    }

    // ============================================================
    // 🔄 MAIN LOOP
    // ============================================================

    /**
     * Runs the timers - call inside loop()
     */
    void run() {
        if (running_) {
#if INSTANT_DEBUG
            Serial.println(F("[Timer] 🔒 Reentrance blocked"));
#endif
            return; // 🛡️ Anti-reentrance
        }

        running_ = true;

        const uint32_t now = millis();
        for (int i = 0; i < INSTANT_TIMERS_MAX; i++) {
            Task& t = tasks_[i];

            if (!t.inUse || !t.active || !t.fn) continue;
            if ((int32_t)(now - t.nextAt) < 0) continue;

            // ⏰ Schedule the next execution
            if (t.interval > 0) {
                do {
                    t.nextAt += t.interval;
                } while ((int32_t)(now - t.nextAt) >= 0);
            } else {
                t.nextAt = now + 1; // 🛡️ Safety if interval==0
            }

#if INSTANT_DEBUG
            Serial.print(F("[Timer] 🔔 Executing timer id="));
            Serial.println(i);
#endif

            Fn callback = t.fn;
            callback();

            // 📉 Decrement and release if finished
            if (t.remaining > 0 && --t.remaining == 0) {
#if INSTANT_DEBUG
                Serial.print(F("[Timer] ✅ Timer id="));
                Serial.print(i);
                Serial.println(F(" finished"));
#endif
                t = Task{};
            }
        }

        running_ = false;
    }

    // ============================================================
    // 📊 STATISTICS
    // ============================================================

    /**
     * Number of active timers
     */
    int activeCount() const {
        int count = 0;
        for (int i = 0; i < INSTANT_TIMERS_MAX; i++) {
            if (tasks_[i].inUse && tasks_[i].active) count++;
        }
        return count;
    }

    /**
     * Number of used slots
     */
    int usedCount() const {
        int count = 0;
        for (int i = 0; i < INSTANT_TIMERS_MAX; i++) {
            if (tasks_[i].inUse) count++;
        }
        return count;
    }

    /**
     * Number of free slots
     */
    int freeCount() const {
        return INSTANT_TIMERS_MAX - usedCount();
    }

private:
    struct Task {
        Fn       fn        = nullptr;
        uint32_t interval  = 0;
        uint32_t nextAt    = 0;
        uint16_t remaining = 0;   // 0 = infinite
        bool     active    = false;
        bool     inUse     = false;
    };

    Task tasks_[INSTANT_TIMERS_MAX];
    bool running_ = false; // 🔒 Protection against nested loops

    int schedule(uint32_t ms, Fn fn, uint16_t repeat, bool en) {
        if (!fn) {
#if INSTANT_DEBUG
            Serial.println(F("[Timer] ❌ Schedule impossible: callback null"));
#endif
            return -1;
        }

        if (ms == 0) {
#if INSTANT_DEBUG
            Serial.println(F("[Timer] ⚠️ Schedule with ms=0, forced to 1ms"));
#endif
            ms = 1; // 🛡️ Safety
        }

        const int id = firstFree();
        if (id < 0) {
#if INSTANT_DEBUG
            Serial.println(F("[Timer] ❌ No more slots available!"));
#endif
            return -1;
        }

        tasks_[id].fn        = fn;
        tasks_[id].interval  = ms;
        tasks_[id].nextAt    = millis() + ms;
        tasks_[id].remaining = repeat;
        tasks_[id].active    = en;
        tasks_[id].inUse     = true;

#if INSTANT_DEBUG
        Serial.print(F("[Timer] ➕ Timer created: id="));
        Serial.print(id);
        Serial.print(F(" interval="));
        Serial.print(ms);
        Serial.print(F(" ms repeat="));
        Serial.println(repeat);
#endif

        return id;
    }

    int firstFree() const {
        for (int i = 0; i < INSTANT_TIMERS_MAX; i++) {
            if (!tasks_[i].inUse) return i;
        }
        return -1;
    }

    static inline bool valid(int id) {
        return id >= 0 && id < INSTANT_TIMERS_MAX;
    }
};
