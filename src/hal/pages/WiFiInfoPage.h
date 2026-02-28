#ifndef SAUNA_HAL_PAGES_WIFI_INFO_H
#define SAUNA_HAL_PAGES_WIFI_INFO_H

#include "../DisplayPage.h"
#include "../WiFi.h"

namespace HAL {

/**
 * @brief Страница отображения WiFi информации с MAC адресом
 */
class WiFiInfoPage : public DisplayPage {
private:
    WiFiManager* wifi;
    String lastIP;
    String lastStatus;
    unsigned long lastUpdate;
    const unsigned long updateInterval;

public:
    WiFiInfoPage(WiFiManager* mgr, const String& title = "WiFi Info")
        : DisplayPage(title, 3),
          wifi(mgr),
          lastUpdate(0),
          updateInterval(1000) {}

    void onEnter() override {
        DisplayPage::onEnter();
        lastUpdate = 0;
    }

    void render(LCD& lcd, bool force = false) override {
        if (!wifi) return;

        unsigned long now = millis();
        bool needUpdate = force || (now - lastUpdate >= updateInterval);

        if (!needUpdate) return;
        lastUpdate = now;

        // Строка 1: WiFi иконка и SSID
        const char* ssid = wifi->getConfig().ssid;
        if (ssid && strlen(ssid) > 0) {
            lcd.line_printf(0, "%c %s", WIFI_CHAR, ssid);
        } else {
            lcd.line_printf(0, "---");
        }

        // Строка 2: Статус или RSSI + канал
        if (wifi->isConnected()) {
            const int rssi = wifi->getRSSI();
            const int quality = wifi->getSignalQuality();
            const int ch = wifi->getChannel();
            lcd.line_printf(1, "%3ddBm(%3d%%) Ch:%2d", rssi, quality, ch);
        } else {
            const String status = wifi->getStatusString();
            lcd.line_printf(1, status.c_str());
        }

        // Строка 3: IP адрес
        const String ip = wifi->getIPAddressString();
        lcd.line_printf(2, ip.c_str());

        // Строка 4: MAC адрес
        lcd.setCursor(0, 3);
        const String mac = wifi->getMACAddress();
        lcd.line_printf(3, mac.c_str());

        lastIP = ip;
        lastStatus = wifi->getStatusString();
    }
};

} // namespace HAL

#endif // SAUNA_HAL_PAGES_WIFI_INFO_H
