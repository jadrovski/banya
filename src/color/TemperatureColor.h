#pragma once
#include "Arduino.h"
#include "Color.h"


// Temperature-based Color (Blackbody radiation)
class TemperatureColor : public Color {
private:
    float kelvin;

public:
    TemperatureColor(const float kelvin = 6500) : kelvin(kelvin) {
    }

    // Factory from Celsius
    static TemperatureColor fromCelsius(const float celsius) {
        return TemperatureColor(273.15 + celsius);
    }

    // Get/set temperature
    float getKelvin() const { return kelvin; }
    float getCelsius() const { return kelvin - 273.15; }
    void setKelvin(float k) { kelvin = k; }
    void setCelsius(float c) { kelvin = c + 273.15; }

    // Color transformations
    RGB toRGB() const override;

    HSV toHSV() const override;

    HSL toHSL() const override;

    String toString() const override;

    // Temperature operations
    TemperatureColor warmer(float deltaK) const;

    TemperatureColor cooler(float deltaK) const;

    // Common temperature presets
    static TemperatureColor candle() { return TemperatureColor(1900); }
    static TemperatureColor tungsten() { return TemperatureColor(2700); }
    static TemperatureColor halogen() { return TemperatureColor(3200); }
    static TemperatureColor sunlight() { return TemperatureColor(5500); }
    static TemperatureColor daylight() { return TemperatureColor(6500); }
    static TemperatureColor overcast() { return TemperatureColor(7500); }
    static TemperatureColor shade() { return TemperatureColor(9000); }
};