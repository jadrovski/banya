#ifndef SAUNA_HAL_TOUCH_H
#define SAUNA_HAL_TOUCH_H

#include <Arduino.h>
#include <driver/touch_pad.h>

namespace HAL {
    /**
     * @brief Конфигурация Touch сенсора
     */
    struct TouchConfig {
        touch_pad_t touchPin; // Touch пин (T0-T9)
        uint16_t threshold; // Порог срабатывания
        uint32_t debounceMs; // Время подавления дребезга (мс)

        TouchConfig(
            touch_pad_t pin = TOUCH_PAD_NUM4, // T4 = GPIO4
            uint16_t thresh = 50,
            uint32_t debounce = 200
        ) : touchPin(pin), threshold(thresh), debounceMs(debounce) {
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
    namespace TouchPins {
        constexpr touch_pad_t T0 = TOUCH_PAD_NUM0; // GPIO4
        constexpr touch_pad_t T1 = TOUCH_PAD_NUM1; // GPIO0
        constexpr touch_pad_t T2 = TOUCH_PAD_NUM2; // GPIO2
        constexpr touch_pad_t T3 = TOUCH_PAD_NUM3; // GPIO15
        constexpr touch_pad_t T4 = TOUCH_PAD_NUM4; // GPIO13
        constexpr touch_pad_t T5 = TOUCH_PAD_NUM5; // GPIO12
        constexpr touch_pad_t T6 = TOUCH_PAD_NUM6; // GPIO14
        constexpr touch_pad_t T7 = TOUCH_PAD_NUM7; // GPIO27
        constexpr touch_pad_t T8 = TOUCH_PAD_NUM8; // GPIO33
        constexpr touch_pad_t T9 = TOUCH_PAD_NUM9; // GPIO32
    }

    /**
     * @brief Hardware Access Layer для Touch сенсора ESP32
     *
     * Поддерживает:
     * - Ёмкостные touch сенсоры ESP32
     * - Настраиваемый порог срабатывания
     * - Подавление дребезга
     * - Статистика нажатий
     */
    class TouchSensor {
    private:
        TouchConfig config;
        bool initialized;
        bool lastTouchState;
        unsigned long lastTouchTime;
        unsigned long lastDebounceTime;
        uint32_t touchCount;
        uint16_t baselineValue;

    public:
        /**
         * @brief Конструктор TouchSensor
         * @param cfg Конфигурация
         */
        explicit TouchSensor(const TouchConfig &cfg = TouchConfig())
            : config(cfg),
              initialized(false),
              lastTouchState(false),
              lastTouchTime(0),
              lastDebounceTime(0),
              touchCount(0),
              baselineValue(0) {
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

            // Настройка конкретного пина
            ret = touch_pad_config(config.touchPin, config.threshold);
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
            Serial.print("Touch: Baseline = ");
            Serial.println(baselineValue);
            Serial.print("Touch: Current value = ");
            uint16_t current;
            touch_pad_read(config.touchPin, &current);
            Serial.println(current);
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
            bool touched = value < config.threshold;

            unsigned long currentTime = millis();

            // Подавление дребезга: если состояние изменилось, сбрасываем таймер
            if (touched != lastTouchState) {
                lastDebounceTime = currentTime;
            }

            // Если прошло достаточно времени после последнего изменения
            if ((currentTime - lastDebounceTime) > config.debounceMs) {
                // Обновляем стабильное состояние только если оно изменилось
                if (touched != lastTouchState) {
                    lastTouchState = touched;
                    if (touched) {
                        // Зафиксировано нажатие
                        touchCount++;
                        lastTouchTime = currentTime;
                    }
                }
            }

            // Возвращаем текущее "сырое" состояние с учётом порога
            // Debounce применяется только для счётчика нажатий
            return touched;
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
         * @brief Установить порог срабатывания
         * @param threshold Порог (0-65535)
         */
        void setThreshold(uint16_t threshold) {
            config.threshold = threshold;
            touch_pad_set_thresh(config.touchPin, threshold);
        }

        /**
         * @brief Получить текущий порог
         */
        uint16_t getThreshold() const { return config.threshold; }

        /**
         * @brief Проверка статуса инициализации
         */
        bool isInitialized() const { return initialized; }

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
            info += config.threshold;
            info += " | Touches: ";
            info += touchCount;

            if (lastTouchState) {
                info += " [TOUCHED]";
            }

            return info;
        }

        /**
         * @brief Получить конфигурацию
         */
        const TouchConfig &getConfig() const { return config; }
    };
} // namespace HAL

#endif // SAUNA_HAL_TOUCH_H
