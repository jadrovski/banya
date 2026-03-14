#ifndef BANYA_HAL_TOUCH_H
#define BANYA_HAL_TOUCH_H

#include <Arduino.h>
#include <driver/touch_pad.h>
#include <functional>

/**
 * @brief Типы событий Touch сенсора
 */
enum class TouchEvent {
    NONE,           // Нет события
    TAP,            // Короткое нажатие
    LONG_PRESS,     // Длительное нажатие
    VERY_LONG_PRESS // Очень длительное нажатие
};

/**
 * @brief Конфигурация Touch сенсора
 */
struct TouchConfig {
    touch_pad_t touchPin;           // Touch пин (T0-T9)
    float thresholdPercent;         // Порог срабатывания (0.0-1.0, процент от baseline)
    uint32_t debounceMs;            // Время подавления дребезга (мс)
    uint32_t longPressMs;           // Время для long press (мс)
    uint32_t veryLongPressMs;       // Время для very-long press (мс)
    bool enableCallbacks;           // Включить callback-и

    TouchConfig(
        touch_pad_t pin = TOUCH_PAD_NUM4,   // T4 = GPIO4
        float threshPercent = 0.9f,
        uint32_t debounce = 200,
        uint32_t longPressTime = 1000,      // 1 second for long press
        uint32_t veryLongPressTime = 3000   // 3 seconds for very-long press
    ) : touchPin(pin),
        thresholdPercent(threshPercent),
        debounceMs(debounce),
        longPressMs(longPressTime),
        veryLongPressMs(veryLongPressTime),
        enableCallbacks(true) {
    }
};

/**
 * @brief Доступные Touch пины на ESP32
 *
 * T0  - GPIO4
 * T1  - GPIO0  (не рекомендуется, используется для boot)
 * T2  - GPIO2  (встроенный LED)
 * T3  - GPIO15
 * T4  - GPIO13
 * T5  - GPIO12
 * T6  - GPIO14
 * T7  - GPIO27
 * T8  - GPIO33
 * T9  - GPIO32
 */
struct TouchPins {
    static constexpr touch_pad_t T0 = TOUCH_PAD_NUM0; // GPIO4
    static constexpr touch_pad_t T1 = TOUCH_PAD_NUM1; // GPIO0
    static constexpr touch_pad_t T2 = TOUCH_PAD_NUM2; // GPIO2
    static constexpr touch_pad_t T3 = TOUCH_PAD_NUM3; // GPIO15
    static constexpr touch_pad_t T4 = TOUCH_PAD_NUM4; // GPIO13
    static constexpr touch_pad_t T5 = TOUCH_PAD_NUM5; // GPIO12
    static constexpr touch_pad_t T6 = TOUCH_PAD_NUM6; // GPIO14
    static constexpr touch_pad_t T7 = TOUCH_PAD_NUM7; // GPIO27
    static constexpr touch_pad_t T8 = TOUCH_PAD_NUM8; // GPIO33
    static constexpr touch_pad_t T9 = TOUCH_PAD_NUM9; // GPIO32
};

/**
 * @brief Callback типы для событий Touch
 */
using TouchCallback = std::function<void(TouchEvent event)>;

/**
 * @brief Hardware Access Layer для Touch сенсора ESP32
 *
 * Поддерживает:
 * - Ёмкостные touch сенсоры ESP32
 * - Настраиваемый порог срабатывания
 * - Подавление дребезга
 * - Статистика нажатий
 * - Long press и very-long press детекция
 * - Callback-и для событий
 */
