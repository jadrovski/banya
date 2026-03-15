#pragma once

#include <Arduino.h>
#include "../color/RGB.h"
#include "../color/HSV.h"
#include "../color/HSL.h"
#include "../color/TemperatureColor.h"

/**
 * @brief Конфигурация RGB LED
 */
struct RGBLEDConfig {
    uint8_t pinRed;             // GPIO красный канал (по умолчанию 25)
    uint8_t pinGreen;           // GPIO зеленый канал (по умолчанию 26)
    uint8_t pinBlue;            // GPIO синий канал (по умолчанию 27)
    uint8_t channelRed;         // PWM канал красный (0-15)
    uint8_t channelGreen;       // PWM канал зеленый (0-15)
    uint8_t channelBlue;        // PWM канал синий (0-15)
    uint32_t pwmFrequency;      // Частота PWM в Гц (по умолчанию 1000)
    uint8_t pwmResolution;      // Разрешение PWM бит (по умолчанию 8)
    float gamma;                // Гамма-коррекция (по умолчанию 2.2)
    bool enableGamma;           // Включить гамма-коррекцию (по умолчанию true)

    RGBLEDConfig(
        uint8_t rPin = 25,
        uint8_t gPin = 26,
        uint8_t bPin = 27,
        uint8_t rCh = 0,
        uint8_t gCh = 1,
        uint8_t bCh = 2,
        uint32_t freq = 1000,
        uint8_t res = 8,
        float gm = 2.2f,
        bool enableGm = true
    ) : pinRed(rPin), pinGreen(gPin), pinBlue(bPin),
        channelRed(rCh), channelGreen(gCh), channelBlue(bCh),
        pwmFrequency(freq), pwmResolution(res),
        gamma(gm), enableGamma(enableGm) {}
};

/**
 * @brief Hardware Access Layer для RGB LED через ESP32 LEDC
 * 
 * Поддерживает:
 * - PWM управление яркостью каждого канала
 * - Гамма-коррекцию для линейного восприятия яркости
 * - Плавные переходы между цветами
 * - Различные цветовые модели (RGB, HSV, HSL, Temperature)
 */
class RGBLED {
protected:
    RGBLEDConfig config;
    RGB currentColor;
    float brightness;
    bool initialized;

    // Таблица гамма-коррекции (2.2 gamma)
    static const uint16_t GAMMA_TABLE[256];

public:
    /**
     * @brief Конструктор RGB LED
     * @param cfg Конфигурация
     */
    explicit RGBLED(const RGBLEDConfig& cfg = RGBLEDConfig())
        : config(cfg),
          currentColor(0, 0, 0),
          brightness(1.0f),
          initialized(false) {}

    virtual ~RGBLED() = default;

    /**
     * @brief Инициализация PWM и пинов
     * @return true если успешно
     */
    virtual bool begin() {
        // Настройка PWM каналов
        ledcSetup(config.channelRed, config.pwmFrequency, config.pwmResolution);
        ledcSetup(config.channelGreen, config.pwmFrequency, config.pwmResolution);
        ledcSetup(config.channelBlue, config.pwmFrequency, config.pwmResolution);

        // Привязка пинов
        ledcAttachPin(config.pinRed, config.channelRed);
        ledcAttachPin(config.pinGreen, config.channelGreen);
        ledcAttachPin(config.pinBlue, config.channelBlue);

        // Инициализация выключенным
        writeToPWM(0, 0, 0);
        initialized = true;
        return true;
    }

    /**
     * @brief Освобождение ресурсов
     */
    virtual void end() {
        ledcDetachPin(config.pinRed);
        ledcDetachPin(config.pinGreen);
        ledcDetachPin(config.pinBlue);
        initialized = false;
    }

    // === Установка цвета ===

    /**
     * @brief Установка цвета RGB
     * @param color Цвет RGB
     */
    virtual void setColor(const RGB& color) {
        currentColor = color;
        updateHardware();
    }

    /**
     * @brief Установка цвета HSV
     * @param color Цвет HSV
     */
    virtual void setColor(const HSV& color) {
        setColor(color.toRGB());
    }

    /**
     * @brief Установка цвета HSL
     * @param color Цвет HSL
     */
    virtual void setColor(const HSL& color) {
        setColor(color.toRGB());
    }

    /**
     * @brief Установка цвета TemperatureColor
     * @param color Температура цвета
     */
    virtual void setColor(const TemperatureColor& color) {
        setColor(color.toRGB());
    }

    /**
     * @brief Установка цвета по компонентам
     * @param r Красный (0-255)
     * @param g Зеленый (0-255)
     * @param b Синий (0-255)
     */
    virtual void setColor(uint8_t r, uint8_t g, uint8_t b) {
        setColor(RGB(r, g, b));
    }

    /**
     * @brief Установка белого цвета
     * @param brightness Яркость (0-255)
     */
    virtual void setWhite(uint8_t brightness = 255) {
        setColor(brightness, brightness, brightness);
    }

    // === Яркость ===

    /**
     * @brief Установка общей яркости
     * @param value Яркость 0.0-1.0
     */
    virtual void setBrightness(float value) {
        brightness = constrain(value, 0.0f, 1.0f);
        updateHardware();
    }

    /**
     * @brief Получить текущую яркость
     * @return Яркость 0.0-1.0
     */
    virtual float getBrightness() const { return brightness; }

