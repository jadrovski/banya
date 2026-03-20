#include "TemperatureRangeColor.h"
#include "HSV.h"
#include "HSL.h"

// Temperature ranges with hysteresis
// Heating thresholds: when temp rises PAST this value, switch to this color
// Cooling thresholds: when temp falls BELOW this value, switch to previous color
struct TempThreshold {
    float heatTo;     // Switch to this range when heating past this
    float coolTo;     // Switch to prev range when cooling below this
    uint8_t r, g, b;  // Color
};

const TempThreshold thresholds[7] = {
    {0,  -50,  255, 0, 255},    // Range 0: Magenta
    {35,  25, 0, 0, 255},      // Range 1: Blue
    {62,  55, 0, 190, 160},    // Range 2: Light Orange/White
    {71,  68, 0, 255, 255},    // Range 3: Aqua
    {77,  74, 0, 255, 0},      // Range 4: Green
    {82,  79, 255, 190, 0},    // Range 5: Orange
    {86,  84, 255, 0, 0}       // Range 6: Red
};

RGB TemperatureRangeColor::updateTemperature(float temperature) {
    // Only update direction if change is significant (>0.1°C)
    if (abs(temperature - lastTemperature) > 0.1f) {
        isIncreasing = temperature > lastTemperature;
    }
    lastTemperature = temperature;

    updateRange(temperature);
    return getCurrentColor();
}

RGB TemperatureRangeColor::getCurrentColor() const {
    return getColorForRange(currentRangeIndex);
}

RGB TemperatureRangeColor::getColorForRange(uint8_t rangeIndex) {
    if (rangeIndex >= NUM_RANGES) {
        rangeIndex = NUM_RANGES - 1;
    }
    const TempThreshold& t = thresholds[rangeIndex];
    return RGB(t.r, t.g, t.b);
}

void TemperatureRangeColor::updateRange(float temperature) {
    // Handle negative temperatures - range 0
    if (temperature < 0) {
        currentRangeIndex = 0;
        return;
    }

    // Handle temperatures above max range (86°C+)
    if (temperature >= thresholds[NUM_RANGES - 1].heatTo) {
        currentRangeIndex = NUM_RANGES - 1;
        return;
    }

    if (isIncreasing) {
        // Heating: find range where temp >= heatTo threshold
        // Start from current range and move up
        for (uint8_t i = currentRangeIndex + 1; i < NUM_RANGES; i++) {
            if (temperature >= thresholds[i].heatTo) {
                currentRangeIndex = i;
            } else {
                break;
            }
        }
    } else {
        // Cooling: check if temp dropped below current range's coolTo threshold
        // Move down to the appropriate range
        for (int8_t i = currentRangeIndex - 1; i >= 0; i--) {
            if (temperature <= thresholds[i + 1].coolTo) {
                currentRangeIndex = i;
            } else {
                break;
            }
        }
    }
}

String TemperatureRangeColor::toString() const {
    char buffer[64];
    RGB color = getCurrentColor();
    snprintf(buffer, sizeof(buffer), "TempRange[%u]: RGB(%u,%u,%u)",
             currentRangeIndex, color.red, color.green, color.blue);
    return String(buffer);
}
