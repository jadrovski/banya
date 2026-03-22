#pragma once

#include "../DisplayPage.h"
#include "../../hal/ITemperatureSensor.h"

/**
 * @brief Страница отображения температур DS18B20
 *
 * ITemperatureSensor автоматически обновляет температуры в фоне.
 * Страница только отображает текущие значения.
 */
class DallasSensorsPage : public DisplayPage {
private:
    ITemperatureSensor** tempSensorPtr;
    float lastTemp1;
    float lastTemp2;
    float lastTemp3;
    unsigned long lastUpdate;
    const unsigned long updateInterval;

    ITemperatureSensor* getSensor() const {
        return *tempSensorPtr;
    }

public:
    DallasSensorsPage(ITemperatureSensor** sensor, const String& title = "Dallas Sensors", unsigned long interval = 1000)
        : DisplayPage(title, 0),
          tempSensorPtr(sensor),
          lastTemp1(-127),
          lastTemp2(-127),
          lastTemp3(-127),
          lastUpdate(0),
          updateInterval(interval) {}

    void onEnter() override {
        DisplayPage::onEnter();
        lastUpdate = 0;
        lastTemp1 = -127;
        lastTemp2 = -127;
        lastTemp3 = -127;
    }

    void render(LCD2004& lcd, bool force = false) override {
        if (!getSensor()) return;

        updateDisplay(lcd, force);
    }

private:
    void updateDisplay(LCD2004& lcd, bool force) {
        unsigned long now = millis();
        bool needUpdate = force || (now - lastUpdate >= updateInterval);

        if (!needUpdate) return;
        lastUpdate = now;

        ITemperatureSensor* tempSensor = getSensor();

        // Заголовок
        lcd.line_printf(0, "%s", title.c_str());
        lcd.setCursor(lcd.getConfig().columns - 2, 0);
        lcd.print(tempSensor->getSensorCount());

        // Сенсор 1
        float temp1 = tempSensor->getTemperature(0);
        if (tempSensor->isConnected(0) && temp1 != DEVICE_DISCONNECTED_C) {
            lcd.line_printf(1, "T1: %.1f C", temp1);
            lastTemp1 = temp1;
        } else {
            lcd.line_printf(1, "T1: --.- C");
        }

        // Сенсор 2
        float temp2 = tempSensor->getTemperature(1);
        if (tempSensor->isConnected(1) && temp2 != DEVICE_DISCONNECTED_C) {
            lcd.line_printf(2, "T2: %.1f", temp2);
            lastTemp2 = temp2;
        } else {
            lcd.line_printf(2, "T2: --.- C");
        }

        // Сенсор 3
        float temp3 = tempSensor->getTemperature(2);
        if (tempSensor->isConnected(2) && temp3 != DEVICE_DISCONNECTED_C) {
            lcd.line_printf(3, "T3: %.1f", temp3);
            lastTemp3 = temp3;
        } else {
            lcd.line_printf(3, "T3: --.- C");
        }
    }
};
