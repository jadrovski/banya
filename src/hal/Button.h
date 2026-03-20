#pragma once

#include <Arduino.h>
#include <functional>

/**
 * @brief Button event types
 */
enum class ButtonEvent {
    NONE,           // No event
    PRESSED,        // Button pressed (immediate)
    RELEASED,       // Button released (immediate)
    TAP,            // Short press and release
    LONG_PRESS,     // Long press held
    VERY_LONG_PRESS // Very long press held
};

/**
 * @brief Button configuration
 */
struct ButtonConfig {
    int8_t buttonPin;             // GPIO pin number
    bool activeLow;               // true: button connects to GND, false: connects to VCC
    uint32_t debounceMs;          // Debounce time (ms)
    uint32_t longPressMs;         // Time for long press (ms)
    uint32_t veryLongPressMs;     // Time for very-long press (ms)
    bool enablePullup;            // Enable internal pull-up resistor
    bool enableCallbacks;         // Enable callbacks

    ButtonConfig(
        int8_t pin = 15,          // GPIO15
        bool activeLowMode = true,
        uint32_t debounce = 50,
        uint32_t longPressTime = 1000,      // 1 second for long press
        uint32_t veryLongPressTime = 3000,  // 3 seconds for very-long press
        bool enablePull = true
    ) : buttonPin(pin),
        activeLow(activeLowMode),
        debounceMs(debounce),
        longPressMs(longPressTime),
        veryLongPressMs(veryLongPressTime),
        enablePullup(enablePull),
        enableCallbacks(true) {
    }
};

/**
 * @brief Callback type for button events
 */
using ButtonCallback = std::function<void(ButtonEvent event)>;

/**
 * @brief Hardware Access Layer for Button input on ESP32
 *
 * Features:
 * - Configurable GPIO pin
 * - Active-low or active-high configuration
 * - Configurable debounce
 * - Press/release event detection
 * - Tap detection (short press and release)
 * - Long press and very-long press detection
 * - Callback support for events
 * - Press duration tracking
 */
class Button {
private:
    ButtonConfig config;
    bool initialized;
    bool lastButtonState;         // Stable debounced state
    bool lastRawState;            // Raw state before debounce
    unsigned long lastButtonTime; // Time of last state change
    unsigned long lastDebounceTime; // Time of last raw state change
    unsigned long pressStartTime; // Time when press started
    uint32_t pressCount;          // Total press count
    uint32_t releaseCount;        // Total release count
    uint32_t tapCount;            // Tap event count
    uint32_t longPressCount;      // Long press event count
    uint32_t veryLongPressCount;  // Very-long press event count
    bool longPressTriggered;      // Flag: long press already triggered
    bool veryLongPressTriggered;  // Flag: very-long press already triggered
    ButtonCallback callback;      // Callback for events
    bool wasPressedForTap;        // Flag: was pressed for TAP detection
    unsigned long tapPressStart;  // Time of press for TAP
    bool tapProcessed;            // Flag: TAP already processed

public:
    /**
     * @brief Button constructor
     * @param cfg Configuration
     */
    explicit Button(const ButtonConfig &cfg = ButtonConfig())
        : config(cfg),
          initialized(false),
          lastButtonState(!config.activeLow),  // Rest state
          lastRawState(!config.activeLow),
          lastButtonTime(0),
          lastDebounceTime(0),
          pressStartTime(0),
          pressCount(0),
          releaseCount(0),
          tapCount(0),
          longPressCount(0),
          veryLongPressCount(0),
          longPressTriggered(false),
          veryLongPressTriggered(false),
          callback(nullptr),
          wasPressedForTap(false),
          tapPressStart(0),
          tapProcessed(false) {
    }

    /**
     * @brief Initialize button pin
     * @return true if successful
     */
    bool begin() {
        if (config.buttonPin < 0) {
            Serial.println("Button: Invalid pin");
            return false;
        }

        // Configure pin mode
        if (config.enablePullup && config.activeLow) {
            pinMode(config.buttonPin, INPUT_PULLUP);
        } else if (config.enablePullup && !config.activeLow) {
            pinMode(config.buttonPin, INPUT_PULLDOWN);
        } else {
            pinMode(config.buttonPin, INPUT);
        }

        // Read initial state
        lastRawState = readRaw();
        lastButtonState = lastRawState;

        initialized = true;
        Serial.print("Button: Initialized on GPIO");
        Serial.println(config.buttonPin);
        Serial.print("Button: Active ");
        Serial.println(config.activeLow ? "LOW" : "HIGH");

        return true;
    }

