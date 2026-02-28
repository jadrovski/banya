#include <Arduino.h>
#include <cmath>
#include "TemperatureColor.h"
#include "RGB.h"
#include "HSV.h"
#include "HSL.h"

// TemperatureColor Implementation
RGB TemperatureColor::toRGB() const {
    float temp = kelvin / 100.0;
    float r, g, b;

    if (temp <= 66) {
        r = 255;
        g = temp;
        g = 99.4708025861 * log(g) - 161.1195681661;
        if (temp <= 19) {
            b = 0;
        } else {
            b = temp - 10;
            b = 138.5177312231 * log(b) - 305.0447927307;
        }
    } else {
        r = temp - 60;
        r = 329.698727446 * pow(r, -0.1332047592);
        g = temp - 60;
        g = 288.1221695283 * pow(g, -0.0755148492);
        b = 255;
    }

    return RGB(
        constrain(r, 0.0f, 255.0f),
        constrain(g, 0.0f, 255.0f),
        constrain(b, 0.0f, 255.0f)
    );
}

HSV TemperatureColor::toHSV() const {
    return toRGB().toHSV();
}

HSL TemperatureColor::toHSL() const {
    return toRGB().toHSL();
}

String TemperatureColor::toString() const {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Temperature(%.0fK)", kelvin);
    return String(buffer);
}

TemperatureColor TemperatureColor::warmer(float deltaK) const {
    return TemperatureColor(kelvin - deltaK);
}

TemperatureColor TemperatureColor::cooler(float deltaK) const {
    return TemperatureColor(kelvin + deltaK);
}