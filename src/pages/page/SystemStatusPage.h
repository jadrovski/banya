#ifndef BANYA_HAL_PAGES_STATUS_H
#define BANYA_HAL_PAGES_STATUS_H

#include "../DisplayPage.h"
#include "../../hal/WiFi.h"

// ============================================================================
// Страница 4: Общий статус системы
// ============================================================================

/**
 * @brief Страница общего статуса системы
 */
class SystemStatusPage : public DisplayPage {
private:
    HAL::WiFiManager* wifi;
    unsigned long lastUpdate;
    const unsigned long updateInterval;

public:
    SystemStatusPage(HAL::WiFiManager* mgr, const String& title = "System Status")
        : DisplayPage(title, 3),
          wifi(mgr),
          lastUpdate(0),
          updateInterval(1000) {}

    void onEnter() override {
        DisplayPage::onEnter();
        lastUpdate = 0;
    }

    void render(HAL::LCD& lcd, bool force = false) override {
        unsigned long now = millis();
        bool needUpdate = force || (now - lastUpdate >= updateInterval);

        if (!needUpdate) return;
        lastUpdate = now;

        // Row 0: Uptime
        lcd.setCursor(0, 0);
        unsigned long uptime = millis() / 1000;
        lcd.print("Up: ");

        unsigned long days = uptime / 86400;
        unsigned long hours = (uptime % 86400) / 3600;
        unsigned long mins = (uptime % 3600) / 60;
        unsigned long secs = uptime % 60;

        if (days > 0) {
            lcd.print((int)days);
            lcd.print("d ");
        }
        if (hours < 10) lcd.print("0");
        lcd.print((int)hours);
        lcd.print(":");
        if (mins < 10) lcd.print("0");
        lcd.print((int)mins);
        lcd.print(":");
        if (secs < 10) lcd.print("0");
        lcd.print((int)secs);
        lcd.print("              ");

        // Row 1: Free heap
        lcd.setCursor(0, 1);
        lcd.print("Free: ");
        lcd.print((int)(ESP.getFreeHeap() / 1024));
        lcd.print("KB ");
    }
};

#endif // BANYA_HAL_PAGES_STATUS_H
