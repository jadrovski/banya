#ifndef SAUNA_HAL_PAGES_DALLAS_SENSORS_H
#define SAUNA_HAL_PAGES_DALLAS_SENSORS_H

#include "../DisplayPage.h"
#include "../DS18B20.h"

namespace HAL {

/**
 * @brief Страница отображения температур DS18B20
 */
class DallasSensorsPage : public DisplayPage {
private:
    DS18B20Manager* dsManager;
    float lastTemp1;
    float lastTemp2;
    unsigned long lastUpdate;
    const unsigned long updateInterval;

public:
    DallasSensorsPage(DS18B20Manager* ds, const String& title = "Dallas Sensors")
        : DisplayPage(title, 0),
          dsManager(ds),
          lastTemp1(-127),
          lastTemp2(-127),
          lastUpdate(0),
          updateInterval(1000) {}

    void onEnter() override {
        DisplayPage::onEnter();
        lastUpdate = 0; // Принудительное обновление при входе
    }

    void render(LCD& lcd, bool force = false) override {
        if (!dsManager) return;

        unsigned long now = millis();
        bool needUpdate = force || (now - lastUpdate >= updateInterval);

        if (!needUpdate) return;
        lastUpdate = now;

        // Запрос температур
        dsManager->requestTemperatures();
        while (!dsManager->isConversionComplete()) {
            yield();
        }
        dsManager->updateTemperatures();

        // Заголовок
        lcd.setCursor(0, 0);
        lcd.print(title);
        lcd.print("          ");

        // Сенсор 1
        float temp1 = dsManager->getTemperature(0);
        if (dsManager->isConnected(0) && temp1 != DEVICE_DISCONNECTED_C) {
            lcd.setCursor(0, 1);
            lcd.print("T1: ");
            lcd.print(temp1, 1);
            lcd.print("C ");
            if (temp1 != lastTemp1) {
                lcd.print(">"); // Индикатор изменения
            } else {
                lcd.print(" ");
            }
            lastTemp1 = temp1;
        } else {
            lcd.setCursor(0, 1);
            lcd.print("T1: --.- C      ");
        }

        // Сенсор 2
        float temp2 = dsManager->getTemperature(1);
        if (dsManager->isConnected(1) && temp2 != DEVICE_DISCONNECTED_C) {
            lcd.setCursor(0, 2);
            lcd.print("T2: ");
            lcd.print(temp2, 1);
            lcd.print("C ");
            if (temp2 != lastTemp2) {
                lcd.print(">");
            } else {
                lcd.print(" ");
            }
            lastTemp2 = temp2;
        } else {
            lcd.setCursor(0, 2);
            lcd.print("T2: --.- C      ");
        }

        // Статус
        lcd.setCursor(0, 3);
        lcd.print("Sensors: ");
        lcd.print(dsManager->getSensorCount());
        lcd.print(" found    ");
    }
};

} // namespace HAL

#endif // SAUNA_HAL_PAGES_DALLAS_SENSORS_H
