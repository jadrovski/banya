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
#include "pages/PageManager.h"
#include "web/WebServer.h"
#include "hal/Button.h"
#include "color/TemperatureRangeColor.h"

// I2C конфигурация
constexpr uint8_t I2C_SDA_PIN = 21;
constexpr uint8_t I2C_SCL_PIN = 22;
constexpr uint32_t I2C_SPEED = 100000;

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
constexpr uint8_t LED_PWM_CHANNEL_R = 0;
constexpr uint8_t LED_PWM_CHANNEL_G = 1;
constexpr uint8_t LED_PWM_CHANNEL_B = 2;
constexpr uint32_t LED_PWM_FREQUENCY = 1000;
constexpr uint8_t LED_PWM_RESOLUTION = 8;
constexpr float LED_GAMMA = 2.2f;

constexpr touch_pad_t TOUCH_PIN = TOUCH_PAD_NUM3; // Touch конфигурация (используем T3 = GPIO15)
constexpr float TOUCH_THRESHOLD_PERCENT = 0.8f; // Порог срабатывания тача (% от baseline)
constexpr uint32_t TOUCH_DEBOUNCE_MS = 50;
constexpr uint32_t TOUCH_LONG_PRESS_MS = 1000;
constexpr uint32_t TOUCH_VERY_LONG_PRESS_MS = 5000;

// Button конфигурация (GPIO15)
constexpr uint8_t BUTTON_PIN = 15;
constexpr uint32_t BUTTON_DEBOUNCE_MS = 50;
constexpr uint32_t BUTTON_LONG_PRESS_MS = 1000;
constexpr uint32_t BUTTON_VERY_LONG_PRESS_MS = 5000;

// Глобальные объекты HAL
I2CBus mainBus(I2CBusConfig(&Wire, I2C_SDA_PIN, I2C_SCL_PIN, I2C_SPEED)); // NOLINT(*-interfaces-global-init)

// Конфигурация и создание HAL-объектов
LCD2004 lcd(LCD2004Config(LCD_I2C_ADDR, mainBus, LCD_COLUMNS, LCD_ROWS, true, false));
BME280Sensor bme(BME280Config(BME280_I2C_ADDR, mainBus, SEA_LEVEL_PRESSURE_HPA));
DS18B20Manager ds18b20(DS18B20Config(DS18B20_PIN, 12, false, DS18B20_UPDATE_INTERVAL));
RGBLED ledStrip(
    RGBLEDConfig(
        LED_R_PIN, LED_G_PIN, LED_B_PIN,
        LED_PWM_CHANNEL_R, LED_PWM_CHANNEL_G, LED_PWM_CHANNEL_B,
        LED_PWM_FREQUENCY, LED_PWM_RESOLUTION, LED_GAMMA, true));

// WiFi и веб-сервер
WiFiSettings wifiSettings;
WiFiConfig wifiConfig("", "", 15000, true); // Credentials будут загружены из NVS
WiFiManager wifi(wifiConfig);

// Temperature-based LED color controller
TemperatureRangeColor tempColor(25.0f);

// Mock temperature sensor (for web testing)
float mockTemperature = -1.0f; // -1 means disabled (use real sensor)

// Serial temperature control
bool serialTempControl = false;
float serialTempValue = 25.0f;
const float TEMP_STEP = 0.5f;
const float TEMP_MIN = -10.0f;
const float TEMP_MAX = 100.0f;

// OTA
LCDOTAPresenter otaPresenter(&lcd);
OTAManager ota(OTAConfig("banya-controller", 3232, nullptr, false), &otaPresenter);

// Forward declaration
Status getStatus();

BanyaWebServer webServer(WebServerConfig(), getStatus, &ledStrip, &wifi, &wifiSettings, &lcd, &ota);

// TouchSensor touch(TouchConfig(TOUCH_PIN, TOUCH_THRESHOLD_PERCENT, TOUCH_DEBOUNCE_MS, TOUCH_LONG_PRESS_MS,
//                               TOUCH_VERY_LONG_PRESS_MS));

Button button(ButtonConfig(BUTTON_PIN, true, BUTTON_DEBOUNCE_MS, BUTTON_LONG_PRESS_MS, BUTTON_VERY_LONG_PRESS_MS));

