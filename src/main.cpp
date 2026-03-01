#include <Arduino.h>
#include "hal/HAL.h"
#include "hal/pages/Sensors.h"
#include "hal/pages/Status.h"
#include "LEDController.h"
#include "LEDStrip.h"
#include "color/SaunaColors.h"

// ============================================================================
// Конфигурация оборудования
// ============================================================================

// I2C конфигурация
constexpr uint8_t I2C_SDA_PIN = 21;
constexpr uint8_t I2C_SCL_PIN = 22;

// LCD конфигурация
constexpr uint8_t LCD_I2C_ADDR = 0x27;

// BME280 конфигурация
constexpr uint8_t BME280_I2C_ADDR = 0x76;
constexpr float SEALEVELPRESSURE_HPA = 1013.25f;
constexpr float HPA_TO_MMHG = 0.75006156f;

// DS18B20 конфигурация
constexpr uint8_t DS18B20_PIN = 5;

// RGB LED конфигурация
constexpr uint8_t LED_R_PIN = 25;
constexpr uint8_t LED_G_PIN = 26;
constexpr uint8_t LED_B_PIN = 27;
constexpr uint8_t LEDC_CHANNEL_R = 0;
constexpr uint8_t LEDC_CHANNEL_G = 1;
constexpr uint8_t LEDC_CHANNEL_B = 2;

// WiFi конфигурация (из build_flags в platformio.ini)
#ifndef WIFI_SSID
    #error "WIFI_SSID not defined! Check platformio.ini and wifi.ini"
#endif
#ifndef WIFI_PASSWORD
    #error "WIFI_PASSWORD not defined! Check platformio.ini and wifi.ini"
#endif

constexpr touch_pad_t TOUCH_PIN = TOUCH_PAD_NUM3; // Touch конфигурация (используем T3 = GPIO15)
constexpr float TOUCH_THRESHOLD_PERCENT = 0.8f; // Порог срабатывания тача (% от baseline)
constexpr uint32_t TOUCH_DEBOUNCE_MS = 50;

// ============================================================================
// Глобальные объекты HAL
// ============================================================================

// Конфигурация и создание HAL-объектов
HAL::LCDConfig lcdConfig(LCD_I2C_ADDR, I2C_SDA_PIN, I2C_SCL_PIN, 20, 4, true, false);
HAL::LCD lcd(lcdConfig);

HAL::BME280Config bmeConfig(BME280_I2C_ADDR, I2C_SDA_PIN, I2C_SCL_PIN, SEALEVELPRESSURE_HPA);
HAL::BME280Sensor bme(bmeConfig);

HAL::DS18B20Config dsConfig(DS18B20_PIN, 2, 12, false);
HAL::DS18B20Manager ds18b20(dsConfig);

HAL::RGBLEDConfig ledConfig(LED_R_PIN, LED_G_PIN, LED_B_PIN,
                            LEDC_CHANNEL_R, LEDC_CHANNEL_G, LEDC_CHANNEL_B,
                            1000, 8, 2.2f, true);
HAL::RGBLED ledStrip(ledConfig);

// WiFi и веб-сервер
HAL::WiFiConfig wifiConfig(WIFI_SSID, WIFI_PASSWORD, 15000, true);
HAL::WiFiManager wifi(wifiConfig);
HAL::SaunaWebServer webServer;

// Touch сенсор
HAL::TouchConfig touchConfig(TOUCH_PIN, TOUCH_THRESHOLD_PERCENT, TOUCH_DEBOUNCE_MS);
HAL::TouchSensor touch(touchConfig);

// Менеджер страниц
HAL::PageManager pageMgr;

// Страницы (будут созданы в setup)
HAL::DallasSensorsPage *dallasPage = nullptr;
HAL::BME280Page *bmePage = nullptr;
HAL::DisplayPage *wifiPage = nullptr;
HAL::SystemStatusPage *statusPage = nullptr;

