#ifndef SAUNA_HAL_PAGES_DALLAS_SENSORS_H
#define SAUNA_HAL_PAGES_DALLAS_SENSORS_H

#include "../DisplayPage.h"
#include "../DS18B20.h"

namespace HAL {

/**
 * @brief Страница отображения температур DS18B20
 *
 * DS18B20Manager автоматически обновляет температуры в фоне.
 * Страница только отображает текущие значения.
 */
class DallasSensorsPage : public DisplayPage {
private:
    DS18B20Manager* dsManager;
    float lastTemp1;
    float lastTemp2;
    unsigned long lastUpdate;
    const unsigned long updateInterval;

public:
    DallasSensorsPage(DS18B20Manager* ds, const String& title = "Dallas Sensors", unsigned long interval = 1000)
        : DisplayPage(title, 0),
          dsManager(ds),
          lastTemp1(-127),
          lastTemp2(-127),
          lastUpdate(0),
          updateInterval(interval) {}

    void onEnter() override {
        DisplayPage::onEnter();
        lastUpdate = 0;
        lastTemp1 = -127;
        lastTemp2 = -127;
    }

    void render(LCD& lcd, bool force = false) override {
        if (!dsManager) return;

        updateDisplay(lcd, force);
    }

private:
    void updateDisplay(LCD& lcd, bool force) {
        unsigned long now = millis();
        bool needUpdate = force || (now - lastUpdate >= updateInterval);

        if (!needUpdate) return;
        lastUpdate = now;

        // Заголовок
        lcd.line_printf(0, "%s", title.c_str());

        // Сенсор 1
        float temp1 = dsManager->getTemperature(0);
        if (dsManager->isConnected(0) && temp1 != DEVICE_DISCONNECTED_C) {
            lcd.line_printf(1, "T1: %.1f C", temp1);
            lastTemp1 = temp1;
        } else {
            lcd.line_printf(1, "T1: --.- C");
        }

        // Сенсор 2
        float temp2 = dsManager->getTemperature(1);
        if (dsManager->isConnected(1) && temp2 != DEVICE_DISCONNECTED_C) {
            lcd.line_printf(2, "T2: %.1f C", temp2);
            lastTemp2 = temp2;
        } else {
            lcd.line_printf(2, "T2: --.- C");
        }

        // Статус
        lcd.line_printf(3, "Sensors: %d found", dsManager->getSensorCount());
    }
};

} // namespace HAL

#endif // SAUNA_HAL_PAGES_DALLAS_SENSORS_H
