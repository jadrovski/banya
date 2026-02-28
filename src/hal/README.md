# Hardware Access Layer (HAL) для Sauna Controller

## Обзор

Hardware Access Layer (HAL) предоставляет абстрактный, конфигурируемый интерфейс для работы с оборудованием в проекте Sauna Controller. Все классы находятся в пространстве имён `HAL`.

## Структура

```
src/hal/
├── HAL.h              # Главный заголовочный файл (импортирует всё)
├── I2CDevice.h        # Базовый класс для I2C устройств
├── LCD.h              # LCD 2004 I2C дисплей
├── BME280.h           # Сенсор температуры/влажности/давления
├── DS18B20.h          # Менеджер OneWire температурных сенсоров
└── RGBLED.h           # RGB LED через PWM (LEDC)
```

## Быстрый старт

```cpp
#include "hal/HAL.h"

// Создание объектов с конфигурацией по умолчанию
HAL::LCD lcd;
HAL::BME280Sensor bme;
HAL::DS18B20Manager ds18b20;
HAL::RGBLED led;

void setup() {
    // Инициализация
    lcd.begin();
    bme.begin();
    ds18b20.begin();
    led.begin();
}

void loop() {
    // Чтение сенсоров
    float temp = bme.getTemperature();
    float humidity = bme.getHumidity();
    
    // Отображение
    lcd.printAt(0, 0, "Temp: " + String(temp));
    
    // Управление LED
    led.setColor(RGB(255, 0, 0));
}
```

## Конфигурация

### LCD

```cpp
HAL::LCDConfig lcdConfig(
    0x27,   // I2C адрес
    21,     // SDA пин
    22,     // SCL пин
    20,     // Колонок
    4,      // Строк
    true,   // Подсветка включена
    false   // Курсор выключен
);
HAL::LCD lcd(lcdConfig);
```

### BME280

```cpp
HAL::BME280Config bmeConfig(
    0x76,       // I2C адрес
    21,         // SDA пин
    22,         // SCL пин
    1013.25f    // Давление на уровне моря (гПа)
);
HAL::BME280Sensor bme(bmeConfig);
```

### DS18B20

```cpp
HAL::DS18B20Config dsConfig(
    5,      // GPIO пин данных
    2,      // Максимум сенсоров
    12,     // Разрешение (9-12 бит)
    false   // Асинхронное чтение (не ждать конвертацию)
);
HAL::DS18B20Manager ds18b20(dsConfig);
```

### RGB LED

```cpp
HAL::RGBLEDConfig ledConfig(
    25,         // Red пин
    26,         // Green пин
    27,         // Blue пин
    0, 1, 2,    // PWM каналы
    1000,       // PWM частота (Гц)
    8,          // PWM разрешение (бит)
    2.2f,       // Гамма-коррекция
    true        // Включить гамму
);
HAL::RGBLED led(ledConfig);
```

## API

### I2CDevice (базовый класс)

| Метод | Описание |
|-------|----------|
| `begin()` | Инициализация I2C |
| `isConnected()` | Проверка наличия устройства |
| `getAddress()` | Получить I2C адрес |
| `getInfo()` | Информация об устройстве |

### LCD

| Метод | Описание |
|-------|----------|
| `begin()` | Инициализация дисплея |
| `clear()` | Очистка экрана |
| `setCursor(col, row)` | Установка курсора |
| `print(text)` | Вывод текста |
| `printAt(col, row, text)` | Вывод с позиционированием |
| `clearLine(line)` | Очистка строки |
| `backlight()` / `noBacklight()` | Подсветка |
| `cursor()` / `noCursor()` | Курсор |
| `createChar(slot, data)` | Пользовательский символ |

### BME280Sensor

| Метод | Описание |
|-------|----------|
| `begin()` | Инициализация сенсора |
| `readData()` | Чтение всех данных |
| `getTemperature()` | Температура (°C) |
| `getHumidity()` | Влажность (%) |
| `getPressure_hPa()` | Давление (гПа) |
| `getPressure_mmHg()` | Давление (мм рт.ст.) |
| `getAltitude()` | Высота (м) |
| `isValid()` | Проверка валидности данных |

### DS18B20Manager

| Метод | Описание |
|-------|----------|
| `begin()` | Инициализация менеджера |
| `getSensorCount()` | Количество сенсоров |
| `requestTemperatures()` | Запрос температур (неблокирующий) |
| `isConversionComplete()` | Проверка готовности |
| `updateTemperatures()` | Обновление данных |
| `getTemperature(index)` | Температура по индексу |
| `getAddress(index, addr)` | Получить адрес сенсора |
| `isConnected(index)` | Проверка подключения |
| `setResolution(index, bits)` | Установка разрешения (9-12) |

### RGBLED

| Метод | Описание |
|-------|----------|
| `begin()` | Инициализация PWM |
| `setColor(RGB/HSV/HSL)` | Установка цвета |
| `setBrightness(value)` | Яркость (0.0-1.0) |
| `dim(factor)` | Уменьшение яркости |
| `on()` / `off()` | Включить/выключить |
| `fadeTo(color, duration)` | Плавный переход |
| `blink(color, on, off, n)` | Мигание |
| `pulse(color, period)` | Пульсация |
| `enableGamma(enable)` | Гамма-коррекция |

## Диагностика

В main.cpp добавлена команда `D` для вывода информации о всех устройствах:

```
> D
=== HAL Diagnostics ===
LCD 20x4 @ 0x27 (SDA:21, SCL:22)
BME280 @ 0x76 (SDA:21, SCL:22) T:25.3°C H:45% P:1013hPa
DS18B20 Manager (Pin:5) Sensors:2/2 [0:25.5°C] [1:26.1°C]
RGB LED (R:25,G:26,B:27) Ch:0,1,2 Freq:1000Hz Res:8-bit Bright:50%
```

## Преимущества HAL

1. **Конфигурируемость** - все параметры задаются в одном месте
2. **Инкапсуляция** - детали работы с железом скрыты
3. **Тестируемость** - легко мокать для юнит-тестов
4. **Расширяемость** - просто добавить новые устройства
5. **Читаемость** - понятный и документированный API

## Примеры использования

### Чтение всех сенсоров

```cpp
void readAllSensors() {
    // Запрос DS18B20
    ds18b20.requestTemperatures();
    
    // Ожидание готовности
    while (!ds18b20.isConversionComplete()) {
        yield();
    }
    ds18b20.updateTemperatures();
    
    // Чтение BME280
    bme.readData();
    
    // Использование данных
    float t1 = ds18b20.getTemperature(0);
    float t2 = ds18b20.getTemperature(1);
    float t3 = bme.getTemperature();
    float humidity = bme.getHumidity();
}
```

### Управление LED с эффектами

```cpp
void ledEffect() {
    // Плавное включение красного
    led.fadeTo(RGB(255, 0, 0), 1000);
    delay(500);
    
    // Пульсация
    for (int i = 0; i < 5; i++) {
        led.pulse(RGB(0, 255, 0), 500);
        delay(500);
    }
    
    // Мигание синим
    led.blink(RGB(0, 0, 255), 200, 200, 3);
    
    // Выключение
    led.off();
}
```

### Отображение на LCD

```cpp
void displayStatus() {
    lcd.clear();
    lcd.printAt(0, 0, "Sauna Controller");
    lcd.printAt(0, 1, "Temp: " + String(bme.getTemperature(), 1) + "C");
    lcd.printAt(0, 2, "Humidity: " + String(bme.getHumidity()) + "%");
    lcd.printAt(0, 3, "Pressure: " + String(bme.getPressure_mmHg()) + "mmHg");
}
```