// Sauna-specific LED strip (обертка над HAL::RGBLED)
LEDStrip *pLedStrip = nullptr;
LEDStripController *pLedController = nullptr;

// ============================================================================
// Режимы работы
// ============================================================================

enum SaunaMode {
    MODE_AUTO,
    MODE_TEMPERATURE,
    MODE_HUMIDITY,
    MODE_COMFORT,
    MODE_SAFETY,
    MODE_RELAX,
    MODE_MANUAL
};

SaunaMode currentMode = MODE_AUTO;

// ============================================================================
// Функции LCD
// ============================================================================

void displayWelcome() {
    lcd.setCursor(8, 1);
    lcd.write(RUSSIAN_B_CHAR);
    lcd.print("AH");
    lcd.write(RUSSIAN_YA_CHAR);
    lcd.setCursor(8, 2);
    lcd.print("v1.0");
    delay(5000);
    lcd.clear();
    delay(500);
}

// ============================================================================
// Чтение сенсоров
// ============================================================================

void readDS18B20Temperatures() {
    ds18b20.requestTemperatures();
    while (!ds18b20.isConversionComplete()) {
        yield();
    }
    ds18b20.updateTemperatures();
}

// ============================================================================
// Провайдер статуса для веб-сервера
// ============================================================================

HAL::SaunaStatus getSaunaStatus() {
    HAL::SaunaStatus status;

    // Температуры
    status.temp1 = ds18b20.getTemperature(0);
    status.temp2 = ds18b20.getTemperature(1);
    status.temp3 = bme.getTemperature();
    status.humidity = bme.getHumidity();
    status.pressure = bme.getPressure_mmHg(HPA_TO_MMHG);

    // Статус сенсоров
    status.sensor1Connected = ds18b20.isConnected(0) && (status.temp1 != DEVICE_DISCONNECTED_C);
    status.sensor2Connected = ds18b20.isConnected(1) && (status.temp2 != DEVICE_DISCONNECTED_C);

    // WiFi
    status.wifiIP = wifi.getIPAddressString();
    status.wifiStatus = wifi.getStatusString();

    // Режим
    const char *modeNames[] = {"Auto", "Temp", "Humidity", "Comfort", "Safety", "Relax", "Manual"};
    status.mode = modeNames[currentMode];

    return status;
}

// ============================================================================
// Обработка серийных команд
// ============================================================================

