#pragma once
#include "Arduino.h"
#include "Color.h"
#include "RGB.h"
#include "HSV.h"
#include "HSL.h"
#include "../hal/RGBLED.h"

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
 * 
 * Features smooth color transitions between adjacent ranges by interpolating
 * colors when temperature is within a transition zone near thresholds.
 * Uses time-based fading for smooth visual transitions.
 */
class TemperatureRangeColor : public Color {
public:
    static const uint8_t NUM_RANGES = 7;
    static const uint32_t FADE_DURATION_MS = 5000;  // Fade duration in milliseconds (5 seconds)

private:
    uint8_t currentRangeIndex;
    float lastTemperature;
    bool isIncreasing;
    float transitionBlend;  // Kept for API compatibility (not used for time-based fading)
    
    // Time-based fading
    RGB targetColor;
    RGB displayedColor;
    unsigned long fadeStartTime;
    bool isFading;

    // Multi-core fade animation
    RGBLED* ledStripPtr;
    TaskHandle_t fadeTaskHandle;
    bool fadeTaskRunning;
    RGB fadeStartColor;  // Captured start color for current fade

public:
    /**
     * @brief Constructor
     * @param initialTemp Initial temperature value
     */
    TemperatureRangeColor(float initialTemp = 25.0f)
        : currentRangeIndex(0), lastTemperature(initialTemp), isIncreasing(true), transitionBlend(0.0f),
          targetColor(0, 0, 255), displayedColor(0, 0, 255), fadeStartTime(0), isFading(false),
          ledStripPtr(nullptr), fadeTaskHandle(nullptr), fadeTaskRunning(false), fadeStartColor(0, 0, 255) {
        // Determine initial range based on temperature
        updateRange(initialTemp);
        targetColor = getColorForRange(currentRangeIndex);
        displayedColor = targetColor;
    }

    /**
     * @brief Update temperature and get corresponding color
     * @param temperature Current temperature in °C
     * @return RGB color for current temperature (with smooth transition if applicable)
     */
    RGB updateTemperature(float temperature);

    /**
     * @brief Update the displayed color based on fade progress
     * @return Current displayed RGB color (after applying fade)
     */
    RGB updateDisplayedColor();

    /**
     * @brief Get color for current temperature without updating
     * @return Current target RGB color (may differ from displayed during fade)
     */
    RGB getCurrentColor() const;

    /**
     * @brief Get the currently displayed color (after fade applied)
     * @return Displayed RGB color
     */
    RGB getDisplayedColor() const { return displayedColor; }

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
     * @brief Get the transition blend factor (0.0-1.0)
     * @return 0.0 = fully in current range, 1.0 = transitioning to next range
     */
    float getTransitionBlend() const { return transitionBlend; }

    /**
     * @brief Check if currently fading
     * @return true if fade animation is in progress
     */
    bool isFadingActive() const { return isFading; }

    /**
     * @brief Run fade animation on Core 0 (non-blocking for main loop)
     * @param ledStrip Pointer to RGBLED instance to update during fade
     * @note Creates a task on Core 0 to handle fade animation
     */
    void runBlockingFade(RGBLED* ledStrip);

    /**
     * @brief Static task function for fade animation (runs on Core 0)
     * @param parameter Pointer to TemperatureRangeColor instance
     */
    static void fadeAnimationTask(void* parameter);

    /**
     * @brief Get the RGB color for a specific range
     * @param rangeIndex Range index (0-5)
     * @return RGB color
     */
    static RGB getColorForRange(uint8_t rangeIndex);

    /**
     * @brief Get smoothly interpolated color between two ranges
     * @param rangeIndex1 First range index
     * @param rangeIndex2 Second range index
     * @param blend Blend factor (0.0 = range1, 1.0 = range2)
     * @return Interpolated RGB color
     */
    static RGB getBlendedColor(uint8_t rangeIndex1, uint8_t rangeIndex2, float blend);

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
