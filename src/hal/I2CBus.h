#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <vector>

/**
 * @brief Конфигурация I2C шины
 */
struct I2CBusConfig {
    TwoWire* wire;        // I2C порт (Wire или Wire1)
    uint8_t sdaPin;       // GPIO SDA
    uint8_t sclPin;       // GPIO SCL
    uint32_t clockSpeed;  // Частота I2C (по умолчанию 100kHz)
    uint32_t timeoutMs;   // Таймаут операций (по умолчанию 100ms)

    I2CBusConfig(
        TwoWire* w = &Wire,
        uint8_t sda = 21,
        uint8_t scl = 22,
        uint32_t speed = 100000,
        uint32_t timeout = 100
    ) : wire(w), sdaPin(sda), sclPin(scl), clockSpeed(speed), timeoutMs(timeout) {}
};

/**
 * @brief Класс управления I2C шиной
 * 
 * Инкапсулирует инициализацию и конфигурацию I2C шины.
 * Должен быть создан и инициализирован ДО устройств на шине.
 * 
 * Пример использования:
 * @code
 * I2CBus mainBus(I2CBusConfig(&Wire, 21, 22, 100000));
 * 
 * void setup() {
 *     mainBus.begin();
 *     // Теперь можно инициализировать устройства
 *     lcd.begin();
 *     bme.begin();
 * }
 * @endcode
 */
class I2CBus {
private:
    I2CBusConfig config;
    bool initialized;

public:
    explicit I2CBus(const I2CBusConfig& cfg = I2CBusConfig())
        : config(cfg), initialized(false) {}

    /**
     * @brief Инициализация I2C шины
     */
    void begin() {
        if (initialized) {
            return;
        }
        config.wire->begin(config.sdaPin, config.sclPin);
        config.wire->setClock(config.clockSpeed);
        config.wire->setTimeOut(config.timeoutMs);
        initialized = true;
    }

    /**
     * @brief Завершение работы с I2C шиной (освобождение ресурсов)
     */
    void end() {
        if (initialized) {
            config.wire->end();
            initialized = false;
        }
    }

    /**
     * @brief Получить доступ к TwoWire порту
     */
    TwoWire* getWire() const { return config.wire; }

    /**
     * @brief Получить частоту I2C шины
     */
    uint32_t getClockSpeed() const { return config.clockSpeed; }

    /**
     * @brief Установить частоту I2C шины (динамически)
     * @param speed Частота в Гц
     */
    void setClockSpeed(const uint32_t speed) {
        config.clockSpeed = speed;
        if (initialized) {
            config.wire->setClock(speed);
        }
    }

    /**
     * @brief Проверить состояние инициализации
     */
    bool isInitialized() const { return initialized; }

    /**
     * @brief Проверка наличия устройства на шине
     * @param address I2C адрес устройства
     * @return true если устройство отвечает
     */
    bool hasDevice(const uint8_t address) const {
        if (!initialized) {
            return false;
        }
        config.wire->beginTransmission(address);
        return config.wire->endTransmission() == 0;
    }

    /**
     * @brief Сканирование шины I2C
     * @return Vector с адресами найденных устройств
     */
    std::vector<uint8_t> scanBus() const {
        std::vector<uint8_t> devices;
        if (!initialized) {
            return devices;
        }
        
        for (uint8_t addr = 0x01; addr < 0x7F; addr++) {
            if (hasDevice(addr)) {
                devices.push_back(addr);
            }
        }
        return devices;
    }

    /**
     * @brief Получить информацию о шине
     */
    String getInfo() const {
        String info = "I2C Bus (SDA:";
        info += config.sdaPin;
        info += ", SCL:";
        info += config.sclPin;
        info += ", ";
        info += config.clockSpeed / 1000;
        info += "kHz)";
        return info;
    }
};