void handleSerialCommands() {
    while (Serial.available()) {
        char cmd = Serial.read();
        cmd = toupper(cmd);

        switch (cmd) {
            case 'A':
                currentMode = MODE_AUTO;
                Serial.println("Mode: Auto");
                if (statusPage) statusPage->setMode("Auto");
                break;
            case 'T':
                currentMode = MODE_TEMPERATURE;
                Serial.println("Mode: Temperature");
                if (statusPage) statusPage->setMode("Temp");
                break;
            case 'H':
                currentMode = MODE_HUMIDITY;
                Serial.println("Mode: Humidity");
                if (statusPage) statusPage->setMode("Humidity");
                break;
            case 'C':
                currentMode = MODE_COMFORT;
                Serial.println("Mode: Comfort");
                if (statusPage) statusPage->setMode("Comfort");
                break;
            case 'S':
                currentMode = MODE_SAFETY;
                Serial.println("Mode: Safety");
                if (statusPage) statusPage->setMode("Safety");
                break;
            case 'R':
                currentMode = MODE_RELAX;
                Serial.println("Mode: Relax");
                if (statusPage) statusPage->setMode("Relax");
                break;
            case 'M':
                currentMode = MODE_MANUAL;
                Serial.println("Mode: Manual");
                if (statusPage) statusPage->setMode("Manual");
                break;

            // Управление яркостью (0-9)
            case '0': ledStrip.setBrightness(0.005);
                break;
            case '1': ledStrip.setBrightness(0.1);
                break;
            case '2': ledStrip.setBrightness(0.2);
                break;
            case '3': ledStrip.setBrightness(0.3);
                break;
            case '4': ledStrip.setBrightness(0.4);
                break;
            case '5': ledStrip.setBrightness(0.5);
                break;
            case '6': ledStrip.setBrightness(0.6);
                break;
            case '7': ledStrip.setBrightness(0.7);
                break;
            case '8': ledStrip.setBrightness(0.8);
                break;
            case '9': ledStrip.setBrightness(0.9);
                break;

            // Ручные цвета
            case 'W':
                ledStrip.setColor(RGB(255, 255, 255));
                Serial.println("Color: White");
                break;
            case 'E': // Red
                ledStrip.setColor(RGB(255, 0, 0));
                Serial.println("Color: Red");
                break;
            case 'F': // Green
                ledStrip.setColor(RGB(0, 255, 0));
                Serial.println("Color: Green");
                break;
            case 'V': // Blue
                ledStrip.setColor(RGB(0, 0, 255));
                Serial.println("Color: Blue");
                break;

            // Эффекты
            case 'P':
                pLedController->startPulse(SaunaColors::fire(), 3000);
                Serial.println("Effect: Pulse");
                break;
            case 'Q':
                pLedController->startRainbow(3000);
                Serial.println("Effect: Rainbow");
                break;
            case 'B':
                pLedController->startBlink(SaunaColors::ice(), 200, 200, 10);
                Serial.println("Effect: Blink");
                break;
            case 'X':
                pLedController->stopEffect();
                Serial.println("Effect: Stopped");
                break;
            case 'O':
                pLedController->off();
                Serial.println("Off");
                break;

            // Информация
            case 'I':
                Serial.println(ledStrip.getInfo());
                Serial.printf("Mode: %d, Brightness: %.0f%%\n",
                              currentMode, ledStrip.getBrightness() * 100);
                break;

            // HAL диагностика
            case 'D':
                Serial.println("=== HAL Diagnostics ===");
                Serial.println(lcd.getInfo());
                Serial.println(bme.getInfo());
                Serial.println(ds18b20.getInfo());
                Serial.println(ledStrip.getInfo());
                Serial.println(wifi.getInfo());
                Serial.print("Touch: Value=");
                Serial.print(touch.read());
                Serial.print(", Baseline=");
                Serial.print(touch.getBaselineValue());
                Serial.print(", Threshold=");
                Serial.println(touch.getThreshold());
                Serial.println(touch.getInfo());
                Serial.println(pageMgr.getInfo());
                break;

            // WiFi переподключение
            case 'L': // Link
                Serial.println("WiFi: Reconnecting...");
                wifi.reconnect();
                break;

            // Переключение страниц
            case '>':
            case '.':
            case 'N': // Next page (не конфликтует с другими командами)
                Serial.println("Page: Next");
                pageMgr.nextPage();
                break;
            case '<':
            case ',':
                Serial.println("Page: Prev");
                pageMgr.prevPage();
                break;
        }
    }
}