class TouchSensor {
private:
    TouchConfig config;
    bool initialized;
    bool lastTouchState;
    bool lastRawState;
    unsigned long lastTouchTime;
    unsigned long lastDebounceTime;
    unsigned long touchStartTime;      // Время начала текущего нажатия
    uint32_t touchCount;
    uint32_t longPressCount;         // Счётчик long press событий
    uint32_t veryLongPressCount;     // Счётчик very-long press событий
    uint16_t baselineValue;
    uint16_t currentThreshold;       // Calculated threshold from baseline * percentage
    bool longPressTriggered;         // Флаг: long press уже сработал
    bool veryLongPressTriggered;     // Флаг: very-long press уже сработал
    TouchCallback callback;          // Callback для событий
    bool wasTouchedForTap;           // Флаг: было нажатие для TAP детекции
    unsigned long tapPressStart;     // Время начала нажатия для TAP
    bool tapProcessed;               // Флаг: TAP уже обработан

public:
    /**
     * @brief Конструктор TouchSensor
     * @param cfg Конфигурация
     */
    explicit TouchSensor(const TouchConfig &cfg = TouchConfig())
        : config(cfg),
          initialized(false),
          lastTouchState(false),
          lastRawState(false),
          lastTouchTime(0),
          lastDebounceTime(0),
          touchStartTime(0),
          touchCount(0),
          longPressCount(0),
          veryLongPressCount(0),
          baselineValue(0),
          longPressTriggered(false),
          veryLongPressTriggered(false),
          callback(nullptr),
          wasTouchedForTap(false),
          tapPressStart(0),
          tapProcessed(false) {
    }

    /**
     * @brief Инициализация touch сенсора
     * @return true если успешно
     */
    bool begin() {
        esp_err_t ret = touch_pad_init();
        if (ret != ESP_OK) {
            Serial.println("Touch: Init failed");
            return false;
        }

        // Настройка конкретного пина (временный порог, будет обновлен после калибровки)
        ret = touch_pad_config(config.touchPin, 0);
        if (ret != ESP_OK) {
            Serial.println("Touch: Config failed");
            return false;
        }

        // Задержка для стабилизации
        delay(100);

        // Измерение базового значения (несколько чтений для стабильности)
        calibrate();

        initialized = true;
        Serial.print("Touch: Initialized on pin ");
        Serial.println(getPinName());

        return true;
    }

    /**
     * @brief Калибровка базового значения
     */
    void calibrate() {
        // Несколько чтений для стабильности
        uint32_t sum = 0;
        const uint8_t samples = 10;

        for (uint8_t i = 0; i < samples; i++) {
            uint16_t value;
            touch_pad_read(config.touchPin, &value);
            sum += value;
            delay(10);
        }

        baselineValue = sum / samples;
        // Calculate threshold from percentage
        currentThreshold = (uint16_t)(baselineValue * config.thresholdPercent);

        Serial.print("Touch: Baseline = ");
        Serial.println(baselineValue);
        Serial.print("Touch: Threshold = ");
        Serial.println(currentThreshold);
        Serial.print("Touch: Threshold % = ");
        Serial.println(config.thresholdPercent, 2);

        // Apply calculated threshold to hardware
        touch_pad_set_thresh(config.touchPin, currentThreshold);
    }

    /**
     * @brief Чтение текущего значения сенсора
     * @return Значение (0-65535)
     */
    uint16_t read() const {
        uint16_t value;
        touch_pad_read(config.touchPin, &value);
        return value;
    }

    /**
     * @brief Проверка нажатия (с подавлением дребезга)
     * @return true если нажатие обнаружено
     */
    bool isTouched() {
        if (!initialized) return false;

        uint16_t value = read();

        // Touch работает инвертировано: меньше = касание
        bool touched = value < currentThreshold;

        unsigned long currentTime = millis();

        // Подавление дребезга: если "сырое" состояние изменилось, сбрасываем таймер
        if (touched != lastRawState) {
            lastDebounceTime = currentTime;
            lastRawState = touched;
        }

        // Если прошло достаточно времени после последнего изменения "сырого" состояния
        if ((currentTime - lastDebounceTime) > config.debounceMs) {
            // Обновляем стабильное состояние только если оно изменилось
            if (touched != lastTouchState) {
                lastTouchState = touched;
                if (touched) {
                    // Зафиксировано нажатие
                    touchCount++;
                    lastTouchTime = currentTime;
                    touchStartTime = currentTime;
                    longPressTriggered = false;
                    veryLongPressTriggered = false;
                    // Для TAP детекции
                    wasTouchedForTap = true;
                    tapPressStart = currentTime;
                    tapProcessed = false;
                } else {
                    // Отпускание
                }
            }
        }

        // Возвращаем debounce-ное состояние
        return lastTouchState;
    }

