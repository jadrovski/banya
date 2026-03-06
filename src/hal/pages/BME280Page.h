#ifndef BANYA_HAL_PAGES_BME280_H
#define BANYA_HAL_PAGES_BME280_H

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
        lcd.line_printf(0, "%s", title.c_str());

        // Температура - строка 1
        lcd.line_printf(1, "T: %.1f C", temp);
        lastTemp = temp;

        // Влажность - строка 2
        lcd.line_printf(2, "H: %.1f %%", humidity);
        lastHumidity = humidity;

        // Давление - строка 3
        lcd.line_printf(3, "P: %d mmHg", (int)pressure_mmHg);
        lastPressure = pressure_mmHg;
    }
};

} // namespace HAL

#endif // BANYA_HAL_PAGES_BME280_H
