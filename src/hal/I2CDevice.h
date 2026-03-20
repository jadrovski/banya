#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "I2CBus.h"

/**
 * @brief Базовый класс для всех I2C устройств
 *
 * Предоставляет общую функциональность для I2C устройств:
 * - Проверка наличия устройства на шине
 * - Базовая диагностика
 * - Доступ к общей I2C шине
 *
 * @note I2C шина должна быть инициализирована ДО инициализации устройств
 */
class I2CDevice {
protected:
    I2CBus& bus;          ///< Ссылка на I2C шину (обязательна)
    uint8_t i2cAddress;   ///< I2C адрес устройства
    bool initialized;     ///< Флаг инициализации

public:
    /**
     * @brief Конструктор I2C устройства
     * @param address I2C адрес устройства
     * @param i2cBus Ссылка на объект I2C шины
     */
    I2CDevice(uint8_t address, I2CBus& i2cBus)
        : bus(i2cBus), i2cAddress(address), initialized(false) {}

    virtual ~I2CDevice() = default;

    /**
     * @brief Инициализация I2C устройства
     *
     * Проверяет что шина инициализирована
     * @return true если успешно
     */
    virtual bool begin() {
        if (!bus.isInitialized()) {
            Serial.println("ERROR: I2C bus not initialized before device!");
            return false;
        }
        initialized = true;
        return true;
    }

    /**
     * @brief Проверка наличия устройства на шине
     * @return true если устройство отвечает
     */
    virtual bool isConnected() {
        return bus.hasDevice(i2cAddress);
    }

    /**
     * @brief Получить I2C адрес устройства
     */
    uint8_t getAddress() const { return i2cAddress; }

    /**
     * @brief Получить статус инициализации
     */
    bool isInitialized() const { return initialized; }

    /**
     * @brief Получить доступ к I2C шине
     */
    I2CBus& getBus() const { return bus; }

    /**
     * @brief Получить информацию об устройстве
     */
    virtual String getInfo() const {
        String info = String("I2C Device @ 0x") + String(i2cAddress, HEX);
        info += " on " + bus.getInfo();
        return info;
    }
};
