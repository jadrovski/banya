#ifndef BANYA_HAL_DS18B20_H
#define BANYA_HAL_DS18B20_H

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "I2CDevice.h"

/**
 * @brief Конфигурация DS18B20
 */
struct DS18B20Config {
    uint8_t dataPin;                // GPIO пин данных (по умолчанию 5)
    uint8_t resolution;             // Разрешение 9-12 бит (по умолчанию 12)
    bool waitForConversion;         // Ждать конвертацию (по умолчанию false)
    unsigned long updateInterval;   // Интервал обновления температур (по умолчанию 2000мс)

    DS18B20Config(
        uint8_t pin = 5,
        uint8_t res = 12,
        bool wait = false,
        unsigned long interval = 2000
    ) : dataPin(pin), resolution(res), waitForConversion(wait), updateInterval(interval) {}
};

/**
 * @brief Данные одного сенсора DS18B20
 */
struct DS18B20SensorData {
    uint8_t index;                  // Индекс сенсора (0-based)
    DeviceAddress address;          // 64-битный адрес
    float temperature;              // Температура в °C
    bool connected;                 // Подключен ли сенсор
    uint8_t resolution;             // Текущее разрешение

    DS18B20SensorData()
        : index(0), temperature(-127), connected(false), resolution(12) {
        memset(address, 0, sizeof(DeviceAddress));
    }
};

/**
 * @brief Hardware Access Layer для сенсоров DS18B20
 *
 * Поддерживает:
 * - Неограниченное количество сенсоров на одной шине (ограничено памятью ESP32)
 * - Индивидуальное адресование по уникальному 64-битному адресу
 * - Разрешение 9-12 бит
 * - Асинхронное чтение температур
 */
class DS18B20Manager {
private:
    DS18B20Config config;
    OneWire* oneWire;
    DallasTemperature* sensors;
    DS18B20SensorData* sensorData;  // Динамический массив
    uint8_t sensorCount;
    unsigned long lastRequestTime;
    bool conversionComplete;
    unsigned long lastUpdateTime;
    bool conversionInProgress;

public:
    /**
     * @brief Конструктор DS18B20Manager
     * @param cfg Конфигурация
     */
    explicit DS18B20Manager(const DS18B20Config& cfg = DS18B20Config())
        : config(cfg),
          oneWire(nullptr),
          sensors(nullptr),
          sensorData(nullptr),
          sensorCount(0),
          lastRequestTime(0),
          conversionComplete(false),
          lastUpdateTime(0),
          conversionInProgress(false) {}

    ~DS18B20Manager() {
        if (sensorData) delete[] sensorData;
        if (sensors) delete sensors;
        if (oneWire) delete oneWire;
    }

    /**
     * @brief Инициализация менеджера сенсоров
     * @return true если успешно
     */
    bool begin() {
        oneWire = new OneWire(config.dataPin);
        sensors = new DallasTemperature(oneWire);
        sensors->begin();

        // Отключаем ожидание конвертации для асинхронной работы
        sensors->setWaitForConversion(config.waitForConversion);

        // Получение количества сенсоров и выделение памяти
        sensorCount = sensors->getDeviceCount();
        if (sensorCount > 0) {
            sensorData = new DS18B20SensorData[sensorCount];
            
            // Инициализация данных сенсоров
            for (uint8_t i = 0; i < sensorCount; i++) {
                sensors->getAddress(sensorData[i].address, i);
                sensors->setResolution(sensorData[i].address, config.resolution);
                sensorData[i].resolution = config.resolution;
                sensorData[i].connected = true;
                sensorData[i].index = i;
            }
        }

        // Первичное чтение
        requestTemperatures();

        return sensorCount > 0;
    }

    /**
     * @brief Запрос температур (неблокирующий)
     * @return Количество запрошенных сенсоров
     */
    uint8_t requestTemperatures() {
        if (!sensors) return 0;
        
        sensors->requestTemperatures();
        lastRequestTime = millis();
        conversionComplete = config.waitForConversion;
        
        return sensorCount;
    }

    /**
     * @brief Проверка готовности данных
     * @return true если конвертация завершена
     */
    bool isConversionComplete() {
        if (config.waitForConversion) {
            // В режиме ожидания просто проверяем флаг
            if (!conversionComplete) {
                conversionComplete = sensors->isConversionComplete();
            }
            return conversionComplete;
        } else {
            // В асинхронном режиме всегда проверяем датчик
            return sensors->isConversionComplete();
        }
    }

