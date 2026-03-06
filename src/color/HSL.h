#ifndef BANYA_HSL_H
#define BANYA_HSL_H

#include "Color.h"

// Forward declarations
class HSV;
class RGB;
class String;

// HSL Color Model (Hue: 0-360, Saturation: 0-1, Lightness: 0-1)
class HSL : public Color {
public:
    float hue, saturation, lightness;

    HSL() : hue(0), saturation(0), lightness(0) {
    }

    HSL(
        const float hue,
        const float saturation,
        const float lightness
    ) : hue(hue), saturation(saturation), lightness(lightness) {
    }

    // Factory methods
    static HSL fromRGB(const RGB &rgb);

    static HSL fromHSV(const HSV &hsv);

    // Color operations
    HSL lighten(float factor) const;

    HSL darken(float factor) const;

    // Color transformations
    RGB toRGB() const override;

    HSV toHSV() const override;

    HSL toHSL() const override { return *this; }

    String toString() const override;
};


#endif //BANYA_HSL_H
