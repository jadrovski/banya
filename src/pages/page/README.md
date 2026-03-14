# Система страниц дисплея

## Обзор

Система страниц позволяет выводить различную информацию на LCD дисплей, переключаясь между страницами с помощью:
- **Touch сенсора** (GPIO33 / T8)
- **Serial команд** (`>`, `.`, `<`, `,`)

## Архитектура

```
src/hal/
├── DisplayPage.h        # Базовый класс DisplayPage
├── PageManager.h        # PageManager и drawPageIndicator()
├── DisplayPages.h       # Convenience header (включает DisplayPage.h + PageManager.h)
├── Touch.h              # Touch сенсор ESP32
└── pages/
    ├── DallasSensorsPage.h  # Страница DS18B20
    ├── BME280Page.h         # Страница BME280
    ├── WiFiInfoPage.h       # Страница WiFi Info
    ├── Status.h             # Страница System Status
    └── Sensors.h            # Convenience header (включает все страницы сенсоров)
```

## Страницы

| № | Страница | Описание |
|---|----------|----------|
| 1 | Dallas Sensors | Температуры DS18B20 (T1, T2) |
| 2 | BME280 Sensor | Температура, влажность, давление, высота |
| 3 | WiFi Settings | SSID, статус, IP адрес, RSSI |
| 4 | System Status | Режим, WiFi, uptime |
| 5 | Help Commands | Список команд |

## Переключение страниц

### Touch сенсор
- **Касание** → следующая страница
- Подключен к **GPIO33 (T8)**
- Порог срабатывания: 50 (настраивается)

### Serial команды
```
> или .  → Следующая страница
< или ,  → Предыдущая страница
```

### Индикатор
Внизу экрана отображается:
```
[##---] 2/5 >Next
```
- `#` - текущая страница
- `-` - другие страницы
- `2/5` - номер текущей / всего страниц

## Добавление новой страницы

### Шаг 1: Создать класс страницы

```cpp
#include "../DisplayPage.h"

namespace HAL {

class MyCustomPage : public DisplayPage {
private:
    // Данные страницы
    
public:
    MyCustomPage(const String& title = "My Page")
        : DisplayPage(title, 3) {} // 3 = индекс
    
    void onEnter() override {
        DisplayPage::onEnter();
        // Инициализация при входе
    }
    
    void render(LCD& lcd, bool force = false) override {
        if (!force) return;
        
        lcd.setCursor(0, 0);
        lcd.print(title);
        
        // Отрисовка контента
        lcd.setCursor(0, 1);
        lcd.print("Hello World!");
    }
};

} // namespace HAL
```

### Шаг 2: Добавить страницу в main.cpp

```cpp
// Объявление
HAL::MyCustomPage* myPage = nullptr;

// В setup() - создание
myPage = new HAL::MyCustomPage("My Custom");
pageMgr.addPage(std::unique_ptr<HAL::DisplayPage>(myPage));

// Или напрямую:
pageMgr.addPage(std::make_unique<HAL::MyCustomPage>("My Custom"));
```

## Настройка Touch сенсора

### Изменение пина
```cpp
// В main.cpp
constexpr touch_pad_t TOUCH_PIN = TOUCH_PAD_NUM9; // T9 = GPIO32
```

### Изменение порога
```cpp
constexpr uint16_t TOUCH_THRESHOLD = 30; // Меньше = чувствительнее
```

### Доступные пины
| Пин | GPIO | Примечание |
|-----|------|------------|
| T0 | GPIO4 | ✓ Рекомендуется |
| T1 | GPIO0 | ✗ Boot стреп |
| T2 | GPIO2 | ✗ Встроенный LED |
| T3 | GPIO15 | ✓ |
| T4 | GPIO13 | ✓ |
| T5 | GPIO12 | ✓ |
| T6 | GPIO14 | ✓ |
| T7 | GPIO27 | ✓ |
| T8 | GPIO33 | ✓ (по умолчанию) |
| T9 | GPIO32 | ✓ |

## Калибровка Touch

При старте выполняется автоматическая калибровка:
```
Touch: Initialized on pin T8 (GPIO33)
Touch: Baseline = 150
Touch: Threshold = 50
```

Если касания не работают:
1. Уменьшите порог (например, до 30)
2. Проверьте подключение провода к GPIO33
3. Увеличьте площадь касания

## Авто-переключение страниц

```cpp
// Включить авто-переключение каждые 10 секунд
pageMgr.setAutoSwitch(10000);

// В loop() уже вызывается:
pageMgr.updateAutoSwitch();
```

## Примеры использования

### Получение текущей страницы
```cpp
uint8_t idx = pageMgr.getCurrentPageIndex();
HAL::DisplayPage* page = pageMgr.getCurrentPage();
String title = page->getTitle();
```

### Программное переключение
```cpp
pageMgr.nextPage();      // Следующая
pageMgr.prevPage();      // Предыдущая
pageMgr.goToPage(2);     // Конкретная (0-based)
```

### Информация о системе страниц
```cpp
Serial.println(pageMgr.getInfo());
// Вывод: "Pages: 5 | Current: 2 (WiFi Settings)"
```

## Статистика сборки

- **RAM:** 46.3 KB / 320 KB (14%)
- **Flash:** 872 KB / 1.3 MB (66%)

## Команды Serial

| Команда | Описание |
|---------|----------|
| `>` или `.` | Следующая страница |
| `<` или `,` | Предыдущая страница |
| `D` | Диагностика (включая статус страниц) |
| `L` | WiFi переподключение |
