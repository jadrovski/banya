#ifndef BANYA_LEDSTRIP_H
#define BANYA_LEDSTRIP_H

#include <Arduino.h>
#include "color/BanyaColors.h"

class TemperatureColor;

class LEDStrip {
protected:
    uint8_t pinR, pinG, pinB;
    uint8_t channelR, channelG, channelB;
    uint32_t frequency;
    uint8_t resolution;
    RGB currentColor;
    float brightness;  // 0.0 to 1.0
    bool gammaEnabled;
    float gammaValue;

    // Gamma correction table (8-bit)
    static const uint16_t GAMMA_TABLE[256];

public:
    // Constructor
    LEDStrip(uint8_t pinR, uint8_t pinG, uint8_t pinB,
             uint8_t channelR = 0, uint8_t channelG = 1, uint8_t channelB = 2,
             uint32_t freq = 1000, uint8_t res = 8);

    virtual ~LEDStrip() = default;

    // Initialization
    virtual void begin();
    virtual void end();

    // Color setting with different color models
    virtual void setColor(const RGB& color);
    virtual void setColor(const HSV& color);
    virtual void setColor(const HSL& color);
    virtual void setColor(const TemperatureColor& color);

    // Brightness control
    virtual void setBrightness(float brightness);  // 0.0 to 1.0
    virtual float getBrightness() const { return brightness; }
    virtual void dim(float factor);  // Multiply brightness

    // Effects
    virtual void fadeTo(const RGB& target, uint32_t durationMs);
    virtual void blink(const RGB& color, uint32_t onTime, uint32_t offTime, uint8_t repeats = 1);
    virtual void pulse(const RGB& color, uint32_t periodMs);

    // Gamma correction
    virtual void enableGamma(bool enable) { gammaEnabled = enable; }
    virtual void setGamma(float gamma) { gammaValue = gamma; }
    virtual bool isGammaEnabled() const { return gammaEnabled; }

    // Status
    virtual RGB getCurrentColor() const { return currentColor; }
    virtual bool isOn() const { return brightness > 0; }
    virtual void off() { setBrightness(0); }
    virtual void on() { setBrightness(1.0); }

    // Hardware info
    virtual String getInfo() const;

protected:
    virtual void writeToPWM(uint8_t r, uint8_t g, uint8_t b);
    virtual uint8_t applyGamma(uint8_t value) const;
    virtual void updateHardware();
};

// Banya-specific LED strip with temperature/humidity awareness
class BanyaLEDStrip : public LEDStrip {
private:
    float currentTemp;
    float currentHumidity;

public:
    BanyaLEDStrip(uint8_t pinR, uint8_t pinG, uint8_t pinB);

    // Update environmental data
    void updateEnvironment(float temperatureC, float humidity);

    // Banya-specific modes
    void setTemperatureMode();
    void setHumidityMode();
    void setComfortMode();
    void setSafetyMode();
    void setRelaxMode();

    // Automatic mode based on conditions
    void setAutoMode();

    // Get environmental data
    float getTemperature() const { return currentTemp; }
    float getHumidity() const { return currentHumidity; }

private:
    RGB calculateTemperatureColor() const;
    RGB calculateHumidityColor() const;
    RGB calculateComfortColor() const;
    RGB calculateSafetyColor() const;
};

#endif // BANYA_LEDSTRIP_H