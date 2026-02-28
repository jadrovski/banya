#ifndef SAUNA_HAL_BME280_H
#define SAUNA_HAL_BME280_H

#include <Arduino.h>
#include <Adafruit_BME280.h>
#include "I2CDevice.h"

namespace HAL {

/**
 * @brief Конфигурация BME280
 */
struct BME280Config {
    uint8_t i2cAddress;      // I2C адрес (по умолчанию 0x76)
    uint8_t sdaPin;          // GPIO SDA (по умолчанию 21)
    uint8_t sclPin;          // GPIO SCL (по умолчанию 22)
    float seaLevelPressure;  // Давление на уровне моря в гПа (по умолчанию 1013.25)

    BME280Config(
        uint8_t addr = 0x76,
        uint8_t sda = 21,
        uint8_t scl = 22,
        float pressure = 1013.25f
    ) : i2cAddress(addr), sdaPin(sda), sclPin(scl), seaLevelPressure(pressure) {}
};

/**
 * @brief Данные с сенсора BME280
 */
struct BME280Data {
    float temperature;    // Температура в °C
    float humidity;       // Влажность в %
    float pressure;       // Давление в гПа
    float altitude;       // Высота в метрах

    BME280Data() : temperature(0), humidity(0), pressure(0), altitude(0) {}
};

/**
 * @brief Hardware Access Layer для сенсора BME280
 * 
 * Предоставляет измерение:
 * - Температуры (°C)
 * - Влажности (%)
 * - Давления (гПа, мм рт.ст.)
 * - Высоты над уровнем моря (м)
 */
class BME280Sensor : public I2CDevice {
private:
    BME280Config config;
    Adafruit_BME280* sensor;
    BME280Data lastData;
    unsigned long lastReadTime;

public:
    /**
     * @brief Конструктор BME280
     * @param cfg Конфигурация сенсора
     */
    explicit BME280Sensor(const BME280Config& cfg = BME280Config())
        : I2CDevice(cfg.i2cAddress, cfg.sdaPin, cfg.sclPin),
          config(cfg),
          sensor(nullptr),
          lastData(),
          lastReadTime(0) {}

    ~BME280Sensor() override {
        if (sensor) delete sensor;
    }

    /**
     * @brief Инициализация сенсора
     * @return true если успешно
     */
    bool begin() override {
        sensor = new Adafruit_BME280();
        
        if (!sensor->begin(config.i2cAddress, i2cPort)) {
            initialized = false;
            return false;
        }

        initialized = true;
        readData(); // Первичное чтение
        return true;
    }

    /**
     * @brief Проверка подключения сенсора
     */
    bool isConnected() override {
        if (!sensor) return false;
        return I2CDevice::isConnected();
    }

    /**
     * @brief Чтение данных с сенсора
     * @return Структура с данными
     */
    BME280Data readData() {
        if (!initialized || !sensor) {
            return lastData;
        }

        lastData.temperature = sensor->readTemperature();
        lastData.humidity = sensor->readHumidity();
        lastData.pressure = sensor->readPressure() / 100.0f; // Па -> гПа
        lastData.altitude = sensor->readAltitude(config.seaLevelPressure);
        lastReadTime = millis();

        return lastData;
    }

    /**
     * @brief Получить температуру
     * @return Температура в °C
     */
    float getTemperature() {
        if (millis() - lastReadTime > 1000) { // Авто-обновление раз в секунду
            readData();
        }
        return lastData.temperature;
    }

    /**
     * @brief Получить влажность
     * @return Влажность в %
     */
    float getHumidity() {
        if (millis() - lastReadTime > 1000) {
            readData();
        }
        return lastData.humidity;
    }

    /**
     * @brief Получить давление в гПа
     * @return Давление в гПа
     */
    float getPressure_hPa() {
        if (millis() - lastReadTime > 1000) {
            readData();
        }
        return lastData.pressure;
    }

    /**
     * @brief Получить давление в мм рт.ст.
     * @param hpaTommhg Коэффициент конвертации (по умолчанию 0.75006156)
     * @return Давление в мм рт.ст.
     */
    float getPressure_mmHg(float hpaTommhg = 0.75006156f) {
        return getPressure_hPa() * hpaTommhg;
    }

    /**
     * @brief Получить высоту над уровнем моря
     * @return Высота в метрах
     */
    float getAltitude() {
        if (millis() - lastReadTime > 1000) {
            readData();
        }
        return lastData.altitude;
    }

    /**
     * @brief Получить время последнего чтения
     */
    unsigned long getLastReadTime() const { return lastReadTime; }

    /**
     * @brief Проверка валидности данных
     * @return true если данные корректны
     */
    bool isValid() const {
        return initialized && 
               lastData.temperature > -100 && lastData.temperature < 150 &&
               lastData.humidity >= 0 && lastData.humidity <= 100;
    }

    /**
     * @brief Получить информацию о сенсоре
     */
    String getInfo() const override {
        String info = "BME280 @ 0x";
        info += String(config.i2cAddress, HEX);
        info += " (SDA:";
        info += config.sdaPin;
        info += ", SCL:";
        info += config.sclPin;
        info += ")";
        
        if (initialized) {
            info += " T:";
            info += lastData.temperature;
            info += "°C H:";
            info += lastData.humidity;
            info += "% P:";
            info += lastData.pressure;
            info += "hPa";
        }
        
        return info;
    }

    /**
     * @brief Получить конфигурацию
     */
    const BME280Config& getConfig() const { return config; }
};

} // namespace HAL

#endif // SAUNA_HAL_BME280_H
