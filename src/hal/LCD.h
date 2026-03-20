#pragma once

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "I2CDevice.h"

#define WIFI_CHAR byte(1)
#define RUSSIAN_B_CHAR byte(2)
#define RUSSIAN_YA_CHAR byte(3)

/**
 * @brief Конфигурация LCD дисплея
 */
struct LCD2004Config {
    uint8_t i2cAddress; // I2C адрес (по умолчанию 0x27)
    I2CBus& bus;        // Ссылка на I2C шину
    uint8_t columns;    // Количество колонок (по умолчанию 20)
    uint8_t rows;       // Количество строк (по умолчанию 4)
    bool backlightOnStart; // Подсветка включена при старте (по умолчанию true)
    bool cursorOnStart;    // Курсор включен при старте (по умолчанию false)

    LCD2004Config(
        uint8_t addr,
        I2CBus& i2cBus,
        uint8_t cols = 20,
        uint8_t rows = 4,
        bool backlight = true,
        bool cursor = false
    ) : i2cAddress(addr), bus(i2cBus),
        columns(cols), rows(rows), backlightOnStart(backlight), cursorOnStart(cursor) {
    }
};

/**
 * @brief Hardware Access Layer для LCD 2004 I2C дисплея
 *
 * Инкапсулирует управление LCD дисплеем с I2C интерфейсом.
 * Поддерживает дисплеи 20x4, 16x2 и другие совместимые.
 */
class LCD2004 : public I2CDevice {
private:
    static const byte smiley[8];
    static const byte russianB[8];
    static const byte russianYA[8];
    static const byte wifiIcon[8];
    static const byte touchIcon[8];
    static const byte arrowRight[8];

    LCD2004Config config;
    LiquidCrystal_I2C *lcd;
    bool backlightState;
    bool cursorState;

    /**
     * @brief Внутренняя функция форматирования и вывода
     */
    void printFormatted(const char *format, va_list args) {
        char buffer[config.columns + 1];

        const int len = vsnprintf(buffer, sizeof(buffer), format, args);

        if (len < config.columns) {
            for (int i = len; i < config.columns; i++) {
                buffer[i] = ' ';
            }
            buffer[config.columns] = '\0';
        }

        print(buffer);
    }

public:
    /**
     * @brief Конструктор LCD
     * @param cfg Конфигурация дисплея
     */
    explicit LCD2004(const LCD2004Config &cfg)
        : I2CDevice(cfg.i2cAddress, cfg.bus),
          config(cfg),
          lcd(nullptr),
          backlightState(cfg.backlightOnStart),
          cursorState(cfg.cursorOnStart) {
    }

    ~LCD2004() override {
        delete lcd;
    }

    /**
     * @brief Инициализация дисплея
     * @return true если успешно
     */
    bool begin() override {
        if (!I2CDevice::begin()) {
            return false;
        }

        lcd = new LiquidCrystal_I2C(config.i2cAddress, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
        lcd->begin(config.columns, config.rows, LCD_5x8DOTS);

        // Проверяем наличие дисплея на шине I2C
        if (!I2CDevice::isConnected()) {
            delete lcd;
            lcd = nullptr;
            initialized = false;
            return false;
        }

        if (config.backlightOnStart) {
            backlight();
        } else {
            noBacklight();
        }

        if (config.cursorOnStart) {
            cursor();
        } else {
            noCursor();
        }

        lcd->createChar(WIFI_CHAR, const_cast<byte*>(wifiIcon));
        lcd->createChar(RUSSIAN_B_CHAR, const_cast<byte*>(russianB));
        lcd->createChar(RUSSIAN_YA_CHAR, const_cast<byte*>(russianYA));

        initialized = true;
        return true;
    }

    /**
     * @brief Перезагрузка дисплея (destroy + init)
     * @return true если успешно
     */
    bool reboot() {
        Serial.println("LCD: Reboot requested...");

        // Destroy existing object
        if (lcd) {
            Serial.println("LCD: Destroying existing instance...");
            delete lcd;
            lcd = nullptr;
            initialized = false;
        }

        delay(100); // Small delay to ensure cleanup

        // Re-initialize
        Serial.println("LCD: Re-initializing...");
        return begin();
    }

    /**
     * @brief Проверка подключения дисплея
     */
    bool isConnected() override {
        if (!lcd || !initialized) {
            return false;
        }
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

    void line_printf(const char *format, ...) {
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
    void line_printf(uint8_t row, const char *format, ...) {
        va_list args;
        va_start(args, format);
        setCursor(0, row);
        printFormatted(format, args);
        va_end(args);
    }

    size_t print(const String &s) {
        return lcd ? lcd->print(s) : 0;
    }

    size_t print(const char *str) {
        return lcd ? lcd->print(str) : 0;
    }

    size_t println(const String &s) {
        return lcd ? lcd->println(s) : 0;
    }

    size_t println(const char *str) {
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
    void printAt(uint8_t col, uint8_t row, const String &text) {
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
    void createChar(uint8_t slot, const uint8_t *data) {
        if (lcd && slot < 8) {
            lcd->createChar(slot, const_cast<uint8_t *>(data));
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
        info += " on " + bus.getInfo();
        return info;
    }

    /**
     * @brief Получить конфигурацию
     */
    const LCD2004Config &getConfig() const { return config; }
};

// ============================================================================
// Определения пользовательских символов LCD2004 (https://maxpromer.github.io/LCD-Character-Creator/)
// ============================================================================

const byte LCD2004::smiley[8] = {
    B00000,
    B10001,
    B00000,
    B00000,
    B10001,
    B01110,
    B00000,
};

const byte LCD2004::russianB[8] = {
    B11111,
    B10000,
    B10000,
    B11110,
    B10001,
    B10001,
    B11110,
    B00000
};

const byte LCD2004::russianYA[8] = {
    B01111,
    B10001,
    B10001,
    B01001,
    B00111,
    B01001,
    B10001,
    B00000
};

const byte LCD2004::wifiIcon[8] = {
    B00000,
    B00000,
    B01110,
    B10001,
    B00100,
    B01010,
    B00000,
    B00100
};

const byte LCD2004::touchIcon[8] = {
    B00100,
    B01110,
    B11111,
    B00100,
    B00100,
    B00100,
    B00000,
    B00000,
};

const byte LCD2004::arrowRight[8] = {
    B00000,
    B00010,
    B00110,
    B01110,
    B00110,
    B00010,
    B00000,
    B00000,
};
