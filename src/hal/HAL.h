#ifndef BANYA_HAL_H
#define BANYA_HAL_H

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
 * LCD2004 lcd;
 * BME280Sensor bme;
 * DS18B20Manager ds18b20;
 * RGBLED led;
 * WiFiManager wifi;
 * BanyaWebServer webServer;
 * TouchSensor touch;
 * PageManager pageMgr;
 *
 * void setup() {
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

#include "hal/I2CDevice.h"
#include "hal/LCD.h"
#include "hal/BME280.h"
#include "hal/DS18B20.h"
#include "hal/RGBLED.h"
#include "hal/WiFi.h"
#include "hal/WiFiSettings.h"
#include "hal/Touch.h"

#endif // BANYA_HAL_H
