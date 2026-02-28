#ifndef SAUNA_HAL_LCD_H
#define SAUNA_HAL_LCD_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "I2CDevice.h"

namespace HAL {

#define WIFI_CHAR byte(1)

/**
 * @brief Конфигурация LCD дисплея
 */
struct LCDConfig {
    uint8_t i2cAddress;      // I2C адрес (по умолчанию 0x27)
    uint8_t sdaPin;          // GPIO SDA (по умолчанию 21)
    uint8_t sclPin;          // GPIO SCL (по умолчанию 22)
    uint8_t columns;         // Количество колонок (по умолчанию 20)
    uint8_t rows;            // Количество строк (по умолчанию 4)
    bool backlightOnStart;   // Подсветка включена при старте (по умолчанию true)
    bool cursorOnStart;      // Курсор включен при старте (по умолчанию false)

    LCDConfig(
        uint8_t addr = 0x27,
        uint8_t sda = 21,
        uint8_t scl = 22,
        uint8_t cols = 20,
        uint8_t rows = 4,
        bool backlight = true,
        bool cursor = false
    ) : i2cAddress(addr), sdaPin(sda), sclPin(scl),
        columns(cols), rows(rows), backlightOnStart(backlight), cursorOnStart(cursor) {}
};

/**
 * @brief Hardware Access Layer для LCD 2004 I2C дисплея
 * 
 * Инкапсулирует управление LCD дисплеем с I2C интерфейсом.
 * Поддерживает дисплеи 20x4, 16x2 и другие совместимые.
 */
class LCD : public I2CDevice {
private:
    LCDConfig config;
    LiquidCrystal_I2C* lcd;
    bool backlightState;
    bool cursorState;

    /**
     * @brief Внутренняя функция форматирования и вывода
     */
    void printFormatted(const char* format, va_list args) {
        char buffer[config.columns + 1];

        const int len = vsnprintf(buffer, sizeof(buffer), format, args);

        if (len < config.columns) {
            for (int i = len; i < config.columns; i++) {
                buffer[i] = ' ';
            }
            buffer[config.columns] = '\0';
        }

        lcd->print(buffer);
    }

public:
    /**
     * @brief Конструктор LCD
     * @param cfg Конфигурация дисплея
     */
    explicit LCD(const LCDConfig& cfg = LCDConfig())
        : I2CDevice(cfg.i2cAddress, cfg.sdaPin, cfg.sclPin),
          config(cfg),
          lcd(nullptr),
          backlightState(cfg.backlightOnStart),
          cursorState(cfg.cursorOnStart) {}

    ~LCD() override {
        if (lcd) delete lcd;
    }

    /**
     * @brief Инициализация дисплея
     * @return true если успешно
     */
    bool begin() override {
        lcd = new LiquidCrystal_I2C(config.i2cAddress, config.columns, config.rows);
        lcd->init(config.sdaPin, config.sclPin);

        if (config.backlightOnStart) {
            lcd->backlight();
            backlightState = true;
        } else {
            lcd->noBacklight();
            backlightState = false;
        }

        if (config.cursorOnStart) {
            lcd->cursor();
            cursorState = true;
        } else {
            lcd->noCursor();
            cursorState = false;
        }

        initialized = true;
        return true;
    }

    /**
     * @brief Проверка подключения дисплея
     */
    bool isConnected() override {
        if (!lcd) return false;
        return I2CDevice::isConnected();
    }

    // === Базовые операции ===

    /**
     * @brief Очистка экрана
     */
    void clear() {
        if (lcd) lcd->clear();
    }

    /**
     * @brief Возврат курсора в начало
     */
    void home() {
        if (lcd) lcd->home();
    }

    /**
     * @brief Установка позиции курсора
     * @param col Колонка (0-based)
     * @param row Строка (0-based)
     */
    void setCursor(uint8_t col, uint8_t row) {
        if (lcd) lcd->setCursor(col, row);
    }

