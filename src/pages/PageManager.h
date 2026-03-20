#pragma once

#include <Arduino.h>
#include <vector>
#include <memory>
#include "DisplayPage.h"

/**
 * @brief Менеджер страниц дисплея
 *
 * Управление множеством страниц с возможностью переключения
 */
class PageManager {
private:
    std::vector<std::unique_ptr<DisplayPage>> pages;
    uint8_t currentPageIndex;
    LCD2004* lcd;
    bool autoPageSwitch;
    unsigned long lastPageSwitchTime;
    uint32_t pageSwitchInterval;

public:
    /**
     * @brief Конструктор PageManager
     * @param display Ссылка на LCD дисплей
     */
    explicit PageManager(LCD2004& display)
        : currentPageIndex(0),
          lcd(&display),
          autoPageSwitch(false),
          lastPageSwitchTime(0),
          pageSwitchInterval(0) {}

    /**
     * @brief Добавить страницу
     * @param page Указатель на страницу
     * @return Индекс добавленной страницы
     */
    uint8_t addPage(std::unique_ptr<DisplayPage> page) {
        page->setIndex(pages.size());
        pages.push_back(std::move(page));
        return pages.size() - 1;
    }

    /**
     * @brief Переключиться на страницу по индексу
     * @param index Индекс страницы
     * @return true если успешно
     */
    bool goToPage(uint8_t index) {
        if (index >= pages.size()) return false;

        // Выход из текущей страницы
        if (currentPageIndex < pages.size() && pages[currentPageIndex]) {
            pages[currentPageIndex]->onExit();
        }

        currentPageIndex = index;

        // Вход в новую страницу
        if (pages[currentPageIndex]) {
            pages[currentPageIndex]->onEnter();
            if (lcd) {
                lcd->clear();
            }
        }

        lastPageSwitchTime = millis();
        return true;
    }

    /**
     * @brief Переключиться на следующую страницу
     * @return true если успешно
     */
    bool nextPage() {
        if (pages.empty()) return false;

        uint8_t nextIndex = (currentPageIndex + 1) % pages.size();
        return goToPage(nextIndex);
    }

    /**
     * @brief Переключиться на предыдущую страницу
     * @return true если успешно
     */
    bool prevPage() {
        if (pages.empty()) return false;

        uint8_t prevIndex = (currentPageIndex == 0) ? pages.size() - 1 : currentPageIndex - 1;
        return goToPage(prevIndex);
    }

    /**
     * @brief Отрисовка текущей страницы (вызывать в loop)
     * @param force Принудительная перерисовка
     */
    void render(bool force = false) {
        if (currentPageIndex >= pages.size() || !pages[currentPageIndex] || !lcd) {
            return;
        }
        pages[currentPageIndex]->render(*lcd, force);
    }

    /**
     * @brief Получить текущий индекс страницы
     */
    uint8_t getCurrentPageIndex() const { return currentPageIndex; }

    /**
     * @brief Получить текущую страницу
     */
    DisplayPage* getCurrentPage() {
        if (currentPageIndex >= pages.size()) return nullptr;
        return pages[currentPageIndex].get();
    }

    /**
     * @brief Получить количество страниц
     */
    uint8_t getPageCount() const { return pages.size(); }

    /**
     * @brief Получить страницу по индексу
     */
    DisplayPage* getPage(uint8_t index) {
        if (index >= pages.size()) return nullptr;
        return pages[index].get();
    }

    /**
     * @brief Включить авто-переключение страниц
     * @param intervalMs Интервал переключения в мс (0 = выкл)
     */
    void setAutoSwitch(uint32_t intervalMs) {
        pageSwitchInterval = intervalMs;
        autoPageSwitch = (intervalMs > 0);
        lastPageSwitchTime = millis();
    }

    /**
     * @brief Проверка и выполнение авто-переключения
     */
    void updateAutoSwitch() {
        if (!autoPageSwitch || pages.empty()) return;

        if (millis() - lastPageSwitchTime >= pageSwitchInterval) {
            nextPage();
            lastPageSwitchTime = millis();
        }
    }

    void handleLoop() {
        render();
        updateAutoSwitch();
    }

    /**
     * @brief Очистить все страницы
     */
    void clear() {
        pages.clear();
        currentPageIndex = 0;
    }

    /**
     * @brief Получить информацию о менеджере
     */
    String getInfo() const {
        String info = "Pages: ";
        info += pages.size();
        info += " | Current: ";
        info += currentPageIndex;

        if (currentPageIndex < pages.size() && pages[currentPageIndex]) {
            info += " (";
            info += pages[currentPageIndex]->getTitle();
            info += ")";
        }

        if (autoPageSwitch) {
            info += " | Auto: ";
            info += pageSwitchInterval;
            info += "ms";
        }

        return info;
    }
};

/**
 * @brief Отрисовка индикатора текущей страницы
 * @param lcd Ссылка на LCD
 * @param current Текущий индекс
 * @param total Всего страниц
 */
inline void drawPageIndicator(LCD2004& lcd, uint8_t current, uint8_t total) {
    if (total <= 1) return;

    lcd.setCursor(0, 3);
    lcd.print("                    "); // Очистка строки

    lcd.setCursor(0, 3);
    lcd.print("[");

    for (uint8_t i = 0; i < total; i++) {
        if (i == current) {
            lcd.print("#"); // Текущая страница
        } else if (i < total - 1) {
            lcd.print("-"); // Другие страницы
        }
    }

    lcd.print("]");
    lcd.print((int)(current + 1));
    lcd.print("/");
    lcd.print((int)total);

    // Подсказка
    lcd.print(" >Next");
}