PageManager pageManager(lcd);

// Индекс страницы WiFi Setup (используется в callback)
uint8_t wifiSetupIdx = -1;

/**
 * @brief Handle serial commands for temperature control
 * Commands:
 * - 'w' or 'W': Increase temperature
 * - 's' or 'S': Decrease temperature
 * - 'r' or 'R': Reset to real sensor (disable mock)
 * - 'm' or 'M': Enable mock temperature control
 * - '0'-'9': Set specific temperature
 */
void handleSerialTempControl() {
    if (Serial.available()) {
        char cmd = Serial.read();
        
        switch (cmd) {
            case 'w':
            case 'W':
            case '+':
            case 'u':  // up
            case 'U':
                serialTempValue = min(serialTempValue + TEMP_STEP, TEMP_MAX);
                mockTemperature = serialTempValue;
                serialTempControl = true;
                Serial.printf("Temp: %.1f°C (+)\n", serialTempValue);
                break;
                
            case 's':
            case 'S':
            case '-':
            case 'd':  // down
            case 'D':
                serialTempValue = max(serialTempValue - TEMP_STEP, TEMP_MIN);
                mockTemperature = serialTempValue;
                serialTempControl = true;
                Serial.printf("Temp: %.1f°C (-)\n", serialTempValue);
                break;
                
            case 'r':
            case 'R':
                mockTemperature = -1.0f;
                serialTempControl = false;
                Serial.println("Using real sensor");
                break;
                
            case 'm':
            case 'M':
                mockTemperature = serialTempValue;
                serialTempControl = true;
                Serial.printf("Mock temp enabled: %.1f°C\n", serialTempValue);
                break;
                
            case '0':
                serialTempValue = 0.0f;
                mockTemperature = serialTempValue;
                serialTempControl = true;
                Serial.printf("Temp: %.1f°C\n", serialTempValue);
                break;
            case '1':
                serialTempValue = 10.0f;
                mockTemperature = serialTempValue;
                serialTempControl = true;
                Serial.printf("Temp: %.1f°C\n", serialTempValue);
                break;
            case '2':
                serialTempValue = 20.0f;
                mockTemperature = serialTempValue;
                serialTempControl = true;
                Serial.printf("Temp: %.1f°C\n", serialTempValue);
                break;
            case '3':
                serialTempValue = 30.0f;
                mockTemperature = serialTempValue;
                serialTempControl = true;
                Serial.printf("Temp: %.1f°C\n", serialTempValue);
                break;
            case '4':
                serialTempValue = 40.0f;
                mockTemperature = serialTempValue;
                serialTempControl = true;
                Serial.printf("Temp: %.1f°C\n", serialTempValue);
                break;
            case '5':
                serialTempValue = 50.0f;
                mockTemperature = serialTempValue;
                serialTempControl = true;
                Serial.printf("Temp: %.1f°C\n", serialTempValue);
                break;
            case '6':
                serialTempValue = 60.0f;
                mockTemperature = serialTempValue;
                serialTempControl = true;
                Serial.printf("Temp: %.1f°C\n", serialTempValue);
                break;
            case '7':
                serialTempValue = 70.0f;
                mockTemperature = serialTempValue;
                serialTempControl = true;
                Serial.printf("Temp: %.1f°C\n", serialTempValue);
                break;
            case '8':
                serialTempValue = 80.0f;
                mockTemperature = serialTempValue;
                serialTempControl = true;
                Serial.printf("Temp: %.1f°C\n", serialTempValue);
                break;
            case '9':
                serialTempValue = 90.0f;
                mockTemperature = serialTempValue;
                serialTempControl = true;
                Serial.printf("Temp: %.1f°C\n", serialTempValue);
                break;
            
            case '?':
            case 'h':
            case 'H':
                Serial.println("\n=== Temperature Control ===");
                Serial.println("w/u/+ : Increase temperature (+0.5°C)");
                Serial.println("s/d/- : Decrease temperature (-0.5°C)");
                Serial.println("0-9   : Set temperature (0=0°C, 5=50°C, etc.)");
                Serial.println("m     : Enable mock temperature");
                Serial.println("r     : Use real sensor");
                Serial.println("?/h   : Show this help");
                Serial.println("=========================\n");
                break;
                
            default:
                break;
        }
    }
}

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

