#pragma once

#include <Arduino.h>
#include "ITemperatureSensor.h"

/**
 * @brief Configuration for serial temperature sensor
 */
struct SerialTempSensorConfig {
    float stepSize;           // Temperature step for +/- commands (default 0.5°C)
    float minValue;           // Minimum temperature (default -10.0°C)
    float maxValue;           // Maximum temperature (default 100.0°C)
    float defaultValue;       // Default temperature (default 25.0°C)

    SerialTempSensorConfig(
        float step = 0.5f,
        float min = -10.0f,
        float max = 100.0f,
        float def = 25.0f
    ) : stepSize(step), minValue(min), maxValue(max), defaultValue(def) {}
};

/**
 * @brief Hardware Access Layer for serial-controlled mock temperature sensor
 *
 * Provides:
 * - Serial command interface for temperature control (w, s, +, -, 0-9, ?, h)
 * - Mock temperature value for testing
 * - Non-blocking command processing
 *
 * Note: Sensor switching logic (real vs mock) is handled by the application
 * via 'm' and 'r' commands processed in main.cpp.
 */
class SerialTempSensor : public ITemperatureSensor {
public:
    SerialTempSensorConfig config;
private:
    float mockTemperature;

    void printHelp() {
        Serial.println("\n=== Temperature Control ===");
        Serial.println("w/u/+ : Increase temperature (+0.5°C)");
        Serial.println("s/d/- : Decrease temperature (-0.5°C)");
        Serial.println("0-9   : Set temperature (0=0°C, 5=50°C, etc.)");
        Serial.println("?/h   : Show this help");
        Serial.println("=========================\n");
    }

    void processCommand(char cmd) {
        switch (cmd) {
            case 'w':
            case 'W':
            case '+':
            case 'u':  // up
            case 'U':
                mockTemperature = min(mockTemperature + config.stepSize, config.maxValue);
                Serial.printf("Temp: %.1f°C (+)\n", mockTemperature);
                break;

            case 's':
            case 'S':
            case '-':
            case 'd':  // down
            case 'D':
                mockTemperature = max(mockTemperature - config.stepSize, config.minValue);
                Serial.printf("Temp: %.1f°C (-)\n", mockTemperature);
                break;

            case '0':
                mockTemperature = 0.0f;
                Serial.printf("Temp: %.1f°C\n", mockTemperature);
                break;
            case '1':
                mockTemperature = 10.0f;
                Serial.printf("Temp: %.1f°C\n", mockTemperature);
                break;
            case '2':
                mockTemperature = 20.0f;
                Serial.printf("Temp: %.1f°C\n", mockTemperature);
                break;
            case '3':
                mockTemperature = 30.0f;
                Serial.printf("Temp: %.1f°C\n", mockTemperature);
                break;
            case '4':
                mockTemperature = 40.0f;
                Serial.printf("Temp: %.1f°C\n", mockTemperature);
                break;
            case '5':
                mockTemperature = 50.0f;
                Serial.printf("Temp: %.1f°C\n", mockTemperature);
                break;
            case '6':
                mockTemperature = 60.0f;
                Serial.printf("Temp: %.1f°C\n", mockTemperature);
                break;
            case '7':
                mockTemperature = 70.0f;
                Serial.printf("Temp: %.1f°C\n", mockTemperature);
                break;
            case '8':
                mockTemperature = 80.0f;
                Serial.printf("Temp: %.1f°C\n", mockTemperature);
                break;
            case '9':
                mockTemperature = 90.0f;
                Serial.printf("Temp: %.1f°C\n", mockTemperature);
                break;

            case '?':
            case 'h':
            case 'H':
                printHelp();
                break;

            default:
                break;
        }
    }

public:
    /**
     * @brief Constructor for serial temperature sensor
     * @param cfg Configuration
     */
    explicit SerialTempSensor(const SerialTempSensorConfig& cfg = SerialTempSensorConfig())
        : mockTemperature(cfg.defaultValue) {}

    /**
     * @brief Initialize the serial temperature sensor
     * @return true always (no hardware to check)
     */
    bool begin() override {
        Serial.println("Serial Temp Control: Initialized");
        return true;
    }

    /**
     * @brief Process serial commands (non-blocking)
     * Call in loop() to handle serial input
     */
    void handleLoop() override {
        if (Serial.available()) {
            char cmd = Serial.read();
            processCommand(cmd);
        }
    }

    /**
     * @brief Get current temperature value
     * @return Temperature in °C
     */
    float getTemperature(uint8_t index) override {
        return (index == 0) ? mockTemperature : DEVICE_DISCONNECTED_C;
    }

    /**
     * @brief Get current temperature or fallback value
     * @param fallback Fallback temperature
     * @return Mock temperature
     */
    float getTemperatureOr(float fallback) const {
        return mockTemperature;
    }

    /**
     * @brief Check if sensor at index is connected
     * @param index Sensor index (0-based)
     * @return true if index is 0
     */
    bool isConnected(uint8_t index) override {
        return (index == 0);
    }

    /**
     * @brief Get number of available sensors
     * @return 1 (single mock sensor)
     */
    uint8_t getSensorCount() const override {
        return 1;
    }

    /**
     * @brief Set temperature value directly
     * @param temp Temperature in °C (clamped to min/max)
     */
    void setTemperature(float temp) {
        mockTemperature = constrain(temp, config.minValue, config.maxValue);
    }

    /**
     * @brief Get current temperature value
     * @return Current mock temperature
     */
    float getMockTemperature() const { return mockTemperature; }

    /**
     * @brief Get configuration
     */
    const SerialTempSensorConfig& getConfig() const { return config; }

    /**
     * @brief Get sensor information
     * @return Info string
     */
    String getInfo() const override {
        String info = "Serial Temp Control";
        info += " (Range: ";
        info += config.minValue;
        info += "-";
        info += config.maxValue;
        info += "°C, Step: ";
        info += config.stepSize;
        info += "°C)";
        info += " Mock: ";
        info += mockTemperature;
        info += "°C";
        return info;
    }

    /**
     * @brief Get sensor status string
     * @return Status string
     */
    String getStatus() const override {
        String status = "SerialTemp: Mock ";
        status += mockTemperature;
        status += "°C";
        return status;
    }
};
