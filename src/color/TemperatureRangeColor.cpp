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
    {0,  -50,  255, 0, 255},   // Range 0: Magenta
    {35,  25, 0, 0, 255},      // Range 1: Blue
    {62,  55, 100, 200, 255},  // Range 2: Light Blue
    {71,  68, 0, 255, 255},    // Range 3: Aqua
    {77,  74, 0, 255, 0},      // Range 4: Green
    {82,  79, 255, 210, 0},    // Range 5: Orange (reduced green for clearer Red transition)
    {86,  84, 255, 0, 0}       // Range 6: Red
};

RGB TemperatureRangeColor::getBlendedColor(uint8_t rangeIndex1, uint8_t rangeIndex2, float blend) {
    blend = constrain(blend, 0.0f, 1.0f);
    RGB color1 = getColorForRange(rangeIndex1);
    RGB color2 = getColorForRange(rangeIndex2);
    return color1.blend(color2, blend);
}

RGB TemperatureRangeColor::updateTemperature(float temperature) {
    // Only update direction if change is significant (>0.1°C)
    if (abs(temperature - lastTemperature) > 0.1f) {
        isIncreasing = temperature > lastTemperature;
        lastTemperature = temperature;
        updateRange(temperature);

        // Update target color based on new temperature
        RGB newTargetColor = getCurrentColor();

        // Start fade animation only if:
        // 1. Not already fading AND color changed, OR
        // 2. Already fading but target range changed (e.g., Orange->Red vs just moving within Red range)
        if (!isFading && !displayedColor.operator==(newTargetColor)) {
            isFading = true;
            fadeStartTime = millis();
            targetColor = newTargetColor;
        } else if (isFading && !targetColor.operator==(newTargetColor)) {
            // Target range changed mid-fade, update target and continue fading
            targetColor = newTargetColor;
        }
    }

    return updateDisplayedColor();
}

RGB TemperatureRangeColor::updateDisplayedColor() {
    if (!isFading) {
        displayedColor = targetColor;
        return displayedColor;
    }

    unsigned long elapsed = millis() - fadeStartTime;
    float progress = (float)elapsed / FADE_DURATION_MS;

    if (progress >= 1.0f) {
        // Fade complete
        displayedColor = targetColor;
        isFading = false;
    } else {
        // Apply smooth easing (ease-in-out)
        float easedProgress = progress < 0.5f
            ? 2.0f * progress * progress
            : 1.0f - pow(-2.0f * progress + 2.0f, 2.0f) / 2.0f;

        displayedColor = displayedColor.blend(targetColor, easedProgress);
    }

    return displayedColor;
}

void TemperatureRangeColor::runBlockingFade(RGBLED* ledStrip) {
    // Cancel any existing fade task first
    if (fadeTaskRunning && fadeTaskHandle != nullptr) {
        vTaskDelete(fadeTaskHandle);
        fadeTaskHandle = nullptr;
        fadeTaskRunning = false;
    }

    if (!isFading) return;

    // Capture current displayed color as start point (atomic copy)
    fadeStartColor = displayedColor;

    // Store pointer to LED strip for the task
    ledStripPtr = ledStrip;
    fadeTaskRunning = true;

    // Create fade animation task on Core 0
    // Main loop runs on Core 1, so this keeps animations smooth
    xTaskCreatePinnedToCore(
        fadeAnimationTask,      // Task function
        "FadeAnimation",        // Task name
        2048,                   // Stack size (bytes)
        this,                   // Task parameter (pointer to this instance)
        2,                      // Priority (higher = more important)
        &fadeTaskHandle,        // Task handle
        0                       // Pin to Core 0
    );
}

void TemperatureRangeColor::fadeAnimationTask(void* parameter) {
    TemperatureRangeColor* self = (TemperatureRangeColor*)parameter;

    unsigned long startTime = millis();
    RGB startColor = self->fadeStartColor;  // Use captured start color

    while (true) {
        unsigned long elapsed = millis() - startTime;
        float progress = (float)elapsed / self->FADE_DURATION_MS;

        if (progress >= 1.0f) {
            self->displayedColor = self->targetColor;
            self->isFading = false;
            self->ledStripPtr->setColor(self->targetColor);
            break;
        }

        // Apply smooth easing (ease-in-out)
        float easedProgress = progress < 0.5f
            ? 2.0f * progress * progress
            : 1.0f - pow(-2.0f * progress + 2.0f, 2.0f) / 2.0f;

        self->displayedColor = startColor.blend(self->targetColor, easedProgress);
        self->ledStripPtr->setColor(self->displayedColor);

        // Small delay for smooth animation (~20ms = ~50 FPS)
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }

    self->fadeTaskRunning = false;
    self->fadeTaskHandle = nullptr;

    // Delete this task
    vTaskDelete(nullptr);
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
        transitionBlend = 0.0f;
        return;
    }

    // Handle temperatures above max range (86°C+)
    if (temperature >= thresholds[NUM_RANGES - 1].heatTo) {
        currentRangeIndex = NUM_RANGES - 1;
        transitionBlend = 1.0f;
        return;
    }

    if (isIncreasing) {
        // Heating: find range where temp >= heatTo threshold
        for (uint8_t i = currentRangeIndex + 1; i < NUM_RANGES; i++) {
            if (temperature >= thresholds[i].heatTo) {
                currentRangeIndex = i;
            } else {
                break;
            }
        }
        transitionBlend = 0.0f;  // Not used for time-based fading
    } else {
        // Cooling: find range based on coolTo thresholds
        for (int8_t i = currentRangeIndex - 1; i >= 0; i--) {
            if (temperature <= thresholds[i + 1].coolTo) {
                currentRangeIndex = i;
            } else {
                break;
            }
        }
        transitionBlend = 0.0f;  // Not used for time-based fading
    }
}

String TemperatureRangeColor::toString() const {
    char buffer[80];
    RGB color = getDisplayedColor();
    snprintf(buffer, sizeof(buffer), "TempRange[%u] Fade[%s]: RGB(%u,%u,%u)",
             currentRangeIndex, isFading ? "yes" : "no", color.red, color.green, color.blue);
    return String(buffer);
}
