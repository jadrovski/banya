#include <Arduino.h>

#include "IntervalTimer.h"
#include "hal/HAL.h"
#include "ota/OTA.h"
#include "ota/OTAPresenter.h"
#include "adapter/LCDOTAPresenter.h"
#include "pages/page/Sensors.h"
#include "pages/page/SystemStatusPage.h"
#include "pages/page/LEDStripPage.h"
#include "pages/page/WiFiSetupPage.h"
#include "color/BanyaColors.h"
#include "pages/PageManager.h"
#include "web/WebServer.h"

// ============================================================================
// Конфигурация оборудования
// ============================================================================

// I2C конфигурация
constexpr uint8_t I2C_SDA_PIN = 21;
constexpr uint8_t I2C_SCL_PIN = 22;

// LCD конфигурация
constexpr uint8_t LCD_I2C_ADDR = 0x27;
constexpr uint8_t LCD_COLUMNS = 20;
constexpr uint8_t LCD_ROWS = 4;

// BME280 конфигурация
constexpr uint8_t BME280_I2C_ADDR = 0x76;
constexpr float SEA_LEVEL_PRESSURE_HPA = 1013.25f;
constexpr float HPA_TO_MMHG = 0.75006156f;

// DS18B20 конфигурация
constexpr uint8_t DS18B20_PIN = 5;
constexpr unsigned long DS18B20_UPDATE_INTERVAL = 2000; // 2 секунды

// RGB LED конфигурация
constexpr uint8_t LED_R_PIN = 25;
constexpr uint8_t LED_G_PIN = 26;
constexpr uint8_t LED_B_PIN = 27;
constexpr uint8_t LEDC_CHANNEL_R = 0;
constexpr uint8_t LEDC_CHANNEL_G = 1;
constexpr uint8_t LEDC_CHANNEL_B = 2;

constexpr touch_pad_t TOUCH_PIN = TOUCH_PAD_NUM3; // Touch конфигурация (используем T3 = GPIO15)
constexpr float TOUCH_THRESHOLD_PERCENT = 0.8f; // Порог срабатывания тача (% от baseline)
constexpr uint32_t TOUCH_DEBOUNCE_MS = 50;
constexpr uint32_t TOUCH_LONG_PRESS_MS = 1000;
constexpr uint32_t TOUCH_VERY_LONG_PRESS_MS = 5000;

// ============================================================================
// Глобальные объекты HAL
// ============================================================================

// I2C шина (одна для всех устройств на шине)
I2CBusConfig i2cBusConfig(&Wire, I2C_SDA_PIN, I2C_SCL_PIN, 100000);
I2CBus mainBus(i2cBusConfig);

// Конфигурация и создание HAL-объектов
LCD2004Config lcdConfig(
    LCD_I2C_ADDR,
    mainBus,
    LCD_COLUMNS,
    LCD_ROWS,
    true,
    false
);
LCD2004 lcd(lcdConfig);

BME280Config bmeConfig(BME280_I2C_ADDR, mainBus, SEA_LEVEL_PRESSURE_HPA);
BME280Sensor bme(bmeConfig);

DS18B20Config dsConfig(DS18B20_PIN, 12, false, DS18B20_UPDATE_INTERVAL);
DS18B20Manager ds18b20(dsConfig);

RGBLEDConfig ledConfig(LED_R_PIN, LED_G_PIN, LED_B_PIN,
                            LEDC_CHANNEL_R, LEDC_CHANNEL_G, LEDC_CHANNEL_B,
                            1000, 8, 2.2f, true);
RGBLED ledStrip(ledConfig);

// WiFi и веб-сервер
WiFiSettings wifiSettings;
WiFiConfig wifiConfig("", "", 15000, true); // Credentials будут загружены из NVS
WiFiManager wifi(wifiConfig);
BanyaWebServer webServer;

// OTA конфигурация
OTAConfig otaConfig;
OTAManager ota(otaConfig);
LCDOTAPresenter otaPresenter(&lcd);

// Touch сенсор
TouchConfig touchConfig(
    TOUCH_PIN,
    TOUCH_THRESHOLD_PERCENT,
    TOUCH_DEBOUNCE_MS,
    TOUCH_LONG_PRESS_MS,
    TOUCH_VERY_LONG_PRESS_MS
);
TouchSensor touch(touchConfig);

// Менеджер страниц
PageManager pageMgr;

