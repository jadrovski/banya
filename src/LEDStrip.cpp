#include "Arduino.h"
#include "LEDStrip.h"
#include "color/HSV.h"
#include "color/HSL.h"
#include "color/TemperatureColor.h"

// Gamma correction table (2.2 gamma)
const uint16_t LEDStrip::GAMMA_TABLE[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2,
    2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5,
    5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10,
    10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
    17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
    25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
    37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
    51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
    69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
    90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
    115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
    144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
    177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
    215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255
};

// LEDStrip implementation
LEDStrip::LEDStrip(const uint8_t pinR, const uint8_t pinG, const uint8_t pinB,
                   const uint8_t channelR, const uint8_t channelG, const uint8_t channelB,
                   const uint32_t freq, const uint8_t res)
    : pinR(pinR), pinG(pinG), pinB(pinB),
      channelR(channelR), channelG(channelG), channelB(channelB),
      frequency(freq), resolution(res),
      currentColor(0, 0, 0),
      brightness(1.0),
      gammaEnabled(true),
      gammaValue(2.2) {
}

void LEDStrip::begin() {
    // Setup PWM channels
    ledcSetup(channelR, frequency, resolution);
    ledcSetup(channelG, frequency, resolution);
    ledcSetup(channelB, frequency, resolution);
    
    // Attach pins
    ledcAttachPin(pinR, channelR);
    ledcAttachPin(pinG, channelG);
    ledcAttachPin(pinB, channelB);
    
    // Initialize off
    writeToPWM(0, 0, 0);
}

void LEDStrip::end() {
    ledcDetachPin(pinR);
    ledcDetachPin(pinG);
    ledcDetachPin(pinB);
}

void LEDStrip::setColor(const RGB& color) {
    currentColor = color;
    updateHardware();
}

void LEDStrip::setColor(const HSV& color) {
    setColor(color.toRGB());
}

void LEDStrip::setColor(const HSL& color) {
    setColor(color.toRGB());
}

void LEDStrip::setColor(const TemperatureColor& color) {
    setColor(color.toRGB());
}

void LEDStrip::setBrightness(float brightness) {
    this->brightness = constrain(brightness, 0.0f, 1.0f);
    updateHardware();
}

void LEDStrip::dim(const float factor) {
    brightness = constrain(brightness * factor, 0.0f, 1.0f);
    updateHardware();
}

void LEDStrip::fadeTo(const RGB& target, const uint32_t durationMs) {
    // Simple fade - for complex fades use LEDStripController
    const RGB start = currentColor;
    const unsigned long startTime = millis();
    
    while (millis() - startTime < durationMs) {
        const float progress = (millis() - startTime) / (float)durationMs;
        setColor(start.blend(target, progress));
        delay(10);
    }
    
    setColor(target);
}

void LEDStrip::blink(const RGB& color, uint32_t onTime, uint32_t offTime, uint8_t repeats) {
    RGB original = currentColor;
    
    for (uint8_t i = 0; i < repeats; i++) {
        setColor(color);
        delay(onTime);
        setColor(original);
        if (i < repeats - 1) delay(offTime);
    }
}

void LEDStrip::pulse(const RGB& color, uint32_t periodMs) {
    unsigned long cycleTime = millis() % periodMs;
    float pulseValue = (sin(2 * PI * cycleTime / periodMs) + 1) / 2;
    
    RGB pulseColor = color.dim(pulseValue);
    setColor(pulseColor);
}

String LEDStrip::getInfo() const {
    char buffer[128];
    snprintf(buffer, sizeof(buffer),
             "LEDStrip: Pins(R:%d,G:%d,B:%d) Channels(%d,%d,%d) "
             "Freq:%dHz Res:%d-bit Brightness:%.1f%%",
             pinR, pinG, pinB, channelR, channelG, channelB,
             frequency, resolution, brightness * 100);
    return String(buffer);
}

