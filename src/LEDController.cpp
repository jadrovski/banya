#include "LEDController.h"
#include "hal/RGBLED.h"
#include "color/HSV.h"

// LEDStripController implementation
LEDStripController::LEDStripController(HAL::RGBLED *strip)
    : strip(strip),
      lastUpdate(0),
      effectStart(0),
      currentEffect(EFFECT_NONE),
      effectRepeats(0),
      currentRepeat(0) {
}

void LEDStripController::startFade(const RGB &from, const RGB &to, uint32_t duration) {
    currentEffect = EFFECT_FADE;
    effectColor1 = from;
    effectColor2 = to;
    effectDuration = duration;
    effectStart = millis();
    strip->setColor(from);
}

void LEDStripController::startBlink(const RGB &color, uint32_t onTime, uint32_t offTime, uint8_t repeats) {
    currentEffect = EFFECT_BLINK;
    effectColor1 = color;
    effectColor2 = strip->getCurrentColor();
    effectDuration = onTime + offTime;
    effectRepeats = repeats;
    currentRepeat = 0;
    effectStart = millis();
}

void LEDStripController::startPulse(const RGB &color, uint32_t period) {
    currentEffect = EFFECT_PULSE;
    effectColor1 = color;
    effectDuration = period;
    effectStart = millis();
}

void LEDStripController::startRainbow(uint32_t cycleTime) {
    currentEffect = EFFECT_RAINBOW;
    effectDuration = cycleTime;
    effectStart = millis();
}

void LEDStripController::stopEffect() {
    currentEffect = EFFECT_NONE;
}

void LEDStripController::off() {
    strip->off();
}

void LEDStripController::handleLoop() {
    if (currentEffect == EFFECT_NONE) return;

    switch (currentEffect) {
        case EFFECT_FADE:
            updateFade();
            break;
        case EFFECT_BLINK:
            updateBlink();
            break;
        case EFFECT_PULSE:
            updatePulse();
            break;
        case EFFECT_RAINBOW:
            updateRainbow();
            break;
    }
}

void LEDStripController::updateFade() {
    unsigned long elapsed = millis() - effectStart;
    if (elapsed >= effectDuration) {
        strip->setColor(effectColor2);
        currentEffect = EFFECT_NONE;
        return;
    }

    float progress = elapsed / (float) effectDuration;
    RGB current = effectColor1.blend(effectColor2, progress);
    strip->setColor(current);
}

void LEDStripController::updateBlink() {
    unsigned long elapsed = millis() - effectStart;
    unsigned long cycleTime = elapsed % effectDuration;

    if (cycleTime < effectDuration / 2) {
        strip->setColor(effectColor1); // On
    } else {
        strip->setColor(effectColor2); // Off
    }

    // Check if repeats completed
    if (elapsed >= effectDuration * effectRepeats) {
        strip->setColor(effectColor2);
        currentEffect = EFFECT_NONE;
    }
}

void LEDStripController::updatePulse() {
    unsigned long cycleTime = (millis() - effectStart) % effectDuration;
    float pulseValue = (sin(2 * PI * cycleTime / effectDuration) + 1) / 2;

    RGB pulseColor = effectColor1.dim(pulseValue);
    strip->setColor(pulseColor);
}

void LEDStripController::updateRainbow() {
    unsigned long elapsed = millis() - effectStart;
    float hue = fmod(elapsed * 360.0 / effectDuration, 360.0);

    HSV rainbowColor(hue, 1.0, 1.0);
    strip->setColor(rainbowColor);
}