    /**
     * @brief Read raw button state (no debounce)
     * @return true if button is pressed (active state)
     */
    bool readRaw() const {
        bool pinState = digitalRead(config.buttonPin);
        return config.activeLow ? !pinState : pinState;
    }

    /**
     * @brief Read button state with debounce
     * @return true if button is pressed (active state)
     */
    bool isPressed() {
        if (!initialized) return false;

        bool pressed = readRaw();
        unsigned long currentTime = millis();

        // Debounce: if raw state changed, reset debounce timer
        if (pressed != lastRawState) {
            lastDebounceTime = currentTime;
            lastRawState = pressed;
        }

        // If enough time has passed since last raw state change
        if ((currentTime - lastDebounceTime) > config.debounceMs) {
            // Update stable state only if it changed
            if (pressed != lastButtonState) {
                lastButtonState = pressed;
                lastButtonTime = currentTime;

                if (pressed) {
                    // Button pressed
                    pressCount++;
                    pressStartTime = currentTime;
                    longPressTriggered = false;
                    veryLongPressTriggered = false;
                    // For TAP detection
                    wasPressedForTap = true;
                    tapPressStart = currentTime;
                    tapProcessed = false;
                } else {
                    // Button released
                    releaseCount++;
                }
            }
        }

        // Return debounced state
        return lastButtonState;
    }

    /**
     * @brief Process button events (press, release, tap, long press, very-long press)
     * Call in main loop()
     */
    void handleLoop() {
        if (!initialized || !config.enableCallbacks) return;

        // First update button state (with debounce)
        isPressed();

        bool pressed = lastButtonState;
        unsigned long currentTime = millis();

        // Detect long press and very-long press
        if (pressed) {
            unsigned long pressDuration = currentTime - pressStartTime;

            // Check very-long press (priority)
            if (!veryLongPressTriggered && pressDuration >= config.veryLongPressMs) {
                veryLongPressTriggered = true;
                longPressTriggered = true;
                veryLongPressCount++;
                if (callback) {
                    callback(ButtonEvent::VERY_LONG_PRESS);
                }
            }
            // Check long press
            else if (!longPressTriggered && pressDuration >= config.longPressMs) {
                longPressTriggered = true;
                longPressCount++;
                if (callback) {
                    callback(ButtonEvent::LONG_PRESS);
                }
            }
        }
        // Detect PRESSED event (immediate)
        else if (!pressed && wasPressedForTap) {
            // Button was released - check for TAP
            wasPressedForTap = false;
            unsigned long pressDuration = currentTime - tapPressStart;

            // If released before long press and not yet processed - it's a TAP
            if (pressDuration < config.longPressMs && !tapProcessed) {
                tapProcessed = true;
                tapCount++;
                if (callback) {
                    callback(ButtonEvent::TAP);
                }
            }
        }

        // Detect PRESSED event (when button just pressed)
        static bool wasPressed = false;
        if (pressed && !wasPressed) {
            if (callback) {
                callback(ButtonEvent::PRESSED);
            }
        }
        // Detect RELEASED event (when button just released)
        else if (!pressed && wasPressed) {
            if (callback) {
                callback(ButtonEvent::RELEASED);
            }
        }
        wasPressed = pressed;
    }

    /**
     * @brief Check for new press event (once per press)
     * @return true on new press
     */
    bool isNewPress() {
        static bool wasPressed = false;
        static unsigned long lastDebounce = 0;

        bool pressed = isPressed();
        unsigned long currentTime = millis();

        if (pressed && !wasPressed && (currentTime - lastDebounce) > config.debounceMs) {
            wasPressed = true;
            lastDebounce = currentTime;
            return true;
        }

        if (!pressed) {
            wasPressed = false;
        }

        return false;
    }

