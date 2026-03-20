#pragma once

#include "../ota/OTAPresenter.h"
#include "../hal/LCD.h"

/**
 * @brief LCD2004 implementation of OTAPresenter
 *
 * Displays OTA update status on the LCD screen.
 * This is the only class that knows about both OTA events and LCD.
 */
class LCDOTAPresenter : public OTAPresenter {
public:
    /**
     * @brief Construct a new LCD OTA Presenter
     * @param display Pointer to LCD instance
     */
    explicit LCDOTAPresenter(LCD2004* display) : lcd(display) {}

    void showStart() override {
        if (!lcd) return;

        lcd->clear();
        lcd->setCursor(0, 0);
        lcd->print("Firmware Update");
        lcd->setCursor(0, 2);
        lcd->print("Starting...");
    }

    void showEnd() override {
        if (!lcd) return;

        lcd->clear();
        lcd->setCursor(0, 0);
        lcd->print("Firmware Update");
        lcd->setCursor(0, 1);
        lcd->print("Update Complete!");
        lcd->setCursor(0, 2);
        lcd->print("Rebooting...");
    }

    void showProgress(const OTAProgress& progress) override {
        if (!lcd) return;

        // Show percentage on row 1
        lcd->setCursor(0, 1);
        lcd->print("Progress: ");
        lcd->print(progress.percent);
        lcd->print("%");

        // Progress bar on row 3
        lcd->setCursor(0, 3);
        const uint8_t barWidth = 20;
        const uint8_t filled = (progress.percent * barWidth) / 100;

        lcd->print("[");
        for (uint8_t i = 0; i < barWidth - 2; i++) {
            if (i < filled) {
                lcd->write(FILLED_BLOCK);
            } else {
                lcd->print("-");
            }
        }
        lcd->print("]");
    }

    void showError(OTAErrorType errorType, const String& message) override {
        if (!lcd) return;

        lcd->clear();
        lcd->setCursor(0, 0);
        lcd->print("OTA Error!");
        lcd->setCursor(0, 2);
        lcd->print(message);

        // Optional: log error type to serial
        Serial.printf("OTA: Error [%d]: %s\n", static_cast<int>(errorType), message.c_str());
    }

private:
    LCD2004* lcd = nullptr;
};