// ============================================================================
// Setup
// ============================================================================

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== Sauna Controller HAL ===");

    // Инициализация LCD
    Serial.print("Initializing LCD... ");
    if (lcd.begin()) {
        Serial.println("OK");
        displayWelcome();
    } else {
        Serial.println("FAILED");
    }

    // Инициализация BME280
    Serial.print("Initializing BME280... ");
    if (bme.begin()) {
        Serial.println("OK");
    } else {
        Serial.println("FAILED - check wiring!");
    }

    // Инициализация DS18B20
    Serial.print("Initializing DS18B20... ");
    if (ds18b20.begin()) {
        Serial.print("Found ");
        Serial.print(ds18b20.getSensorCount());
        Serial.println(" sensors");
    } else {
        Serial.println("FAILED - no sensors found");
    }

    // Инициализация RGB LED
    Serial.print("Initializing RGB LED... ");
    if (ledStrip.begin()) {
        Serial.println("OK");
        ledStrip.setBrightness(0.5);
        ledStrip.enableGamma(true);
    } else {
        Serial.println("FAILED");
    }

    // Создание объектов управления LED
    LEDStrip adapter(LED_R_PIN, LED_G_PIN, LED_B_PIN,
                     LEDC_CHANNEL_R, LEDC_CHANNEL_G, LEDC_CHANNEL_B,
                     1000, 8);
    adapter.begin();
    pLedStrip = &adapter;

    LEDStripController ledController(pLedStrip);
    pLedController = &ledController;

    // Инициализация WiFi
    Serial.print("Initializing WiFi... ");
    if (wifi.begin()) {
        Serial.println("OK");
    } else {
        Serial.println("FAILED");
    }

    // Подключение к WiFi
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.write(1); // WiFi иконка
    lcd.print(" Connecting...");
    lcd.setCursor(0, 2);
    lcd.print(WIFI_SSID);

    if (wifi.connect()) {
        // Запуск веб-сервера
        webServer.begin();
        webServer.setStatusProvider(getSaunaStatus);
        webServer.start();

        String ip = wifi.getIPAddressString();
        Serial.println(ip);
        Serial.println("WebServer: Started on http://" + ip);
    } else {
        Serial.println("WiFi: Failed to connect");
        lcd.setCursor(0, 3);
        lcd.print("Failed!");
        // Не делаем delay - страницы отрисуются в loop()
    }

    // Инициализация Touch сенсора
    Serial.print("Initializing Touch... ");
    if (touch.begin()) {
        Serial.println("OK");
        Serial.print("Touch: Baseline = ");
        Serial.println(touch.getBaselineValue());
        Serial.print("Touch: Threshold = ");
        Serial.println(touch.getThreshold());
    } else {
        Serial.println("FAILED");
    }

    // Создание страниц
    Serial.println("Creating display pages...");

    dallasPage = new HAL::DallasSensorsPage(&ds18b20, "Dallas DS18B20");
    bmePage = new HAL::BME280Page(&bme, "BME280 Sensor");
    wifiPage = new HAL::WiFiInfoPage(&wifi, "WiFi Info");
    statusPage = new HAL::SystemStatusPage(&wifi, "System Status");

    // Добавление страниц в менеджер
    pageMgr.addPage(std::unique_ptr<HAL::DisplayPage>(dallasPage));
    pageMgr.addPage(std::unique_ptr<HAL::DisplayPage>(bmePage));
    pageMgr.addPage(std::unique_ptr<HAL::DisplayPage>(wifiPage));
    pageMgr.addPage(std::unique_ptr<HAL::DisplayPage>(statusPage));

    pageMgr.begin(&lcd);

    Serial.print("Pages created: ");
    Serial.println(pageMgr.getPageCount());

    // Установка начальной страницы
    pageMgr.goToPage(0);
    pageMgr.render();

    // Вывод информации о режимах
    Serial.println("\n=== Commands ===");
    Serial.println("Modes: A=Auto, T=Temp, H=Humidity, C=Comfort, S=Safety, R=Relax, M=Manual");
    Serial.println("Manual: 0-9=Brightness, R/G/B/W=Colors, +=Warmer, -=Cooler");
    Serial.println("Effects: P=Pulse, Q=Rainbow, B=Blink, X=Stop, O=Off");
    Serial.println("Pages: > or . = Next, < or , = Prev, Touch = Next");
    Serial.println("Diagnostics: D=HAL Info, I=Status, L=WiFi Reconnect");
    Serial.println("\nWeb Interface: http://" + wifi.getIPAddressString());
    Serial.println("Touch sensor: " + touch.getPinName() + " (for page switching)");
}

// ============================================================================
// Loop
// ============================================================================

void loop() {
    handleSerialCommands();

    if (pLedController) {
        pLedController->update();
    }

    // Обработка веб-клиентов
    webServer.handleClient();

    // Обработка Touch сенсора
    if (touch.isNewTouch()) {
        Serial.println("Touch detected!");
        pageMgr.nextPage();
    }

    pageMgr.render();

    // Авто-переключение страниц (если включено)
    pageMgr.updateAutoSwitch();
}