    /**
     * @brief Обработка событий (tap, long press, very-long press)
     * Вызывать в основном цикле loop()
     */
    void handleLoop() {
        if (!initialized || !config.enableCallbacks) return;

        // Сначала обновляем состояние тача (с учётом debounce)
        isTouched();

        bool touched = lastTouchState;
        unsigned long currentTime = millis();

        // Детекция long press и very-long press
        if (touched) {
            unsigned long pressDuration = currentTime - touchStartTime;

            // Проверка very-long press (приоритет)
            if (!veryLongPressTriggered && pressDuration >= config.veryLongPressMs) {
                veryLongPressTriggered = true;
                longPressTriggered = true;
                veryLongPressCount++;
                if (callback) {
                    callback(TouchEvent::VERY_LONG_PRESS);
                }
            }
            // Проверка long press
            else if (!longPressTriggered && pressDuration >= config.longPressMs) {
                longPressTriggered = true;
                longPressCount++;
                if (callback) {
                    callback(TouchEvent::LONG_PRESS);
                }
            }
        }
        // Детекция TAP (отпускание после короткого нажатия)
        else if (!touched && wasTouchedForTap) {
            wasTouchedForTap = false;
            unsigned long pressDuration = currentTime - tapPressStart;
            if (pressDuration < config.longPressMs && !tapProcessed) {
                tapProcessed = true;
                if (callback) {
                    callback(TouchEvent::TAP);
                }
            }
        }
    }

    /**
     * @brief Проверка нового нажатия (однократно за нажатие)
     * @return true при новом нажатии
     */
    bool isNewTouch() {
        static bool wasTouched = false;
        static unsigned long lastDebounce = 0;

        bool touched = isTouched();
        unsigned long currentTime = millis();

        if (touched && !wasTouched && (currentTime - lastDebounce) > config.debounceMs) {
            wasTouched = true;
            lastDebounce = currentTime;
            return true;
        }

        if (!touched) {
            wasTouched = false;
        }

        return false;
    }

    /**
     * @brief Проверка события TAP (короткое нажатие)
     * @return true если было короткое нажатие (отпускание после нажатия)
     */
    bool isTap() {
        static bool wasTouched = false;
        static unsigned long lastDebounce = 0;
        static unsigned long pressStart = 0;
        static bool tapProcessed = false;

        bool touched = isTouched();
        unsigned long currentTime = millis();

        if (touched && !wasTouched && (currentTime - lastDebounce) > config.debounceMs) {
            wasTouched = true;
            lastDebounce = currentTime;
            pressStart = currentTime;
            tapProcessed = false;
        }

        if (!touched && wasTouched) {
            wasTouched = false;
            // Если отпускание произошло раньше long press - это TAP
            unsigned long pressDuration = currentTime - pressStart;
            if (pressDuration < config.longPressMs && !tapProcessed) {
                tapProcessed = true;
                return true;
            }
        }

        return false;
    }

    /**
     * @brief Получить имя пина
     * @return Строка с именем (T0-T9)
     */
    String getPinName() const {
        switch (config.touchPin) {
            case TOUCH_PAD_NUM0: return "T0 (GPIO4)";
            case TOUCH_PAD_NUM1: return "T1 (GPIO0)";
            case TOUCH_PAD_NUM2: return "T2 (GPIO2)";
            case TOUCH_PAD_NUM3: return "T3 (GPIO15)";
            case TOUCH_PAD_NUM4: return "T4 (GPIO13)";
            case TOUCH_PAD_NUM5: return "T5 (GPIO12)";
            case TOUCH_PAD_NUM6: return "T6 (GPIO14)";
            case TOUCH_PAD_NUM7: return "T7 (GPIO27)";
            case TOUCH_PAD_NUM8: return "T8 (GPIO33)";
            case TOUCH_PAD_NUM9: return "T9 (GPIO32)";
            default: return "Unknown";
        }
    }

    /**
     * @brief Получить GPIO номер пина
     * @return GPIO номер
     */
    int getGPIOPin() const {
        switch (config.touchPin) {
            case TOUCH_PAD_NUM0: return 4;
            case TOUCH_PAD_NUM1: return 0;
            case TOUCH_PAD_NUM2: return 2;
            case TOUCH_PAD_NUM3: return 15;
            case TOUCH_PAD_NUM4: return 13;
            case TOUCH_PAD_NUM5: return 12;
            case TOUCH_PAD_NUM6: return 14;
            case TOUCH_PAD_NUM7: return 27;
            case TOUCH_PAD_NUM8: return 33;
            case TOUCH_PAD_NUM9: return 32;
            default: return -1;
        }
    }