    /**
     * @brief Вывод символа
     * @param c Символ или код пользовательского символа (0-7)
     */
    size_t write(uint8_t c) {
        return lcd ? lcd->write(c) : 0;
    }

    void line_printf(const char* format, ...) {
        va_list args;
        va_start(args, format);
        printFormatted(format, args);
        va_end(args);
    }

    /**
     * @brief Форматированный вывод на указанную строку
     * @param row Номер строки (0-based)
     * @param format Формат строки (printf-style)
     * @param ... Аргументы формата
     */
    void line_printf(uint8_t row, const char* format, ...) {
        va_list args;
        va_start(args, format);
        setCursor(0, row);
        printFormatted(format, args);
        va_end(args);
    }

    size_t print(const String& s) {
        return lcd ? lcd->print(s) : 0;
    }

    size_t print(const char* str) {
        return lcd ? lcd->print(str) : 0;
    }

    size_t println(const String& s) {
        return lcd ? lcd->println(s) : 0;
    }

    size_t println(const char* str) {
        return lcd ? lcd->println(str) : 0;
    }

    size_t print(int val) {
        return lcd ? lcd->print(val) : 0;
    }

    size_t print(float val, int digits = 2) {
        return lcd ? lcd->print(val, digits) : 0;
    }

    // === Форматированный вывод ===

    /**
     * @brief Вывод текста с позиционированием
     */
    void printAt(uint8_t col, uint8_t row, const String& text) {
        if (!lcd) return;
        setCursor(col, row);
        print(text);
    }

    /**
     * @brief Очистка конкретной строки
     * @param lineNumber Номер строки (0-based)
     */
    void clearLine(uint8_t lineNumber) {
        if (!lcd || lineNumber >= config.rows) return;
        setCursor(0, lineNumber);
        for (uint8_t i = 0; i < config.columns; i++) {
            write(' ');
        }
        setCursor(0, lineNumber);
    }

    // === Подсветка и курсор ===

    /**
     * @brief Включение подсветки
     */
    void backlight() {
        if (lcd) {
            lcd->backlight();
            backlightState = true;
        }
    }

    /**
     * @brief Выключение подсветки
     */
    void noBacklight() {
        if (lcd) {
            lcd->noBacklight();
            backlightState = false;
        }
    }

    /**
     * @brief Получить состояние подсветки
     */
    bool isBacklightOn() const { return backlightState; }

    /**
     * @brief Включение курсора
     */
    void cursor() {
        if (lcd) {
            lcd->cursor();
            cursorState = true;
        }
    }

    /**
     * @brief Выключение курсора
     */
    void noCursor() {
        if (lcd) {
            lcd->noCursor();
            cursorState = false;
        }
    }

    /**
     * @brief Получить состояние курсора
     */
    bool isCursorVisible() const { return cursorState; }

    /**
     * @brief Включение/выключение мигания курсора
     */
    void blink() {
        if (lcd) lcd->blink();
    }

    void noBlink() {
        if (lcd) lcd->noBlink();
    }

    // === Пользовательские символы ===

    /**
     * @brief Создание пользовательского символа
     * @param slot Номер слота (0-7)
     * @param data Массив 8 байт (битовая карта 5x8)
     */
    void createChar(uint8_t slot, const uint8_t* data) {
        if (lcd && slot < 8) {
            lcd->createChar(slot, const_cast<uint8_t*>(data));
        }
    }

    // === Информация ===

    /**
     * @brief Получить информацию о дисплее
     */
    String getInfo() const override {
        String info = "LCD ";
        info += config.columns;
        info += "x";
        info += config.rows;
        info += " @ 0x";
        info += String(config.i2cAddress, HEX);
        info += " (SDA:";
        info += config.sdaPin;
        info += ", SCL:";
        info += config.sclPin;
        info += ")";
        return info;
    }

    /**
     * @brief Получить конфигурацию
     */
    const LCDConfig& getConfig() const { return config; }
};

} // namespace HAL

#endif // SAUNA_HAL_LCD_H
