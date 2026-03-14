#ifndef BANYA_LCD_OTA_PRESENTER_H
#define BANYA_LCD_OTA_PRESENTER_H

#include "OTAPresenter.h"
#include "LCD.h"

namespace HAL {

/**
 * @brief LCD implementation of OTAPresenter
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
    explicit LCDOTAPresenter(LCD* display) : lcd(display) {}

    /**
     * @brief Initialize presenter (create custom characters if needed)
     */
    void begin() {
        if (lcd) {
            // Create full block character for progress bar (character 255)
            uint8_t fullBlock[8] = {
                0b11111,
                0b11111,
                0b11111,
                0b11111,
                0b11111,
                0b11111,
                0b11111,
                0b11111
            };
            lcd->createChar(255, fullBlock);
        }
    }

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
                lcd->write(byte(255)); // Full block
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
    LCD* lcd = nullptr;
};

} // namespace HAL

#endif // BANYA_LCD_OTA_PRESENTER_H
