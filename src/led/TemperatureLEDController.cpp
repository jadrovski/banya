#include "TemperatureLEDController.h"
#include "../color/HSV.h"

// Temperature ranges with hysteresis
struct TempThreshold {
    float heatTo;
    float coolTo;
    uint8_t r, g, b;
};

static const TempThreshold thresholds[TemperatureLEDController::NUM_RANGES] = {
    {0,   -50, 255, 0,   255},   // Range 0: Magenta (< 0°C)
    {35,  25,  0,   0,   255},   // Range 1: Blue (0-35°C)
    {62,  55,  100, 200, 255},   // Range 2: Light Blue (35-62°C)
    {71,  68,  0,   255, 255},   // Range 3: Aqua (62-71°C)
    {77,  74,  0,   255, 0},     // Range 4: Green (71-77°C)
    {82,  79,  255, 210, 0},     // Range 5: Orange (77-82°C)
    {86,  84,  255, 0,   0}      // Range 6: Red (> 86°C)
};

TemperatureLEDController::TemperatureLEDController(
    const TemperatureGetter &tempGetter,
    RGBLED& ledStrip,
    const uint32_t fadeDurationMs
) : getTemperature(tempGetter),
    led(ledStrip),
    fadeDurationMs(fadeDurationMs),
    currentTemp(0),
    lastTemp(-999.0f),
    isIncreasing(true),
    currentRangeIndex(0),
    targetColor(0, 0, 0),
    displayedColor(0, 0, 0),
    isFadingActiveFlag(false),
    fadeStartTime(0),
    fadeStartColor(0, 0, 0),
    fadeTaskHandle(nullptr),
    fadeTaskRunning(false) {
}

void TemperatureLEDController::begin() {
    // Initialize temperature and color
    currentTemp = getTemperature();
    updateRange();
    targetColor = getColorForRange(currentRangeIndex);
    displayedColor = targetColor;
    led.setColor(displayedColor);

    Serial.printf("TemperatureLEDController: Started at %.1f°C, Range %u, Color RGB(%u,%u,%u)\n",
        currentTemp, currentRangeIndex, displayedColor.red, displayedColor.green, displayedColor.blue);
}

void TemperatureLEDController::handleLoop() {
    // Only start fade if not already fading and temperature changed
    if (!isFadingActiveFlag && updateTemperature()) {
        startFadeTask();
    }
}

bool TemperatureLEDController::updateTemperature() {
    float newTemp = getTemperature();

    // Skip invalid readings
    if (newTemp == DEVICE_DISCONNECTED_C) {
        return false;
    }

    // Check if temperature changed significantly (>0.1°C)
    if (abs(newTemp - lastTemp) <= 0.1f) {
        return false;
    }

    isIncreasing = newTemp > lastTemp;
    lastTemp = newTemp;
    currentTemp = newTemp;

    // Update range based on hysteresis
    uint8_t prevRange = currentRangeIndex;
    updateRange();

    // Check if color changed
    RGB newTargetColor = getColorForRange(currentRangeIndex);
    if (!displayedColor.operator==(newTargetColor)) {
        targetColor = newTargetColor;
        return true;
    }

    return false;
}

void TemperatureLEDController::updateRange() {
    // Handle negative temperatures
    if (currentTemp < 0) {
        currentRangeIndex = 0;
        return;
    }

    // Handle temperatures above max range
    if (currentTemp >= thresholds[NUM_RANGES - 1].heatTo) {
        currentRangeIndex = NUM_RANGES - 1;
        return;
    }

    if (isIncreasing) {
        // Heating: find highest range where temp >= heatTo
        for (uint8_t i = currentRangeIndex + 1; i < NUM_RANGES; i++) {
            if (currentTemp >= thresholds[i].heatTo) {
                currentRangeIndex = i;
            } else {
                break;
            }
        }
    } else {
        // Cooling: find lowest range based on coolTo
        for (int8_t i = currentRangeIndex - 1; i >= 0; i--) {
            if (currentTemp <= thresholds[i + 1].coolTo) {
                currentRangeIndex = i;
            } else {
                break;
            }
        }
    }
}

