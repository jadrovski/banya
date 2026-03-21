#include <cmath>
#include "RGB.h"
#include "HSV.h"
#include "HSL.h"
#include "TemperatureColor.h"

// RGB Implementation
RGB RGB::fromHSV(const HSV& hsv) {
    return hsv.toRGB();
}

RGB RGB::fromHSL(const HSL& hsl) {
    return hsl.toRGB();
}

RGB RGB::fromTemperature(const float kelvin) {
    return TemperatureColor(kelvin).toRGB();
}

RGB RGB::fromTemperatureCelsius(const float celsius) {
    return TemperatureColor::fromCelsius(celsius).toRGB();
}

RGB RGB::blend(const RGB& other, float ratio) const {
    ratio = constrain(ratio, 0.0f, 1.0f);
    return RGB(
        red * (1 - ratio) + other.red * ratio,
        green * (1 - ratio) + other.green * ratio,
        blue * (1 - ratio) + other.blue * ratio
    );
}

RGB RGB::blendHSV(const RGB& other, float ratio) const {
    HSV start = toHSV();
    HSV end = other.toHSV();
    return start.blend(end, ratio).toRGB();
}

RGB RGB::blendHSVShortestPath(const RGB& other, float ratio) const {
    HSV start = toHSV();
    HSV end = other.toHSV();
    
    float h1 = start.hue;
    float h2 = end.hue;
    
    float deltaH = h2 - h1;
    if (deltaH > 180) {
        h1 += 360;
    } else if (deltaH < -180) {
        h2 += 360;
    }
    
    HSV adjustedStart(h1, start.saturation, start.value);
    HSV adjustedEnd(h2, end.saturation, end.value);
    
    return adjustedStart.blend(adjustedEnd, ratio).toRGB();
}

RGB RGB::dim(float factor) const {
    factor = constrain(factor, 0.0f, 1.0f);
    return RGB(red * factor, green * factor, blue * factor);
}

RGB RGB::brighten(float factor) const {
    factor = max(1.0f, factor);
    return RGB(
        constrain(red * factor, 0.0f, 255.0f),
        constrain(green * factor, 0.0f, 255.0f),
        constrain(blue * factor, 0.0f, 255.0f)
    );
}

RGB RGB::gammaCorrect(float gamma) const {
    float invGamma = 1.0 / gamma;
    return RGB(
        pow(red / 255.0, invGamma) * 255,
        pow(green / 255.0, invGamma) * 255,
        pow(blue / 255.0, invGamma) * 255
    );
}

HSV RGB::toHSV() const {
    return HSV::fromRGB(*this);
}

HSL RGB::toHSL() const {
    return HSL::fromRGB(*this);
}

String RGB::toString() const {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "RGB(%d, %d, %d)", red, green, blue);
    return String(buffer);
}

bool RGB::operator==(const RGB& other) const {
    return red == other.red && green == other.green && blue == other.blue;
}

RGB RGB::operator+(const RGB& other) const {
    return RGB(
        constrain(red + other.red, 0, 255),
        constrain(green + other.green, 0, 255),
        constrain(blue + other.blue, 0, 255)
    );
}

RGB RGB::operator*(float factor) const {
    return RGB(
        constrain(red * factor, 0.0f, 255.0f),
        constrain(green * factor, 0.0f, 255.0f),
        constrain(blue * factor, 0.0f, 255.0f)
    );
}