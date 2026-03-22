#pragma once

#include <Arduino.h>
#include <functional>

/**
 * @brief Abstract interface for temperature sensors
 * 
 * Provides a common interface for different temperature sensor implementations
 * (DS18B20, BME280, SerialTempSensor, etc.) allowing seamless switching between them.
 */
class ITemperatureSensor {
public:
    virtual ~ITemperatureSensor() = default;

    /**
     * @brief Initialize the sensor
     * @return true if successfully initialized
     */
    virtual bool begin() = 0;

    /**
     * @brief Get temperature from specified sensor index
     * @param index Sensor index (0-based)
     * @return Temperature in °C or DEVICE_DISCONNECTED_C if error
     */
    virtual float getTemperature(uint8_t index) = 0;

    /**
     * @brief Check if sensor at specified index is connected
     * @param index Sensor index (0-based)
     * @return true if sensor is connected and responding
     */
    virtual bool isConnected(uint8_t index) = 0;

    /**
     * @brief Get number of available sensors
     * @return Number of sensors (1 for single sensors, N for multi-sensor buses)
     */
    virtual uint8_t getSensorCount() const = 0;

    /**
     * @brief Non-blocking loop handler for async operations
     * Call regularly in main loop() for automatic updates
     */
    virtual void handleLoop() = 0;

    /**
     * @brief Get sensor information string
     * @return Human-readable info about the sensor
     */
    virtual String getInfo() const = 0;

    /**
     * @brief Get sensor status string
     * @return Current status (temperatures, connection state, etc.)
     */
    virtual String getStatus() const = 0;
};

/**
 * @brief Multi-sensor adapter that exposes individual sensors as separate instances
 * 
 * Wraps a multi-sensor implementation (like DS18B20Manager) to provide
 * single-sensor interface for each physical sensor.
 */
class SingleSensorAdapter : public ITemperatureSensor {
private:
    ITemperatureSensor* multiSensor;
    uint8_t sensorIndex;

public:
    /**
     * @brief Create adapter for single sensor from multi-sensor implementation
     * @param multi Multi-sensor implementation
     * @param index Sensor index to expose
     */
    SingleSensorAdapter(ITemperatureSensor* multi, uint8_t index)
        : multiSensor(multi), sensorIndex(index) {}

    bool begin() override {
        return multiSensor->begin();
    }

    float getTemperature(uint8_t index) override {
        if (index != 0) return DEVICE_DISCONNECTED_C;
        return multiSensor->getTemperature(sensorIndex);
    }

    bool isConnected(uint8_t index) override {
        if (index != 0) return false;
        return multiSensor->isConnected(sensorIndex);
    }

    uint8_t getSensorCount() const override {
        return 1;  // Always 1 for single sensor adapter
    }

    void handleLoop() override {
        multiSensor->handleLoop();
    }

    String getInfo() const override {
        return multiSensor->getInfo() + " [Sensor " + String(sensorIndex) + "]";
    }

    String getStatus() const override {
        return multiSensor->getStatus();
    }

    /**
     * @brief Get the actual sensor index in the multi-sensor bus
     */
    uint8_t getSensorIndex() const { return sensorIndex; }
};

/**
 * @brief Function-based temperature sensor adapter
 * 
 * Wraps a std::function<float()> to provide ITemperatureSensor interface.
 * Useful for simple temperature sources that don't need full sensor management.
 */
class FunctionTemperatureSensor : public ITemperatureSensor {
private:
    std::function<float()> tempGetter;
    std::function<bool()> connectedChecker;
    String sensorInfo;
    bool connected;

public:
    /**
     * @brief Create function-based sensor
     * @param getter Function that returns temperature
     * @param connectedFn Optional function to check if sensor is connected
     * @param info Sensor information string
     */
    FunctionTemperatureSensor(
        std::function<float()> getter,
        std::function<bool()> connectedFn = nullptr,
        const String& info = "Function Sensor"
    ) : tempGetter(getter), connectedChecker(connectedFn), sensorInfo(info), connected(true) {}

    bool begin() override {
        return true;
    }

    float getTemperature(uint8_t index) override {
        if (index != 0 || !tempGetter) return DEVICE_DISCONNECTED_C;
        return tempGetter();
    }

    bool isConnected(uint8_t index) override {
        if (index != 0) return false;
        if (connectedChecker) {
            return connectedChecker();
        }
        float temp = getTemperature(0);
        return temp != DEVICE_DISCONNECTED_C && temp > -100.0f && temp < 150.0f;
    }

    uint8_t getSensorCount() const override {
        return 1;
    }

    void handleLoop() override {
        // Nothing to do for function-based sensor
    }

    String getInfo() const override {
        return sensorInfo;
    }

    String getStatus() const override {
        float temp = DEVICE_DISCONNECTED_C;
        bool conn = false;
        if (tempGetter) {
            temp = tempGetter();
            conn = (temp != DEVICE_DISCONNECTED_C && temp > -100.0f && temp < 150.0f);
            if (connectedChecker) {
                conn = connectedChecker();
            }
        }
        return String(conn ? "Connected: " : "Disconnected: ") + String(temp, 1) + "°C";
    }

    /**
     * @brief Set connection status
     */
    void setConnected(bool conn) { connected = conn; }
};