void TemperatureLEDController::startFadeTask() {
    // Cancel existing task if running
    cancelFadeTask();

    isFadingActiveFlag = true;
    fadeStartTime = millis();
    fadeStartColor = displayedColor;
    fadeTaskRunning = true;

    // Create fade task on Core 0
    xTaskCreatePinnedToCore(
        fadeTask,
        "TempLEDFade",
        2048,
        this,
        2,
        &fadeTaskHandle,
        0
    );

    Serial.printf("TemperatureLEDController: Fade started - Range %u, RGB(%u,%u,%u) -> RGB(%u,%u,%u)\n",
        currentRangeIndex,
        fadeStartColor.red, fadeStartColor.green, fadeStartColor.blue,
        targetColor.red, targetColor.green, targetColor.blue);
}

void TemperatureLEDController::fadeTask(void* parameter) {
    TemperatureLEDController* self = (TemperatureLEDController*)parameter;

    HSV startHSV = self->fadeStartColor.toHSV();
    HSV targetHSV = self->targetColor.toHSV();

    // Calculate shortest hue path
    float h1 = startHSV.hue;
    float h2 = targetHSV.hue;
    float deltaH = h2 - h1;
    if (deltaH > 180) {
        h1 += 360;
    } else if (deltaH < -180) {
        h2 += 360;
    }

    unsigned long startTime = millis();

    while (true) {
        unsigned long elapsed = millis() - startTime;
        float progress = (float)elapsed / self->fadeDurationMs;

        if (progress >= 1.0f || !self->isFadingActiveFlag) {
            // Fade complete or cancelled
            self->displayedColor = self->targetColor;
            self->led.setColor(self->displayedColor);
            break;
        }

        // Apply smooth easing (ease-in-out)
        float easedProgress = progress < 0.5f
            ? 2.0f * progress * progress
            : 1.0f - pow(-2.0f * progress + 2.0f, 2.0f) / 2.0f;

        // Blend in HSV space for smooth hue transitions
        HSV blendedHSV(
            h1 * (1 - easedProgress) + h2 * easedProgress,
            startHSV.saturation * (1 - easedProgress) + targetHSV.saturation * easedProgress,
            startHSV.value * (1 - easedProgress) + targetHSV.value * easedProgress
        );

        // Normalize hue
        if (blendedHSV.hue >= 360) blendedHSV.hue -= 360;
        if (blendedHSV.hue < 0) blendedHSV.hue += 360;

        self->displayedColor = blendedHSV.toRGB();
        self->led.setColor(self->displayedColor);

        // Small delay for smooth animation (~50 FPS)
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }

    self->isFadingActiveFlag = false;
    self->fadeTaskRunning = false;
    self->fadeTaskHandle = nullptr;

    Serial.printf("TemperatureLEDController: Fade complete - RGB(%u,%u,%u)\n",
        self->displayedColor.red, self->displayedColor.green, self->displayedColor.blue);

    vTaskDelete(nullptr);
}

void TemperatureLEDController::cancelFadeTask() {
    if (fadeTaskRunning && fadeTaskHandle != nullptr) {
        isFadingActiveFlag = false;
        vTaskDelete(fadeTaskHandle);
        fadeTaskHandle = nullptr;
        fadeTaskRunning = false;
    }
}

RGB TemperatureLEDController::getColorForRange(uint8_t rangeIndex) {
    if (rangeIndex >= NUM_RANGES) {
        rangeIndex = NUM_RANGES - 1;
    }
    const TempThreshold& t = thresholds[rangeIndex];
    return RGB(t.r, t.g, t.b);
}

String TemperatureLEDController::getStatus() const {
    char buffer[128];
    snprintf(buffer, sizeof(buffer),
        "TempLED: %.1f°C Range:%u RGB(%u,%u,%u) Fade:%s",
        currentTemp, currentRangeIndex,
        displayedColor.red, displayedColor.green, displayedColor.blue,
        isFadingActiveFlag ? "yes" : "no");
    return String(buffer);
}
