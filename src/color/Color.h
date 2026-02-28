#ifndef SAUNA_COLOR_H
#define SAUNA_COLOR_H

// Forward declarations
class RGB;
class HSV;
class HSL;
class String;

class Color {
public:
    virtual ~Color() = default;

    virtual RGB toRGB() const = 0;

    virtual HSV toHSV() const = 0;

    virtual HSL toHSL() const = 0;

    virtual String toString() const = 0;
};


#endif //SAUNA_COLOR_H