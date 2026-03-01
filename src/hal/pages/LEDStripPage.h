#ifndef SAUNA_HAL_PAGES_LEDSTRIP_H
#define SAUNA_HAL_PAGES_LEDSTRIP_H

#include "../DisplayPage.h"
#include "../RGBLED.h"
#include "../../LEDController.h"

namespace HAL {

/**
 * @brief Страница статуса LED strip
 * 
 * Отображает:
 * - Текущий цвет (RGB значения)
 * - Яркость (%)
 * - Статус гамма-коррекции
 * - Текущий эффект (если запущен)
 * - Статус включения
 */
class LEDStripPage : public DisplayPage {
private:
    RGBLED* ledStrip;
    LEDStripController* ledController;
    unsigned long lastUpdate;
    const unsigned long updateInterval;

public:
    /**
     * @brief Конструктор страницы LED strip
     * @param strip Указатель на объект RGBLED
     * @param controller Указатель на объект LEDStripController (может быть nullptr)
     * @param title Заголовок страницы
     */
    LEDStripPage(RGBLED* strip, 
                 LEDStripController* controller = nullptr,
                 const String& title = "LED Strip Status")
        : DisplayPage(title, 0),
          ledStrip(strip),
          ledController(controller),
          lastUpdate(0),
          updateInterval(500) {}

    /**
     * @brief Инициализация при входе на страницу
     */
    void onEnter() override {
        DisplayPage::onEnter();
        lastUpdate = 0; // Force update on enter
    }

    /**
     * @brief Отрисовка страницы
     * @param lcd Ссылка на LCD дисплей
     * @param force Принудительная перерисовка
     */
    void render(LCD& lcd, bool force = false) override {
        unsigned long now = millis();
        bool needUpdate = force || (now - lastUpdate >= updateInterval);

        if (!needUpdate) return;
        lastUpdate = now;

        if (!ledStrip) {
            lcd.setCursor(0, 0);
            lcd.print("LED: Not initialized");
            return;
        }

        // Row 0: Status and Power
        lcd.setCursor(0, 0);
        if (ledStrip->isOn()) {
            lcd.print("LED: ON ");
        } else {
            lcd.print("LED: OFF");
        }
        
        // Brightness
        lcd.setCursor(10, 0);
        lcd.print("B:");
        lcd.print((int)(ledStrip->getBrightness() * 100));
        lcd.print("%  ");

        // Row 1: Current Color (RGB)
        RGB color = ledStrip->getCurrentColor();
        lcd.setCursor(0, 1);
        lcd.print("RGB:");
        lcd.print(color.red);
        lcd.print(",");
        lcd.print(color.green);
        lcd.print(",");
        lcd.print(color.blue);
        lcd.print("   ");

        // Row 2: Gamma and Effect
        lcd.setCursor(0, 2);
        lcd.print("Gamma:");
        lcd.print(ledStrip->getConfig().enableGamma ? "ON " : "OFF");
        
        // Effect status
        if (ledController && ledController->isEffectRunning()) {
            lcd.setCursor(12, 2);
            lcd.print(" FX:ON ");
        } else {
            lcd.setCursor(12, 2);
            lcd.print("     ");
        }

        // Row 3: Info or Help
        lcd.setCursor(0, 3);
        lcd.print("Serial: R/G/B/W/P/Q ");
    }
};

} // namespace HAL

#endif // SAUNA_HAL_PAGES_LEDSTRIP_H
