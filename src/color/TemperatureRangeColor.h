#pragma once
#include "Arduino.h"
#include "Color.h"
#include "RGB.h"
#include "HSV.h"
#include "HSL.h"

/**
 * @brief Temperature range with hysteresis
 *
 * Defines a temperature range with:
 * - minTemp: Lower bound (for decreasing temperature)
 * - maxTemp: Upper bound (for increasing temperature)
 * - color: RGB color for this range
 */
struct TemperatureRange {
    float minTemp;      // Temperature threshold when cooling down
    float maxTemp;      // Temperature threshold when heating up
    uint8_t r, g, b;    // RGB color
};

/**
 * @brief Temperature-based LED color controller with hysteresis
 * 
 * Maps temperature ranges to specific colors with hysteresis to prevent
 * rapid color changes near threshold boundaries.
 */
class TemperatureRangeColor : public Color {
public:
    static const uint8_t NUM_RANGES = 7;

private:
    uint8_t currentRangeIndex;
    float lastTemperature;
    bool isIncreasing;

public:
    /**
     * @brief Constructor
     * @param initialTemp Initial temperature value
     */
    TemperatureRangeColor(float initialTemp = 25.0f) 
        : currentRangeIndex(0), lastTemperature(initialTemp), isIncreasing(true) {
        // Determine initial range based on temperature
        updateRange(initialTemp);
    }

    /**
     * @brief Update temperature and get corresponding color
     * @param temperature Current temperature in °C
     * @return RGB color for current temperature
     */
    RGB updateTemperature(float temperature);

    /**
     * @brief Get color for current temperature without updating
     * @return Current RGB color
     */
    RGB getCurrentColor() const;

    /**
     * @brief Get current temperature range index (0-5)
     * @return Range index
     */
    uint8_t getCurrentRangeIndex() const { return currentRangeIndex; }

    /**
     * @brief Check if temperature is increasing
     * @return true if temperature trend is increasing
     */
    bool getTrend() const { return isIncreasing; }

    /**
     * @brief Get the RGB color for a specific range
     * @param rangeIndex Range index (0-5)
     * @return RGB color
     */
    static RGB getColorForRange(uint8_t rangeIndex);

    // Color interface implementation
    RGB toRGB() const override { return getCurrentColor(); }
    HSV toHSV() const override { return toRGB().toHSV(); }
    HSL toHSL() const override { return toRGB().toHSL(); }
    String toString() const override;

private:
    /**
     * @brief Update current range based on temperature and hysteresis
     * @param temperature Current temperature
     */
    void updateRange(float temperature);
};
