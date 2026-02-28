#ifndef SAUNA_HAL_PAGES_STATUS_H
#define SAUNA_HAL_PAGES_STATUS_H

#include "../DisplayPage.h"
#include "../WiFi.h"
#include "../../color/SaunaColors.h"

namespace HAL {

// ============================================================================
// Страница 4: Общий статус системы
// ============================================================================

/**
 * @brief Страница общего статуса системы
 */
class SystemStatusPage : public DisplayPage {
private:
    WiFiManager* wifi;
    String currentMode;
    unsigned long lastUpdate;
    const unsigned long updateInterval;

public:
    SystemStatusPage(WiFiManager* mgr, const String& title = "System Status")
        : DisplayPage(title, 3),
          wifi(mgr),
          lastUpdate(0),
          updateInterval(1000) {}

    void setMode(const String& mode) {
        currentMode = mode;
    }

    void onEnter() override {
        DisplayPage::onEnter();
        lastUpdate = 0;
    }

    void render(LCD& lcd, bool force = false) override {
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

} // namespace HAL

#endif // SAUNA_HAL_PAGES_STATUS_H
