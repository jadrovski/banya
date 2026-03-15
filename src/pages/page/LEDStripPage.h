#ifndef BANYA_HAL_PAGES_LEDSTRIP_H
#define BANYA_HAL_PAGES_LEDSTRIP_H

#include "../DisplayPage.h"
#include "../../hal/RGBLED.h"

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
    unsigned long lastUpdate;
    const unsigned long updateInterval;

public:
    /**
     * @brief Конструктор страницы LED strip
     * @param strip Указатель на объект RGBLED
     * @param title Заголовок страницы
     */
    LEDStripPage(RGBLED* strip,
                 const String& title = "LED Strip Status")
        : DisplayPage(title, 0),
          ledStrip(strip),
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
    void render(LCD2004& lcd, bool force = false) override {
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
        lcd.print("Br: ");
        lcd.print((int)(ledStrip->getBrightness() * 100));
        lcd.print("%  ");

        // Row 1: Current Color (RGB)
        RGB color = ledStrip->getCurrentColor();
        lcd.setCursor(0, 1);
        lcd.print("RGB: ");
        lcd.print(color.red);
        lcd.print(",");
        lcd.print(color.green);
        lcd.print(",");
        lcd.print(color.blue);
        lcd.print("   ");

        // Row 2: Gamma and Effect
        lcd.setCursor(0, 2);
        lcd.print("Gamma: ");
        lcd.print(ledStrip->getConfig().enableGamma ? "ON " : "OFF");

        lcd.line_printf(3, "Fr: %d Res: %d", ledStrip->getConfig().pwmFrequency, ledStrip->getConfig().pwmResolution);
    }
};

#endif // BANYA_HAL_PAGES_LEDSTRIP_H