Status getStatus() {
    Status status;

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

/**
 * @brief Callback для обработки событий Touch сенсора
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
            pageManager.goToPage(wifiSetupIdx);
            pageManager.render();
            break;

        case TouchEvent::TAP:
            pageManager.nextPage();
            break;

        default:
            break;
    }
}

/**
 * @brief Callback для обработки событий Button
 */
void handleButtonCallback(ButtonEvent event) {
    switch (event) {
        case ButtonEvent::LONG_PRESS:
            wifi.reconnect();
            break;

        case ButtonEvent::VERY_LONG_PRESS:
            // Enter WiFi Setup Mode
            Serial.println("Button: Very-long press - Entering WiFi Setup Mode...");

            if (!wifi.isAPEnabled()) {
                wifi.enableAP();
            } else {
                wifi.disableAP();
            }

            // Переходим на страницу настройки WiFi
            pageManager.goToPage(wifiSetupIdx);
            pageManager.render();
            break;

        case ButtonEvent::TAP:
            pageManager.nextPage();
            break;

        default:
            break;
    }
}

void setupSerial() {
    Serial.begin(115200);
    Serial.println("\n=== Banya Controller HAL ===");
}

void setupI2C() {
    // Инициализация I2C шины (перед устройствами!)
    mainBus.begin();
}

void setupLCD() {
    // Инициализация LCD
    Serial.print("Initializing LCD... ");
    if (lcd.begin()) {
        Serial.println("OK");
        displayWelcome();
    } else {
        Serial.println("FAILED");
    }
}

void setupBME280() {
    // Инициализация BME280
    Serial.print("Initializing BME280... ");
    if (bme.begin()) {
        Serial.println("OK");
    } else {
        Serial.println("FAILED - check wiring!");
    }
}

void setupDS18B20() {
    // Инициализация DS18B20
    Serial.print("Initializing DS18B20... ");
    if (ds18b20.begin()) {
        Serial.print("Found ");
        Serial.print(ds18b20.getSensorCount());
        Serial.println(" sensors");
    } else {
        Serial.println("FAILED - no sensors found");
    }
}

void setupLEDStrip() {
    // Инициализация RGB LED
    Serial.print("Initializing RGB LED... ");
    if (ledStrip.begin()) {
        Serial.println("OK");
        ledStrip.setBrightness(0.2);
        ledStrip.enableGamma(true);
    } else {
        Serial.println("FAILED");
    }
}

void setupWifi() {
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
    wifi.begin();
    Serial.print("Initialized WiFi");

    // Update WiFi credentials before connecting
    wifi.setCredentials(wifiConfig.ssid, wifiConfig.password);

    // Подключение к WiFi
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.write(1); // WiFi иконка
    lcd.print(" Connecting...");
    lcd.setCursor(0, 2);
    lcd.print(wifiConfig.ssid);

    wifi.connect();

    if (!wifi.isConnected()) {
        Serial.println("WiFi: Failed to connect");
        lcd.setCursor(0, 3);
        lcd.print("Failed!");
        // Включаем AP если не удалось подключиться и есть настройки
        if (wifiSettings.isConfigured()) {
            Serial.println("WiFi: No connection, will enter AP mode on request");
        }
    }
}

void setupOTA() {
    // Инициализация OTA (только если WiFi подключён)
    if (wifi.isConnected()) {
        Serial.print("Initializing OTA... ");
        if (ota.begin()) {
            Serial.println("OK");
            Serial.println(ota.getInfo());
        } else {
            Serial.println("FAILED");
        }
    }
}

void setupWebServer() {
    // Запуск веб-сервера (всегда, даже если WiFi не подключён)
    webServer.begin();
    webServer.start();
    if (wifi.isConnected()) {
        Serial.printf(
            "WebServer: Started on http://%s:%u\n",
            wifi.getIPAddressString().c_str(),
            webServer.getConfig().port);
    }
}

// void setupTouch() {
//     // Инициализация Touch сенсора
//     Serial.print("Initializing Touch... ");
//     if (touch.begin()) {
//         Serial.println("OK");
//         Serial.print("Touch: Baseline = ");
//         Serial.println(touch.getBaselineValue());
//         Serial.print("Touch: Threshold = ");
//         Serial.println(touch.getThreshold());
//
//         // Установка callback для обработки long press событий
//         touch.setCallback(handleTouchCallback);
//     } else {
//         Serial.println("FAILED");
//     }
// }

void setupButton() {
    // Инициализация Button
    Serial.print("Initializing Button... ");
    if (button.begin()) {
        Serial.println("OK");
        // Установка callback для обработки событий
        button.setCallback(handleButtonCallback);
    } else {
        Serial.println("FAILED");
    }
}

void setupPageManager() {
    // Создание страниц
    Serial.println("Creating display pages...");

    pageManager.addPage(std::unique_ptr<DisplayPage>(new DallasSensorsPage(&ds18b20, "Dallas DS18B20")));
    pageManager.addPage(std::unique_ptr<DisplayPage>(new BME280Page(&bme, "BME280 Sensor")));
    pageManager.addPage(std::unique_ptr<DisplayPage>(new LEDStripPage(&ledStrip, "LED Strip")));
    pageManager.addPage(std::unique_ptr<DisplayPage>(new WiFiInfoPage(&wifi, "WiFi Info")));
    wifiSetupIdx = pageManager.addPage(
        std::unique_ptr<DisplayPage>(new WiFiSetupPage(&wifi, &wifiSettings, "WiFi Setup")));
    pageManager.addPage(std::unique_ptr<DisplayPage>(new SystemStatusPage(&wifi, "System Status")));

    // Установка начальной страницы
    pageManager.goToPage(0);
    pageManager.render();

    Serial.print("Pages created: ");
    Serial.println(pageManager.getPageCount());
}

void setup() {
    setupSerial();
    setupI2C();
    setupLCD();
    setupBME280();
    setupDS18B20();
    setupLEDStrip();
    setupWifi();
    setupOTA();
    setupWebServer();
    // setupTouch();
    setupButton();
    setupPageManager();
}

IntervalTimer lcdConnectionTimer(5000); // 5 секунд

void loop() {
    // Handle serial temperature control commands
    handleSerialTempControl();
    
    // Автоматическое обновление температур DS18B20
    ds18b20.handleLoop();

    // Update LED strip color based on temperature (use first sensor or mock)
    float temp = mockTemperature >= 0 ? mockTemperature : ds18b20.getTemperature(0);
    
    if (temp != DEVICE_DISCONNECTED_C) {
        // Update temperature and check if color changed (needs fade)
        tempColor.updateTemperature(temp);

        // Debug output
        static unsigned long lastDebug = 0;
        if (millis() - lastDebug > 5000) {
            RGB color = tempColor.getDisplayedColor();
            Serial.printf("Temp: %.1f°C Range: %u Color: R=%u G=%u B=%u Fade: %s\n",
                temp, tempColor.getCurrentRangeIndex(),
                color.red, color.green, color.blue,
                tempColor.isFadingActive() ? "yes" : "no");
            lastDebug = millis();
        }

        // Run blocking fade animation if color changed
        if (tempColor.isFadingActive()) {
            tempColor.runBlockingFade(&ledStrip);
        }
    } else {
        Serial.printf("Temperature sensor disconnected! Temp reading: %.1f\n", temp);
    }

    // Обработка веб-клиентов
    webServer.handleLoop();

    // Обработка процесса WiFi переподключения (неблокирующее)
    wifi.handleLoop();

    // Обработка OTA обновлений
    ota.handleLoop();

    // Обработка Touch событий (tap, long press, very-long press)
    // touch.handleLoop();

    // Обработка Button событий (tap, long press, very-long press)
    button.handleLoop();

    pageManager.handleLoop();

    // Проверка подключения LCD (периодически)
    lcdConnectionTimer.handleLoop([] {
        if (!lcd.isConnected()) {
            Serial.println("LCD: Reconnecting...");
            lcd.begin();
        }
    });
}
