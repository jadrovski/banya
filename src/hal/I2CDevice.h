#ifndef BANYA_HAL_I2C_DEVICE_H
#define BANYA_HAL_I2C_DEVICE_H

#include <Arduino.h>
#include <Wire.h>

namespace HAL {

/**
 * @brief Базовый класс для всех I2C устройств
 * 
 * Предоставляет общую функциональность для I2C устройств:
 * - Конфигурация пинов SDA/SCL
 * - Проверка наличия устройства на шине
 * - Базовая диагностика
 */
class I2CDevice {
protected:
    uint8_t sdaPin;
    uint8_t sclPin;
    uint8_t i2cAddress;
    bool initialized;
    TwoWire* i2cPort;

public:
    /**
     * @brief Конструктор I2C устройства
     * @param address I2C адрес устройства
     * @param sda GPIO пин SDA (по умолчанию 21)
     * @param scl GPIO пин SCL (по умолчанию 22)
     * @param wire Ссылка на I2C порт (Wire или Wire1)
     */
    I2CDevice(uint8_t address, uint8_t sda = 21, uint8_t scl = 22, TwoWire* wire = &Wire)
        : sdaPin(sda), sclPin(scl), i2cAddress(address), initialized(false), i2cPort(wire) {}

    virtual ~I2CDevice() = default;

    /**
     * @brief Инициализация I2C устройства
     * @return true если успешно
     */
    virtual bool begin() {
        i2cPort->begin(sdaPin, sclPin);
        initialized = true;
        return true;
    }

    /**
     * @brief Проверка наличия устройства на шине
     * @return true если устройство отвечает
     */
    virtual bool isConnected() {
        i2cPort->beginTransmission(i2cAddress);
        return i2cPort->endTransmission() == 0;
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
     * @brief Получить информацию об устройстве
     */
    virtual String getInfo() const {
        return String("I2C Device @ 0x") + String(i2cAddress, HEX);
    }
};

} // namespace HAL

#endif // BANYA_HAL_I2C_DEVICE_H