// Страницы (будут созданы в setup)
DallasSensorsPage *dallasPage = nullptr;
BME280Page *bmePage = nullptr;
DisplayPage *wifiPage = nullptr;
SystemStatusPage *statusPage = nullptr;
LEDStripPage *ledStripPage = nullptr;
WiFiSetupPage *wifiSetupPage = nullptr;

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
    delay(1000);
    lcd.clear();
    delay(500);
}

// ============================================================================
// Провайдер статуса для веб-сервера
// ============================================================================

BanyaStatus getBanyaStatus() {
    BanyaStatus status;

    // Температуры (DS18B20 обновляется автоматически в loop())
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

    // OTA
    status.otaStatus = ota.getStatusString();
    status.otaProgress = ota.getProgress();

    return status;
}

// ============================================================================
// Обработка событий Touch сенсора
// ============================================================================

/**
 * @brief Callback для обработки событий Touch сенсора
 * - Long press: WiFi reconnect
 * - Very-long press: Enter WiFi Setup Mode (AP mode)
 */
void handleTouchCallback(TouchEvent event) {
    switch (event) {
        case TouchEvent::LONG_PRESS:
            wifi.reconnect();
            break;

        case TouchEvent::VERY_LONG_PRESS:
            // Enter WiFi Setup Mode
            Serial.println("Touch: Very-long press - Entering WiFi Setup Mode...");

            if (!wifi.isAPEnabled()) {
                wifi.enableAP();
            } else {
                wifi.disableAP();
            }

            // Переходим на страницу настройки WiFi
            if (wifiSetupPage) {
                pageMgr.goToPage(4);
                pageMgr.render();
            }
            break;

        case TouchEvent::TAP:
            pageMgr.nextPage();
            break;

        default:
            break;
    }
}

// ============================================================================
// Обработка серийных команд
// ============================================================================

