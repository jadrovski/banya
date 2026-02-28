#ifndef SAUNA_COLORMODEL_H
#define SAUNA_COLORMODEL_H

#include "RGB.h"

// Color presets for sauna
namespace SaunaColors {
    // Temperature-based colors
    static RGB cold() { return {100, 150, 255}; } // Cool blue
    static RGB warm() { return {255, 150, 50}; } // Warm orange
    static RGB hot() { return {255, 50, 0}; } // Hot red

    // Comfort colors
    static RGB comfortable() { return {100, 255, 100}; } // Green
    static RGB warning() { return {255, 255, 0}; } // Yellow
    static RGB danger() { return {255, 50, 0}; } // Red

    // Relaxation colors
    static RGB calm() { return {150, 150, 255}; } // Lavender
    static RGB peaceful() { return {100, 200, 200}; } // Teal
    static RGB zen() { return {50, 150, 100}; } // Sage green

    // Special effects
    static RGB steam() { return {200, 200, 255}; } // Misty blue
    static RGB fire() { return {255, 100, 0}; } // Fire orange
    static RGB ice() { return {100, 200, 255}; } // Ice blue
}

#endif // SAUNA_COLORMODEL_H
