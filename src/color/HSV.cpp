#include <Arduino.h>
#include <cmath>
#include "HSV.h"
#include "RGB.h"
#include "HSL.h"

HSV HSV::fromRGB(const RGB& rgb) {
    const float red = rgb.red / 255.0;
    const float green = rgb.green / 255.0;
    const float blue = rgb.blue / 255.0;

    const float maxVal = max(red, max(green, blue));
    const float minVal = min(red, min(green, blue));
    const float delta = maxVal - minVal;

    float h = 0;
    if (delta > 0) {
        if (maxVal == red) {
            h = 60 * fmod(((green - blue) / delta), 6);
        } else if (maxVal == green) {
            h = 60 * (((blue - red) / delta) + 2);
        } else {
            h = 60 * (((red - green) / delta) + 4);
        }
    }

    if (h < 0) h += 360;

    float s = (maxVal > 0) ? (delta / maxVal) : 0;
    float v = maxVal;

    return HSV(h, s, v);
}

HSV HSV::fromHSL(const HSL& hsl) {
    return hsl.toHSV();
}

HSV HSV::shiftHue(float degrees) const {
    float newHue = fmod(hue + degrees, 360.0);
    if (newHue < 0) newHue += 360;
    return HSV(newHue, saturation, value);
}

HSV HSV::saturate(float factor) const {
    float newS = constrain(saturation * factor, 0.0f, 1.0f);
    return HSV(hue, newS, value);
}

HSV HSV::desaturate(float factor) const {
    float newS = constrain(saturation / factor, 0.0f, 1.0f);
    return HSV(hue, newS, value);
}

RGB HSV::toRGB() const {
    float c = value * saturation;
    float x = c * (1 - fabs(fmod(hue / 60.0, 2) - 1));
    float m = value - c;

    float r1, g1, b1;
    if (hue < 60) {
        r1 = c; g1 = x; b1 = 0;
    } else if (hue < 120) {
        r1 = x; g1 = c; b1 = 0;
    } else if (hue < 180) {
        r1 = 0; g1 = c; b1 = x;
    } else if (hue < 240) {
        r1 = 0; g1 = x; b1 = c;
    } else if (hue < 300) {
        r1 = x; g1 = 0; b1 = c;
    } else {
        r1 = c; g1 = 0; b1 = x;
    }

    return RGB(
        (r1 + m) * 255,
        (g1 + m) * 255,
        (b1 + m) * 255
    );
}

HSL HSV::toHSL() const {
    float l = value * (1 - saturation / 2);
    float s2 = (l == 0 || l == 1) ? 0 : (value - l) / min(l, 1 - l);
    return HSL(hue, s2, l);
}

String HSV::toString() const {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "HSV(%.1f, %.2f, %.2f)", hue, saturation, value);
    return String(buffer);
}