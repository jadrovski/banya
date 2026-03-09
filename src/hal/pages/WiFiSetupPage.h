#ifndef BANYA_HAL_PAGES_WIFI_SETUP_H
#define BANYA_HAL_PAGES_WIFI_SETUP_H

#include "../DisplayPage.h"
#include "../WiFi.h"
#include "../WiFiSettings.h"

namespace HAL {

/**
 * @brief Страница настройки WiFi (режим конфигурации)
 * 
 * Отображает:
 * - Статус режима настройки WiFi
 * - IP адрес для подключения
 * - SSID точки доступа
 * - Инструкцию для пользователя
 */
class WiFiSetupPage : public DisplayPage {
private:
    WiFiManager* wifiManager;
    WiFiSettings* wifiSettings;
    String lastAPIP;
    String lastAPSSID;
    unsigned long lastUpdate;
    const unsigned long updateInterval;
    bool apEnabled;

public:
    WiFiSetupPage(WiFiManager* mgr, WiFiSettings* settings, const String& title = "WiFi Setup")
        : DisplayPage(title, 3),
          wifiManager(mgr),
          wifiSettings(settings),
          lastUpdate(0),
          updateInterval(500),
          apEnabled(false) {}

    void onEnter() override {
        DisplayPage::onEnter();
        lastUpdate = 0;
        apEnabled = false;
    }

    void render(LCD& lcd, bool force = false) override {
        if (!wifiManager) return;

        unsigned long now = millis();
        bool needUpdate = force || (now - lastUpdate >= updateInterval);

        if (!needUpdate) return;
        lastUpdate = now;

        // Проверяем, включён ли AP режим
        bool currentAPEnabled = wifiManager->isAPEnabled();

        // Строка 0: Заголовок
        if (currentAPEnabled) {
            lcd.line_printf(0, "\x04 WiFi Setup Mode"); // Heart icon if available
        } else {
            lcd.line_printf(0, "WiFi Setup Ready");
        }

        // Строка 1: Статус AP
        if (currentAPEnabled) {
            const String apiP = wifiManager->getAPIPAddressString();
            const APConfig& apCfg = wifiManager->getAPConfig();
            
            lcd.line_printf(1, "AP: %s", apCfg.ssid);
            
            // Строка 2: IP адрес
            lcd.line_printf(2, "http://%s", apiP.c_str());
            
            // Строка 3: Инструкция
            lcd.line_printf(3, "Connect & open browser");
            
            apEnabled = true;
        } else {
            // AP ещё не включён - показываем инструкцию
            lcd.line_printf(1, "Visit: /wifi");
            lcd.line_printf(2, "on main web interface");
            lcd.line_printf(3, "Long press: Enable AP");
            
            apEnabled = false;
        }

        lastAPIP = wifiManager->getAPIPAddressString();
        lastAPSSID = wifiManager->getAPConfig().ssid;
    }

    /**
     * @brief Проверка, активен ли режим настройки
     * @return true если AP режим включён
     */
    bool isSetupActive() const {
        return wifiManager ? wifiManager->isAPEnabled() : false;
    }

    /**
     * @brief Получить количество подключённых клиентов
     * @return Количество станций подключённых к AP
     */
    int getConnectedClients() const {
        if (!wifiManager || !apEnabled) return 0;
        return wifiManager->getAPStationCount();
    }
};

} // namespace HAL

#endif // BANYA_HAL_PAGES_WIFI_SETUP_H
