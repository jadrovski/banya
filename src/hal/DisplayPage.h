#ifndef BANYA_HAL_DISPLAY_PAGE_H
#define BANYA_HAL_DISPLAY_PAGE_H

#include <Arduino.h>
#include "LCD.h"

namespace HAL {

/**
 * @brief Базовый класс для страницы дисплея
 *
 * Все страницы должны наследоваться от этого класса
 * и реализовывать метод render()
 */
class DisplayPage {
protected:
    String title;
    uint8_t pageIndex;
    bool visible;

public:
    /**
     * @brief Конструктор страницы
     * @param name Заголовок страницы
     * @param index Индекс страницы
     */
    DisplayPage(const String& name = "Page", uint8_t index = 0)
        : title(name), pageIndex(index), visible(true) {}

    virtual ~DisplayPage() = default;

    /**
     * @brief Отрисовка страницы (вызывается каждый кадр)
     * @param lcd Ссылка на LCD дисплей
     * @param forceForce Принудительная перерисовка
     */
    virtual void render(LCD& lcd, bool force = false) = 0;

    /**
     * @brief Инициализация страницы (вызывается при переключении)
     */
    virtual void onEnter() {
        visible = true;
    }

    /**
     * @brief Выход из страницы (вызывается при переключении)
     */
    virtual void onExit() {
        visible = false;
    }

    /**
     * @brief Получить заголовок страницы
     */
    const String& getTitle() const { return title; }

    /**
     * @brief Получить индекс страницы
     */
    uint8_t getIndex() const { return pageIndex; }

    /**
     * @brief Установить индекс страницы
     */
    void setIndex(uint8_t idx) { pageIndex = idx; }

    /**
     * @brief Проверка видимости страницы
     */
    bool isVisible() const { return visible; }
};

} // namespace HAL

#endif // BANYA_HAL_DISPLAY_PAGE_H
