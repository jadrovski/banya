#ifndef SAUNA_HAL_H
#define SAUNA_HAL_H

/**
 * @file HAL.h
 * @brief Hardware Access Layer для проекта Sauna Controller
 * 
 * Этот файл предоставляет удобный способ импорта всех HAL-компонентов
 * 
 * Пример использования:
 * @code
 * #include "hal/HAL.h"
 * 
 * HAL::LCD lcd;
 * HAL::BME280Sensor bme;
 * HAL::DS18B20Manager ds18b20;
 * HAL::RGBLED led;
 * HAL::WiFiManager wifi;
 * HAL::SaunaWebServer webServer;
 * HAL::TouchSensor touch;
 * HAL::PageManager pageMgr;
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
#include "hal/WebServer.h"
#include "hal/Touch.h"
#include "hal/DisplayPages.h"

#endif // SAUNA_HAL_H