    /**
     * @brief Обновление данных температур
     * Вызывать после requestTemperatures() когда isConversionComplete() == true
     */
    void updateTemperatures() {
        if (!sensors) return;
        
        // В асинхронном режиме проверяем фактическую готовность
        if (!config.waitForConversion && !sensors->isConversionComplete()) {
            return;
        }
        
        // В режиме ожидания проверяем флаг
        if (config.waitForConversion && !conversionComplete) {
            return;
        }

        for (uint8_t i = 0; i < sensorCount; i++) {
            float temp = sensors->getTempC(sensorData[i].address);
            if (temp != DEVICE_DISCONNECTED_C) {
                sensorData[i].temperature = temp;
                sensorData[i].connected = true;
            } else {
                sensorData[i].connected = false;
            }
        }
    }

    /**
     * @brief Получить температуру по индексу
     * @param index Индекс сенсора (0-based)
     * @return Температура в °C или DEVICE_DISCONNECTED_C если ошибка
     */
    float getTemperature(uint8_t index) {
        if (index >= sensorCount) return DEVICE_DISCONNECTED_C;
        return sensorData[index].temperature;
    }

    /**
     * @brief Получить температуру по адресу
     * @param address Массив с адресом (8 байт)
     * @return Температура в °C или DEVICE_DISCONNECTED_C если не найден
     */
    float getTemperatureByAddress(const DeviceAddress address) {
        if (!sensors) return DEVICE_DISCONNECTED_C;
        return sensors->getTempC(address);
    }

    /**
     * @brief Получить адрес сенсора по индексу
     * @param index Индекс сенсора
     * @param address Выходной массив адреса (8 байт)
     * @return true если успешно
     */
    bool getAddress(uint8_t index, DeviceAddress address) {
        if (index >= sensorCount) return false;
        memcpy(address, sensorData[index].address, sizeof(DeviceAddress));
        return true;
    }

    /**
     * @brief Проверка подключения сенсора
     * @param index Индекс сенсора
     * @return true если сенсор подключен
     */
    bool isConnected(uint8_t index) {
        if (index >= sensorCount) return false;
        return sensorData[index].connected;
    }

    /**
     * @brief Получить количество найденных сенсоров
     */
    uint8_t getSensorCount() const { return sensorCount; }

    /**
     * @brief Получить данные сенсора
     * @param index Индекс сенсора
     * @return Структура с данными
     */
    const DS18B20SensorData& getSensorData(uint8_t index) {
        return sensorData[index];
    }

    /**
     * @brief Получить все данные сенсоров
     * @return Массив данных сенсоров
     */
    const DS18B20SensorData* getAllSensorData() const { return sensorData; }

    /**
     * @brief Получить разрешение сенсора
     * @param index Индекс сенсора
     * @return Разрешение в битах (9-12)
     */
    uint8_t getResolution(uint8_t index) {
        if (index >= sensorCount) return 0;
        return sensorData[index].resolution;
    }

    /**
     * @brief Установить разрешение сенсора
     * @param index Индекс сенсора
     * @param resolution Разрешение 9-12 бит
     * @return true если успешно
     */
    bool setResolution(uint8_t index, uint8_t resolution) {
        if (index >= sensorCount || resolution < 9 || resolution > 12) return false;
        
        resolution = constrain(resolution, 9, 12);
        if (sensors->setResolution(sensorData[index].address, resolution)) {
            sensorData[index].resolution = resolution;
            return true;
        }
        return false;
    }

    /**
     * @brief Получить время последнего запроса
     */
    unsigned long getLastRequestTime() const { return lastRequestTime; }

    /**
     * @brief Получить информацию о менеджере
     */
    String getInfo() const {
        String info = "DS18B20 Manager (Pin:";
        info += config.dataPin;
        info += ") Sensors: ";
        info += sensorCount;

        for (uint8_t i = 0; i < sensorCount; i++) {
            info += " [";
            info += i;
            info += ":";
            if (sensorData[i].connected) {
                info += sensorData[i].temperature;
                info += "°C";
            } else {
                info += "X";
            }
            info += "]";
        }

        return info;
    }

    /**
     * @brief Получить конфигурацию
     */
    const DS18B20Config& getConfig() const { return config; }

    /**
     * @brief Автоматическое обновление температур (вызывать в loop)
     * Менеджер сам запрашивает и обновляет температуры по таймеру
     */
    void handleLoop() {
        if (!sensors || sensorCount == 0) return;

        unsigned long now = millis();

        // Если конвертация в процессе, проверяем готовность
        if (conversionInProgress) {
            if (sensors->isConversionComplete()) {
                updateTemperatures();
                conversionInProgress = false;
                lastUpdateTime = now;
            }
            return;
        }

        // Проверка интервала обновления
        if (now - lastUpdateTime >= config.updateInterval) {
            requestTemperatures();
            conversionInProgress = true;
        }
    }
};

#endif // BANYA_HAL_DS18B20_H
