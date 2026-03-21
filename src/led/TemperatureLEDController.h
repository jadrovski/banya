#pragma once
#include "Arduino.h"
#include "../color/RGB.h"
#include "../color/HSV.h"
#include "../hal/RGBLED.h"
#include <DallasTemperature.h>

/**
 * @brief Temperature range with hysteresis
 */
struct TemperatureRange {
    float minTemp;
    float maxTemp;
    uint8_t r, g, b;
};

/**
 * @brief Temperature-based LED controller with smooth HSV blending
 *
 * Features:
 * - Maps temperature ranges to colors with hysteresis
 * - Smooth HSV-based color transitions (shortest hue path)
 * - Non-blocking fade animation on Core 0
 * - Configurable fade duration
 */
class TemperatureLEDController {
public:
    static constexpr uint8_t NUM_RANGES = 7;
    static constexpr uint32_t DEFAULT_FADE_DURATION_MS = 10000;

    /**
     * @brief Temperature getter function type
     */
    typedef std::function<float()> TemperatureGetter;

    /**
     * @brief Constructor
     * @param tempGetter Function to get current temperature
     * @param ledStrip Reference to RGBLED instance
     * @param fadeDurationMs Fade animation duration in milliseconds
     */
    TemperatureLEDController(
        const TemperatureGetter &tempGetter,
        RGBLED& ledStrip,
        uint32_t fadeDurationMs = DEFAULT_FADE_DURATION_MS
    );

    /**
     * @brief Initialize the controller
     */
    void begin();

    /**
     * @brief Main loop handler (call from main loop on Core 1)
     * @note Non-blocking, checks if fade task needs to be started
     */
    void handleLoop();

    /**
     * @brief Get current temperature
     * @return Temperature in °C
     */
    float getCurrentTemperature() const { return currentTemp; }

    /**
     * @brief Get current color range index
     * @return Range index (0-6)
     */
    uint8_t getCurrentRangeIndex() const { return currentRangeIndex; }

    /**
     * @brief Get current target color
     * @return Target RGB color
     */
    RGB getTargetColor() const { return targetColor; }

    /**
     * @brief Get currently displayed color
     * @return Displayed RGB color
     */
    RGB getDisplayedColor() const { return displayedColor; }

    /**
     * @brief Check if fade animation is active
     * @return true if fading
     */
    bool isFadingActive() const { return isFadingActiveFlag; }

    /**
     * @brief Get fade duration
     * @return Duration in milliseconds
     */
    uint32_t getFadeDuration() const { return fadeDurationMs; }

    /**
     * @brief Set fade duration
     * @param ms Duration in milliseconds
     */
    void setFadeDuration(uint32_t ms) { fadeDurationMs = ms; }

    /**
     * @brief Get color for a specific temperature range
     * @param rangeIndex Range index (0-6)
     * @return RGB color
     */
    static RGB getColorForRange(uint8_t rangeIndex);

    /**
     * @brief Get status string
     * @return Status description
     */
    String getStatus() const;

private:
    TemperatureGetter getTemperature;
    RGBLED& led;
    uint32_t fadeDurationMs;

    float currentTemp;
    float lastTemp;
    bool isIncreasing;
    uint8_t currentRangeIndex;

    RGB targetColor;
    RGB displayedColor;

    // Fade animation state
    bool isFadingActiveFlag;
    unsigned long fadeStartTime;
    RGB fadeStartColor;

    // Multi-core fade task
    TaskHandle_t fadeTaskHandle;
    bool fadeTaskRunning;

    /**
     * @brief Update temperature range based on current temperature and hysteresis
     */
    void updateRange();

    /**
     * @brief Check if temperature update should trigger color change
     * @return true if color changed
     */
    bool updateTemperature();

    /**
     * @brief Start fade animation task on Core 0
     */
    void startFadeTask();

    /**
     * @brief Fade animation task function (runs on Core 0)
     * @param parameter Pointer to TemperatureLEDController instance
     */
    static void fadeTask(void* parameter);

    /**
     * @brief Cancel existing fade task if running
     */
    void cancelFadeTask();
};