void LEDStrip::writeToPWM(uint8_t r, uint8_t g, uint8_t b) {
    if (gammaEnabled) {
        r = applyGamma(r);
        g = applyGamma(g);
        b = applyGamma(b);
    }
    
    // Apply brightness
    r = r * brightness;
    g = g * brightness;
    b = b * brightness;
    
    // Write to hardware
    int maxDuty = (1 << resolution) - 1;
    ledcWrite(channelR, map(r, 0, 255, 0, maxDuty));
    ledcWrite(channelG, map(g, 0, 255, 0, maxDuty));
    ledcWrite(channelB, map(b, 0, 255, 0, maxDuty));
}

uint8_t LEDStrip::applyGamma(uint8_t value) const {
    if (!gammaEnabled) return value;
    
    if (gammaValue == 2.2) {
        // Use lookup table for speed
        return GAMMA_TABLE[value];
    } else {
        // Calculate gamma
        return pow(value / 255.0, 1.0 / gammaValue) * 255;
    }
}

void LEDStrip::updateHardware() {
    writeToPWM(currentColor.red, currentColor.green, currentColor.blue);
}

// SaunaLEDStrip implementation
SaunaLEDStrip::SaunaLEDStrip(uint8_t pinR, uint8_t pinG, uint8_t pinB)
    : LEDStrip(pinR, pinG, pinB),
      currentTemp(25.0),
      currentHumidity(50.0) {
}

void SaunaLEDStrip::updateEnvironment(const float temperatureC, const float humidity) {
    currentTemp = temperatureC;
    currentHumidity = constrain(humidity, 0.0f, 100.0f);
}

void SaunaLEDStrip::setTemperatureMode() {
    setColor(calculateTemperatureColor());
}

void SaunaLEDStrip::setHumidityMode() {
    setColor(calculateHumidityColor());
}

void SaunaLEDStrip::setComfortMode() {
    setColor(calculateComfortColor());
}

void SaunaLEDStrip::setSafetyMode() {
    setColor(calculateSafetyColor());
}

void SaunaLEDStrip::setRelaxMode() {
    // Slow color cycle for relaxation
    float hue = fmod(millis() * 0.01, 360.0);
    HSV relaxedColor(hue, 0.7, 0.5);
    setColor(relaxedColor);
}

void SaunaLEDStrip::setAutoMode() {
    // Automatic mode selection based on conditions
    if (currentTemp > 90 || currentHumidity > 70) {
        setSafetyMode();
    } else if (currentTemp > 70) {
        setTemperatureMode();
    } else if (currentHumidity > 50) {
        setHumidityMode();
    } else {
        setComfortMode();
    }
}

RGB SaunaLEDStrip::calculateTemperatureColor() const {
    // Map temperature to color (cold blue to hot red)
    float temp = constrain(currentTemp, 50.0f, 100.0f);
    float ratio = (temp - 50) / 50.0f;
    
    if (ratio < 0.33) {
        // Blue to Cyan
        return RGB(0, 255 * ratio * 3, 255);
    } else if (ratio < 0.66) {
        // Cyan to Green
        return RGB(0, 255, 255 * (1 - (ratio - 0.33) * 3));
    } else {
        // Green to Red
        return RGB(255 * (ratio - 0.66) * 3, 255 * (1 - (ratio - 0.66) * 3), 0);
    }
}

RGB SaunaLEDStrip::calculateHumidityColor() const {
    // Humidity affects blue/white level
    float humidityRatio = currentHumidity / 100.0;
    return RGB(200 * humidityRatio, 200 * humidityRatio, 255);
}

RGB SaunaLEDStrip::calculateComfortColor() const {
    // Simple heat index
    float heatIndex = currentTemp + 0.1 * currentHumidity;
    
    if (heatIndex < 60) return SaunaColors::comfortable();
    else if (heatIndex < 70) return SaunaColors::calm();
    else if (heatIndex < 80) return SaunaColors::warm();
    else return SaunaColors::warning();
}

RGB SaunaLEDStrip::calculateSafetyColor() const {
    // Safety warning colors
    if (currentTemp > 95 || currentHumidity > 80) {
        // Critical - flashing handled by controller
        return SaunaColors::danger();
    } else if (currentTemp > 90 || currentHumidity > 70) {
        // Warning
        return SaunaColors::warning();
    } else {
        // Safe
        return SaunaColors::comfortable();
    }
}
