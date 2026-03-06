#ifndef BANYA_LEDCONTROLLER_H
#define BANYA_LEDCONTROLLER_H
#include "color/RGB.h"

namespace HAL {
    class RGBLED;
}

// LED Strip Controller (manages multiple strips/patterns)
class LEDStripController {
private:
    HAL::RGBLED* strip;
    unsigned long lastUpdate;
    unsigned long effectStart;

    enum Effect {
        EFFECT_NONE,
        EFFECT_FADE,
        EFFECT_BLINK,
        EFFECT_PULSE,
        EFFECT_RAINBOW
    };

    Effect currentEffect;
    RGB effectColor1;
    RGB effectColor2;
    uint32_t effectDuration;
    uint8_t effectRepeats;
    uint8_t currentRepeat;

public:
    LEDStripController(HAL::RGBLED* strip);

    // Effect control
    void startFade(const RGB& from, const RGB& to, uint32_t duration);
    void startBlink(const RGB& color, uint32_t onTime, uint32_t offTime, uint8_t repeats = 255);
    void startPulse(const RGB& color, uint32_t period);
    void startRainbow(uint32_t cycleTime);
    void stopEffect();

    void off();

    // Animation update (call in loop())
    void handleLoop();

    // Check if effect is running
    bool isEffectRunning() const { return currentEffect != EFFECT_NONE; }

private:
    void updateFade();
    void updateBlink();
    void updatePulse();
    void updateRainbow();
};


#endif //BANYA_LEDCONTROLLER_H