    /**
     * @brief Check for TAP event (short press and release)
     * @return true if tap was detected
     */
    bool isTap() {
        static bool wasPressed = false;
        static unsigned long lastDebounce = 0;
        static unsigned long pressStart = 0;
        static bool tapProcessed = false;

        bool pressed = isPressed();
        unsigned long currentTime = millis();

        if (pressed && !wasPressed && (currentTime - lastDebounce) > config.debounceMs) {
            wasPressed = true;
            lastDebounce = currentTime;
            pressStart = currentTime;
            tapProcessed = false;
        }

        if (!pressed && wasPressed) {
            wasPressed = false;
            // If release happened before long press - it's a TAP
            unsigned long pressDuration = currentTime - pressStart;
            if (pressDuration < config.longPressMs && !tapProcessed) {
                tapProcessed = true;
                return true;
            }
        }

        return false;
    }

    /**
     * @brief Get pin number
     * @return GPIO pin number
     */
    int getPin() const {
        return config.buttonPin;
    }

    /**
     * @brief Get pin description
     * @return String with pin info
     */
    String getPinName() const {
        return "GPIO" + String(config.buttonPin);
    }

    /**
     * @brief Get press count
     */
    uint32_t getPressCount() const { return pressCount; }

    /**
     * @brief Get release count
     */
    uint32_t getReleaseCount() const { return releaseCount; }

    /**
     * @brief Get tap count
     */
    uint32_t getTapCount() const { return tapCount; }

    /**
     * @brief Reset press counter
     */
    void resetPressCount() { pressCount = 0; }

    /**
     * @brief Reset release counter
     */
    void resetReleaseCount() { releaseCount = 0; }

    /**
     * @brief Reset tap counter
     */
    void resetTapCount() { tapCount = 0; }

    /**
     * @brief Get time of last state change
     */
    unsigned long getLastButtonTime() const { return lastButtonTime; }

    /**
     * @brief Check initialization status
     */
    bool isInitialized() const { return initialized; }

    /**
     * @brief Set callback for button events
     * @param cb Callback function
     */
    void setCallback(ButtonCallback cb) {
        callback = cb;
    }

    /**
     * @brief Set long press time
     * @param ms Time in milliseconds
     */
    void setLongPressTime(uint32_t ms) {
        config.longPressMs = ms;
    }

    /**
     * @brief Set very-long press time
     * @param ms Time in milliseconds
     */
    void setVeryLongPressTime(uint32_t ms) {
        config.veryLongPressMs = ms;
    }

    /**
     * @brief Get long press time
     * @return Time in milliseconds
     */
    uint32_t getLongPressTime() const { return config.longPressMs; }

    /**
     * @brief Get very-long press time
     * @return Time in milliseconds
     */
    uint32_t getVeryLongPressTime() const { return config.veryLongPressMs; }

    /**
     * @brief Get long press event count
     */
    uint32_t getLongPressCount() const { return longPressCount; }

    /**
     * @brief Get very-long press event count
     */
    uint32_t getVeryLongPressCount() const { return veryLongPressCount; }

    /**
     * @brief Reset long press / very-long press counters
     */
    void resetPressCounts() {
        longPressCount = 0;
        veryLongPressCount = 0;
    }

    /**
     * @brief Get current press duration
     * @return Duration in ms, 0 if not pressed
     */
    unsigned long getCurrentPressDuration() const {
        if (!lastButtonState) return 0;
        return millis() - pressStartTime;
    }

    /**
     * @brief Check if long press was triggered in current press
     */
    bool wasLongPressTriggered() const { return longPressTriggered; }

    /**
     * @brief Check if very-long press was triggered in current press
     */
    bool wasVeryLongPressTriggered() const { return veryLongPressTriggered; }

    /**
     * @brief Get button information
     */
    String getInfo() const {
        bool rawState = readRaw();
        String info = "Button: ";
        info += getPinName();
        info += " | Raw: ";
        info += rawState ? "1" : "0";
        info += " | Pressed: ";
        info += lastButtonState ? "1" : "0";
        info += " | Presses: ";
        info += pressCount;
        info += " | Releases: ";
        info += releaseCount;
        info += " | Taps: ";
        info += tapCount;
        info += " | LP: ";
        info += longPressCount;
        info += " | VLP: ";
        info += veryLongPressCount;

        if (lastButtonState) {
            info += " [PRESSED]";
            info += " (";
            info += getCurrentPressDuration();
            info += "ms)";
        }

        return info;
    }

    /**
     * @brief Get configuration
     */
    const ButtonConfig &getConfig() const { return config; }

    /**
     * @brief Check if button is currently released (not pressed)
     */
    bool isReleased() const { return !lastButtonState; }
};
