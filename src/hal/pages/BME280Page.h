#ifndef SAUNA_HAL_PAGES_BME280_H
#define SAUNA_HAL_PAGES_BME280_H

#include "../DisplayPage.h"
#include "../BME280.h"

namespace HAL {

/**
 * @brief Страница отображения данных BME280
 */
class BME280Page : public DisplayPage {
private:
    BME280Sensor* bme;
    float lastTemp;
    float lastHumidity;
    float lastPressure;
    unsigned long lastUpdate;
    const unsigned long updateInterval;
    const float hpaToMmHg;

public:
    BME280Page(BME280Sensor* sensor, const String& title = "BME280 Sensor", float hpaConv = 0.75006156f)
        : DisplayPage(title, 1),
          bme(sensor),
          lastTemp(-100),
          lastHumidity(-1),
          lastPressure(0),
          lastUpdate(0),
          updateInterval(1000),
          hpaToMmHg(hpaConv) {}

    void onEnter() override {
        DisplayPage::onEnter();
        lastUpdate = 0;
    }

    void render(LCD& lcd, bool force = false) override {
        if (!bme || !bme->isInitialized()) return;

        unsigned long now = millis();
        bool needUpdate = force || (now - lastUpdate >= updateInterval);

        if (!needUpdate) return;
        lastUpdate = now;

        // Чтение данных
        bme->readData();
        float temp = bme->getTemperature();
        float humidity = bme->getHumidity();
        float pressure_mmHg = bme->getPressure_mmHg(hpaToMmHg);

        // Заголовок
        lcd.setCursor(0, 0);
        lcd.print(title);
        lcd.print("            ");

        // Температура - строка 1
        lcd.setCursor(0, 1);
        lcd.print("T: ");
        lcd.print(temp, 1);
        lcd.print("C ");
        if (abs(temp - lastTemp) > 0.05) {
            lcd.print(">");
        } else {
            lcd.print(" ");
        }
        lastTemp = temp;

        // Влажность - строка 2
        lcd.setCursor(0, 2);
        lcd.print("H: ");
        lcd.print(humidity, 1);
        lcd.print("% ");
        if (abs(humidity - lastHumidity) > 0.5) {
            lcd.print(">");
        } else {
            lcd.print(" ");
        }
        lastHumidity = humidity;

        // Давление - строка 3
        lcd.setCursor(0, 3);
        lcd.print("P: ");
        lcd.print((int)pressure_mmHg);
        lcd.print(" mmHg ");
        if (abs(pressure_mmHg - lastPressure) > 0.5) {
            lcd.print(">");
        } else {
            lcd.print(" ");
        }
        lastPressure = pressure_mmHg;
    }
};

} // namespace HAL

#endif // SAUNA_HAL_PAGES_BME280_H
