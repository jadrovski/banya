#ifndef BANYA_HAL_PAGES_WIFI_SETUP_H
#define BANYA_HAL_PAGES_WIFI_SETUP_H

#include "../DisplayPage.h"
#include "../../hal/WiFi.h"
#include "../../hal/WiFiSettings.h"

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
    HAL::WiFiManager* wifiManager;
    HAL::WiFiSettings* wifiSettings;
    String lastAPIP;
    String lastAPSSID;
    unsigned long lastUpdate;
    const unsigned long updateInterval;
    bool apEnabled;

public:
    WiFiSetupPage(HAL::WiFiManager* mgr, HAL::WiFiSettings* settings, const String& title = "WiFi Setup")
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

    void render(HAL::LCD& lcd, bool force = false) override {
        if (!wifiManager) return;

        unsigned long now = millis();
        bool needUpdate = force || (now - lastUpdate >= updateInterval);

        if (!needUpdate) return;
        lastUpdate = now;

        // Проверяем, включён ли AP режим
        bool currentAPEnabled = wifiManager->isAPEnabled();
        const HAL::APConfig& apCfg = wifiManager->getAPConfig();
        const String apiP = wifiManager->getAPIPAddressString();

        // Строка 0: Заголовок со статусом AP (ON/OFF)
        lcd.line_printf(0, "WiFi Setup: %s", currentAPEnabled ? "ON" : "OFF");

        // Строка 1: SSID
        if (currentAPEnabled && apCfg.ssid && strlen(apCfg.ssid) > 0) {
            lcd.line_printf(1, "SSID: %.16s", apCfg.ssid);
        } else {
            lcd.line_printf(1, "SSID: --");
        }

        // Строка 2: Password
        if (currentAPEnabled && apCfg.password && strlen(apCfg.password) > 0) {
            lcd.line_printf(2, "Pass: %.16s", apCfg.password);
        } else {
            lcd.line_printf(2, "Pass: --");
        }

        // Строка 3: IP адрес
        if (currentAPEnabled) {
            lcd.line_printf(3, "IP: %s", apiP.c_str());
        } else {
            lcd.line_printf(3, "IP: --");
        }

        lastAPIP = apiP;
        lastAPSSID = apCfg.ssid ? apCfg.ssid : "";
        apEnabled = currentAPEnabled;
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

#endif // BANYA_HAL_PAGES_WIFI_SETUP_H