    /**
     * @brief Уменьшение яркости
     * @param factor Множитель (0.0-1.0)
     */
    virtual void dim(float factor) {
        setBrightness(brightness * constrain(factor, 0.0f, 1.0f));
    }

    /**
     * @brief Увеличение яркости
     * @param factor Множитель (>1.0)
     */
    virtual void brighten(float factor) {
        setBrightness(brightness * constrain(factor, 1.0f, 10.0f));
    }

    // === Состояние ===

    /**
     * @brief Включить LED
     */
    virtual void on() {
        setBrightness(1.0f);
    }

    /**
     * @brief Выключить LED
     */
    virtual void off() {
        setBrightness(0.0f);
    }

    /**
     * @brief Проверка включен ли LED
     * @return true если яркость > 0
     */
    virtual bool isOn() const { return brightness > 0.0f; }

    /**
     * @brief Получить текущий цвет
     * @return RGB цвет
     */
    virtual RGB getCurrentColor() const { return currentColor; }

    // === Гамма-коррекция ===

    /**
     * @brief Включить/выключить гамма-коррекцию
     * @param enable true для включения
     */
    virtual void enableGamma(bool enable) {
        config.enableGamma = enable;
        updateHardware();
    }

    /**
     * @brief Установить значение гаммы
     * @param gamma Значение гаммы (по умолчанию 2.2)
     */
    virtual void setGamma(float gamma) {
        config.gamma = constrain(gamma, 0.1f, 5.0f);
        updateHardware();
    }

    /**
     * @brief Проверка статуса инициализации
     */
    bool isInitialized() const { return initialized; }

    // === Эффекты (базовые) ===

    /**
     * @brief Плавный переход к цвету
     * @param target Целевой цвет
     * @param durationMs Длительность в мс
     */
    virtual void fadeTo(const RGB& target, uint32_t durationMs) {
        RGB start = currentColor;
        unsigned long startTime = millis();

        while (millis() - startTime < durationMs) {
            float progress = (millis() - startTime) / (float)durationMs;
            setColor(start.blend(target, progress));
            delay(10);
        }

        setColor(target);
    }

    /**
     * @brief Мигание
     * @param color Цвет мигания
     * @param onTime Время включения (мс)
     * @param offTime Время выключения (мс)
     * @param repeats Количество повторений
     */
    virtual void blink(const RGB& color, uint32_t onTime, uint32_t offTime, uint8_t repeats = 1) {
        RGB original = currentColor;

        for (uint8_t i = 0; i < repeats; i++) {
            setColor(color);
            delay(onTime);
            setColor(original);
            if (i < repeats - 1) delay(offTime);
        }
    }

    /**
     * @brief Пульсация
     * @param color Цвет пульсации
     * @param periodMs Период в мс
     */
    virtual void pulse(const RGB& color, uint32_t periodMs) {
        unsigned long cycleTime = millis() % periodMs;
        float pulseValue = (sin(2 * PI * cycleTime / periodMs) + 1) / 2;

        RGB pulseColor = color.dim(pulseValue);
        setColor(pulseColor);
    }

    // === Информация ===

    /**
     * @brief Получить информацию об устройстве
     */
    virtual String getInfo() const {
        String info = "RGB LED (R:";
        info += config.pinRed;
        info += ",G:";
        info += config.pinGreen;
        info += ",B:";
        info += config.pinBlue;
        info += ") Ch:";
        info += config.channelRed;
        info += ",";
        info += config.channelGreen;
        info += ",";
        info += config.channelBlue;
        info += " Freq:";
        info += config.pwmFrequency;
        info += "Hz Res:";
        info += config.pwmResolution;
        info += "-bit Bright:";
        info += (int)(brightness * 100);
        info += "%";
        return info;
    }

    /**
     * @brief Получить конфигурацию
     */
    const RGBLEDConfig& getConfig() const { return config; }

protected:
    /**
     * @brief Запись значений в PWM
     * @param r Красный (0-255)
     * @param g Зеленый (0-255)
     * @param b Синий (0-255)
     */
    virtual void writeToPWM(uint8_t r, uint8_t g, uint8_t b) {
        if (config.enableGamma) {
            r = applyGamma(r);
            g = applyGamma(g);
            b = applyGamma(b);
        }

        // Применение яркости
        r = r * brightness;
        g = g * brightness;
        b = b * brightness;

        // Запись в hardware
        int maxDuty = (1 << config.pwmResolution) - 1;
        ledcWrite(config.channelRed, map(r, 0, 255, 0, maxDuty));
        ledcWrite(config.channelGreen, map(g, 0, 255, 0, maxDuty));
        ledcWrite(config.channelBlue, map(b, 0, 255, 0, maxDuty));
    }

    /**
     * @brief Применение гамма-коррекции
     * @param value Входное значение (0-255)
     * @return Скорректированное значение
     */
    virtual uint8_t applyGamma(uint8_t value) const {
        if (!config.enableGamma) return value;

        if (config.gamma == 2.2f) {
            return GAMMA_TABLE[value];
        } else {
            return pow(value / 255.0, 1.0 / config.gamma) * 255;
        }
    }

    /**
     * @brief Обновление hardware
     */
    virtual void updateHardware() {
        writeToPWM(currentColor.red, currentColor.green, currentColor.blue);
    }
};