    /**
     * @brief Получить количество нажатий
     */
    uint32_t getTouchCount() const { return touchCount; }

    /**
     * @brief Сбросить счётчик нажатий
     */
    void resetTouchCount() { touchCount = 0; }

    /**
     * @brief Получить время последнего нажатия
     */
    unsigned long getLastTouchTime() const { return lastTouchTime; }

    /**
     * @brief Получить базовое значение
     */
    uint16_t getBaselineValue() const { return baselineValue; }

    /**
     * @brief Установить порог срабатывания (в процентах от baseline)
     * @param thresholdPercent Порог (0.0-1.0)
     */
    void setThreshold(float thresholdPercent) {
        config.thresholdPercent = constrain(thresholdPercent, 0.0f, 1.0f);
        // Recalculate threshold based on current baseline
        currentThreshold = (uint16_t)(baselineValue * config.thresholdPercent);
        touch_pad_set_thresh(config.touchPin, currentThreshold);
    }

    /**
     * @brief Получить текущий порог в процентах
     * @return Процентное значение (0.0-1.0)
     */
    float getThresholdPercent() const { return config.thresholdPercent; }

    /**
     * @brief Получить текущий порог в абсолютном значении
     * @return Абсолютное значение порога
     */
    uint16_t getThreshold() const { return currentThreshold; }

    /**
     * @brief Проверка статуса инициализации
     */
    bool isInitialized() const { return initialized; }

    /**
     * @brief Установить callback для событий Touch
     * @param cb Callback функция
     */
    void setCallback(TouchCallback cb) {
        callback = cb;
    }

    /**
     * @brief Установить время long press
     * @param ms Время в миллисекундах
     */
    void setLongPressTime(uint32_t ms) {
        config.longPressMs = ms;
    }

    /**
     * @brief Установить время very-long press
     * @param ms Время в миллисекундах
     */
    void setVeryLongPressTime(uint32_t ms) {
        config.veryLongPressMs = ms;
    }

    /**
     * @brief Получить время long press
     * @return Время в миллисекундах
     */
    uint32_t getLongPressTime() const { return config.longPressMs; }

    /**
     * @brief Получить время very-long press
     * @return Время в миллисекундах
     */
    uint32_t getVeryLongPressTime() const { return config.veryLongPressMs; }

    /**
     * @brief Получить количество long press событий
     */
    uint32_t getLongPressCount() const { return longPressCount; }

    /**
     * @brief Получить количество very-long press событий
     */
    uint32_t getVeryLongPressCount() const { return veryLongPressCount; }

    /**
     * @brief Сбросить счётчики long press / very-long press
     */
    void resetPressCounts() {
        longPressCount = 0;
        veryLongPressCount = 0;
    }

    /**
     * @brief Получить длительность текущего нажатия
     * @return Длительность в мс, 0 если не нажато
     */
    unsigned long getCurrentPressDuration() const {
        if (!lastTouchState) return 0;
        return millis() - touchStartTime;
    }

    /**
     * @brief Проверка, был ли long press в текущем нажатии
     */
    bool wasLongPressTriggered() const { return longPressTriggered; }

    /**
     * @brief Проверка, был ли very-long press в текущем нажатии
     */
    bool wasVeryLongPressTriggered() const { return veryLongPressTriggered; }

    /**
     * @brief Получить информацию о сенсоре
     */
    String getInfo() const {
        uint16_t currentValue = read();
        String info = "Touch: ";
        info += getPinName();
        info += " | Value: ";
        info += currentValue;
        info += " (base:";
        info += baselineValue;
        info += ") | Thresh: ";
        info += currentThreshold;
        info += " (";
        info += (config.thresholdPercent * 100.0f);
        info += "%) | Touches: ";
        info += touchCount;
        info += " | LP: ";
        info += longPressCount;
        info += " | VLP: ";
        info += veryLongPressCount;

        if (lastTouchState) {
            info += " [TOUCHED]";
            info += " (";
            info += getCurrentPressDuration();
            info += "ms)";
        }

        return info;
    }

    /**
     * @brief Получить конфигурацию
     */
    const TouchConfig &getConfig() const { return config; }
};

#endif // BANYA_HAL_TOUCH_H
