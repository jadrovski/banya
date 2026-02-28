#include <Arduino.h>
#include <cmath>
#include "HSL.h"
#include "RGB.h"
#include "HSV.h"

// HSL Implementation
HSL HSL::fromRGB(const RGB& rgb) {
    float r = rgb.red / 255.0;
    float g = rgb.green / 255.0;
    float b = rgb.blue / 255.0;

    float maxVal = max(r, max(g, b));
    float minVal = min(r, min(g, b));
    float delta = maxVal - minVal;

    float h = 0;
    if (delta > 0) {
        if (maxVal == r) {
            h = 60 * fmod(((g - b) / delta), 6);
        } else if (maxVal == g) {
            h = 60 * (((b - r) / delta) + 2);
        } else {
            h = 60 * (((r - g) / delta) + 4);
        }
    }

    if (h < 0) h += 360;

    float l = (maxVal + minVal) / 2;
    float s = (delta == 0) ? 0 : delta / (1 - fabs(2 * l - 1));

    return HSL(h, s, l);
}

HSL HSL::fromHSV(const HSV& hsv) {
    return hsv.toHSL();
}

HSL HSL::lighten(float factor) const {
    float newL = constrain(lightness * factor, 0.0f, 1.0f);
    return HSL(hue, saturation, newL);
}

HSL HSL::darken(float factor) const {
    float newL = constrain(lightness / factor, 0.0f, 1.0f);
    return HSL(hue, saturation, newL);
}

RGB HSL::toRGB() const {
    float c = (1 - fabs(2 * lightness - 1)) * saturation;
    float x = c * (1 - fabs(fmod(hue / 60.0, 2) - 1));
    float m = lightness - c / 2;

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

HSV HSL::toHSV() const {
    float v = lightness + saturation * min(lightness, 1 - lightness);
    float s2 = (v == 0) ? 0 : 2 * (1 - lightness / v);
    return HSV(hue, s2, v);
}

String HSL::toString() const {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "HSL(%.1f, %.2f, %.2f)", hue, saturation, lightness);
    return String(buffer);
}