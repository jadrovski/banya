#pragma once
#include "Arduino.h"
#include "Color.h"

// Forward declarations
class RGB;
class HSL;


// HSV Color Model (Hue: 0-360, Saturation: 0-1, Value: 0-1)
class HSV : public Color {
public:
    float hue, saturation, value;

    HSV() : hue(0), saturation(0), value(0) {
    }

    HSV(
        const float hue,
        const float saturation,
        const float value
    ) : hue(hue), saturation(saturation), value(value) {
    }

    // Factory methods
    static HSV fromRGB(const RGB &rgb);

    static HSV fromHSL(const HSL &hsl);

    // Color operations
    HSV shiftHue(float degrees) const;

    HSV saturate(float factor) const;

    HSV desaturate(float factor) const;

    // Color transformations
    RGB toRGB() const override;

    HSV toHSV() const override { return *this; }

    HSL toHSL() const override;

    String toString() const override;
};