void handleSerialCommands() {
    while (Serial.available()) {
        char cmd = Serial.read();
        cmd = toupper(cmd);

        switch (cmd) {
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
            case 'U': // Blue (U like blUe)
                ledStrip.setColor(RGB(0, 0, 255));
                Serial.println("Color: Blue");
                break;

            // Информация
            case 'I':
                Serial.println(ledStrip.getInfo());
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
    Serial.println("\n=== Banya Controller HAL ===");

    // Инициализация I2C шины (перед устройствами!)
    mainBus.begin();

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

    // Инициализация WiFi Settings (NVS)
    Serial.print("Initializing WiFi Settings (NVS)... ");
    if (wifiSettings.begin()) {
        Serial.println("OK");

        // Загружаем credentials из NVS
        if (wifiSettings.isConfigured()) {
            WiFiCredentials creds = wifiSettings.getCredentials();
            wifiConfig.ssid = strdup(creds.ssid.c_str());
            wifiConfig.password = strdup(creds.password.c_str());
            wifiConfig.autoReconnect = true;
            Serial.print("WiFi Settings: Loaded from NVS - SSID: ");
            Serial.println(creds.ssid);
        } else {
            Serial.println("WiFi Settings: No credentials in NVS - will use AP mode for setup");
        }
    } else {
        Serial.println("FAILED");
        Serial.println("WiFi Settings: Will use AP mode for setup");
    }

    // Инициализация WiFi
    if (wifi.begin()) {
        Serial.println("OK");
    } else {
        Serial.println("FAILED");
    }

    // Update WiFi credentials before connecting
    wifi.setCredentials(wifiConfig.ssid, wifiConfig.password);

    // Подключение к WiFi
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.write(1); // WiFi иконка
    lcd.print(" Connecting...");
    lcd.setCursor(0, 2);
    lcd.print(wifiConfig.ssid);

    bool wifiConnected = wifi.connect();

    // Инициализация OTA (только если WiFi подключён)
    if (wifiConnected) {
        // Configure OTA
        otaConfig.hostname = "banya-controller";
        otaConfig.port = 3232;
        otaConfig.enableProgress = true;
        otaConfig.enableDebug = false;

        Serial.print("Initializing OTA... ");
        if (ota.begin()) {
            Serial.println("OK");
            Serial.print("OTA: Hostname: ");
            Serial.println(otaConfig.hostname);
            Serial.print("OTA: Port: ");
            Serial.println(otaConfig.port);

            // Initialize and set OTA presenter
            otaPresenter.begin();
            ota.setPresenter(&otaPresenter);
        } else {
            Serial.println("FAILED");
        }
    }

    // Запуск веб-сервера (всегда, даже если WiFi не подключён)
    webServer.begin();
    webServer.setStatusProvider(getBanyaStatus);
    webServer.setLEDStrip(&ledStrip);
    webServer.setWiFiManager(&wifi);
    webServer.setWiFiSettings(&wifiSettings);
    webServer.setLCD(&lcd);
    webServer.setOTA(&ota);
    webServer.start();

    if (wifiConnected) {
        String ip = wifi.getIPAddressString();
        Serial.println(ip);
        Serial.println("WebServer: Started on http://" + ip);
    } else {
        Serial.println("WiFi: Failed to connect");
        lcd.setCursor(0, 3);
        lcd.print("Failed!");
        // Включаем AP если не удалось подключиться и есть настройки
        if (wifiSettings.isConfigured()) {
            Serial.println("WiFi: No connection, will enter AP mode on request");
        }
    }

    // Инициализация Touch сенсора
    Serial.print("Initializing Touch... ");
    if (touch.begin()) {
        Serial.println("OK");
        Serial.print("Touch: Baseline = ");
        Serial.println(touch.getBaselineValue());
        Serial.print("Touch: Threshold = ");
        Serial.println(touch.getThreshold());

        // Установка callback для обработки long press событий
        touch.setCallback(handleTouchCallback);
    } else {
        Serial.println("FAILED");
    }

    // Создание страниц
    Serial.println("Creating display pages...");

    dallasPage = new DallasSensorsPage(&ds18b20, "Dallas DS18B20");
    bmePage = new BME280Page(&bme, "BME280 Sensor");
    wifiPage = new WiFiInfoPage(&wifi, "WiFi Info");
    wifiSetupPage = new WiFiSetupPage(&wifi, &wifiSettings, "WiFi Setup");
    statusPage = new SystemStatusPage(&wifi, "System Status");
    ledStripPage = new LEDStripPage(&ledStrip, "LED Strip");

    // Добавление страниц в менеджер
    pageMgr.addPage(std::unique_ptr<DisplayPage>(dallasPage));
    pageMgr.addPage(std::unique_ptr<DisplayPage>(bmePage));
    pageMgr.addPage(std::unique_ptr<DisplayPage>(ledStripPage));
    pageMgr.addPage(std::unique_ptr<DisplayPage>(wifiPage));
    pageMgr.addPage(std::unique_ptr<DisplayPage>(wifiSetupPage));
    pageMgr.addPage(std::unique_ptr<DisplayPage>(statusPage));

    pageMgr.begin(&lcd);

    Serial.print("Pages created: ");
    Serial.println(pageMgr.getPageCount());

    // Установка начальной страницы
    pageMgr.goToPage(0);
    pageMgr.render();

    // Вывод информации о режимах
    Serial.println("\n=== Commands ===");
    Serial.println("Manual: 0-9=Brightness, R/G/B/W=Colors, +=Warmer, -=Cooler");
    Serial.println("Effects: P=Pulse, Q=Rainbow, B=Blink, X=Stop, O=Off");
    Serial.println("Pages: > or . = Next, < or , = Prev, Touch Tap = Next");
    Serial.println("Touch: Long press = WiFi reconnect, Very-long press = WiFi Setup");
    Serial.println("WiFi Setup: V=Enter Setup Mode (very-long press)");
    Serial.println("Diagnostics: D=HAL Info, I=Status, L=WiFi Reconnect");
    Serial.println("\nWeb Interface: http://" + wifi.getIPAddressString());
    Serial.println("Touch sensor: " + touch.getPinName() + " (for page switching)");
    Serial.println("WiFi Settings: " + wifiSettings.getInfo());
}

// ============================================================================
// Loop
// ============================================================================

IntervalTimer lcdTimer(5000); // 5 секунд

void loop() {
    handleSerialCommands();

    // Автоматическое обновление температур DS18B20
    ds18b20.handleLoop();

    // Обработка веб-клиентов
    webServer.handleLoop();

    // Обработка процесса WiFi переподключения (неблокирующее)
    wifi.handleLoop();

    // Обработка OTA обновлений
    ota.handleLoop();

    // Обработка Touch событий (tap, long press, very-long press)
    touch.handleLoop();

    pageMgr.handleLoop();

    // Проверка подключения LCD (периодически)
    lcdTimer.handleLoop([] {
        if (!lcd.isConnected()) {
            Serial.println("LCD: Reconnecting...");
            lcd.begin();
        }
    });
}
