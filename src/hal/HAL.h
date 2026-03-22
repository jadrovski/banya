#pragma once

/**
 * @file HAL.h
 * @brief Hardware Access Layer для проекта Banya Controller
 *
 * Этот файл предоставляет удобный способ импорта всех HAL-компонентов
 *
 * Пример использования:
 * @code
 * #include "hal/HAL.h"
 *
 * // Создаем I2C шину (одна для всех устройств)
 * I2CBus mainBus(I2CBusConfig(&Wire, 21, 22, 100000));
 *
 * // Создаем устройства с ссылкой на шину
 * LCD2004 lcd(LCD2004Config(0x27, mainBus));
 * BME280Sensor bme(BME280Config(0x76, mainBus));
 * DS18B20Manager ds18b20;
 * RGBLED led;
 * WiFiManager wifi;
 * BanyaWebServer webServer;
 * TouchSensor touch;
 * PageManager pageMgr;
 *
 * void setup() {
 *     // Сначала инициализируем шину
 *     mainBus.begin();
 *
 *     // Затем устройства
 *     lcd.begin();
 *     bme.begin();
 *     ds18b20.begin();
 *     led.begin();
 *     wifi.begin();
 *     wifi.connect();
 *     touch.begin();
 * }
 * @endcode
 */

#include "hal/I2CBus.h"
#include "hal/I2CDevice.h"
#include "hal/LCD.h"
#include "hal/BME280.h"
#include "hal/DS18B20.h"
#include "hal/RGBLED.h"
#include "hal/WiFi.h"
#include "hal/WiFiSettings.h"
#include "hal/Touch.h"
#include "hal/SerialTempSensor.h"
#include "hal/ITemperatureSensor.h"
