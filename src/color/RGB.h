#pragma once
#include "Arduino.h"
#include "Color.h"

// Forward declarations
class HSV;
class HSL;
class String;

class RGB : public Color {
public:
    uint8_t red, green, blue;

    RGB() : red(0), green(0), blue(0) {
    }

    RGB(const uint8_t red, const uint8_t green, const uint8_t blue) : red(red), green(green), blue(blue) {
    }

    // Factory methods
    static RGB fromHSV(const HSV &hsv);

    static RGB fromHSL(const HSL &hsl);

    static RGB fromTemperature(float kelvin);

    static RGB fromTemperatureCelsius(float celsius);

    // Color operations
    RGB blend(const RGB &other, float ratio) const;

    RGB dim(float factor) const;

    RGB brighten(float factor) const;

    RGB gammaCorrect(float gamma = 2.2) const;

    // Color transformations
    RGB toRGB() const override { return *this; }

    HSV toHSV() const override;

    HSL toHSL() const override;

    String toString() const override;

    // Operator overloads
    bool operator==(const RGB &other) const;

    RGB operator+(const RGB &other) const;

    RGB operator*(float factor) const;
};