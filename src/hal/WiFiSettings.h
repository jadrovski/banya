#ifndef BANYA_HAL_WIFI_SETTINGS_H
#define BANYA_HAL_WIFI_SETTINGS_H

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>

namespace HAL {

/**
 * @brief Конфигурация WiFi для хранения в NVS
 */
struct WiFiCredentials {
    String ssid;
    String password;
    bool autoConnect;

    WiFiCredentials() : ssid(""), password(""), autoConnect(true) {}
    WiFiCredentials(const String& s, const String& p, bool autoC = true)
        : ssid(s), password(p), autoConnect(autoC) {}
};

/**
 * @brief Менеджер настроек WiFi с хранением в NVS (Non-Volatile Storage)
 *
 * Использует ESP32 Preferences API для хранения:
 * - SSID и пароль WiFi сети
 * - Флаг авто-подключения
 * - Статус настроенной сети
 *
 * Namespace: "wifi_creds"
 * Keys: "ssid", "pass", "auto"
 */
class WiFiSettings {
private:
    Preferences preferences;
    const char* namespaceName = "wifi_creds";
    bool configured;

    /**
     * @brief Проверка наличия сохранённых настроек (внутренняя)
     * @return true если настройки сохранены
     */
    bool checkConfigured() const {
        // Preferences не имеет const методов, поэтому костыль через const_cast
        Preferences& prefs = const_cast<Preferences&>(preferences);
        return prefs.isKey("ssid");
    }

public:
    /**
     * @brief Конструктор WiFiSettings
     */
    WiFiSettings() : configured(false) {}

    /**
     * @brief Инициализация хранилища настроек
     * @return true если успешно
     */
    bool begin() {
        esp_err_t ret = preferences.begin(namespaceName, false); // false = read/write
        if (ret != ESP_OK) {
            Serial.print("WiFiSettings: Failed to open namespace, error: ");
            Serial.println(ret);
            return false;
        }

        // Проверяем, есть ли сохранённые настройки
        configured = checkConfigured();

        Serial.println("WiFiSettings: Initialized");
        Serial.print("WiFiSettings: Configured: ");
        Serial.println(configured ? "Yes" : "No");

        if (configured) {
            Serial.print("WiFiSettings: SSID: ");
            Serial.println(getSSID());
        }

        return true;
    }

    /**
     * @brief Закрыть хранилище (освободить ресурсы)
     */
    void end() {
        preferences.end();
        Serial.println("WiFiSettings: Closed");
    }

    /**
     * @brief Сохранить WiFi credentials в NVS
     * @param creds Данные для сохранения
     * @return true если успешно
     */
    bool save(const WiFiCredentials& creds) {
        esp_err_t ret;

        // Сохраняем SSID
        ret = preferences.putString("ssid", creds.ssid);
        if (ret == 0) {
            Serial.print("WiFiSettings: Saved SSID: ");
            Serial.println(creds.ssid);
        } else {
            Serial.print("WiFiSettings: Failed to save SSID, error: ");
            Serial.println(ret);
            return false;
        }

        // Сохраняем пароль
        ret = preferences.putString("pass", creds.password);
        if (ret == 0) {
            Serial.println("WiFiSettings: Saved password");
        } else {
            Serial.print("WiFiSettings: Failed to save password, error: ");
            Serial.println(ret);
            return false;
        }

        // Сохраняем флаг авто-подключения
        ret = preferences.putBool("auto", creds.autoConnect);
        if (ret == 0) {
            Serial.print("WiFiSettings: Saved auto-connect: ");
            Serial.println(creds.autoConnect ? "Yes" : "No");
        } else {
            Serial.print("WiFiSettings: Failed to save auto-connect, error: ");
            Serial.println(ret);
            return false;
        }

        configured = true;
        Serial.println("WiFiSettings: All credentials saved successfully");
        return true;
    }

    /**
     * @brief Сохранить credentials (удобная версия)
     * @param ssid SSID сети
     * @param password Пароль
     * @param autoConnect Авто-подключение
     * @return true если успешно
     */
    bool save(const String& ssid, const String& password, bool autoConnect = true) {
        return save(WiFiCredentials(ssid, password, autoConnect));
    }

    /**
     * @brief Получить сохранённый SSID
     * @return SSID или пустая строка если не настроено
     */
    String getSSID() {
        return preferences.getString("ssid", "");
    }

    /**
     * @brief Получить сохранённый пароль
     * @return Пароль или пустая строка если не настроено
     */
    String getPassword() {
        return preferences.getString("pass", "");
    }

    /**
     * @brief Получить флаг авто-подключения
     * @return true если авто-подключение включено
     */
    bool getAutoConnect() {
        return preferences.getBool("auto", true);
    }

    /**
     * @brief Получить сохранённые credentials
     * @return WiFiCredentials struct
     */
    WiFiCredentials getCredentials() {
        WiFiCredentials creds;
        creds.ssid = preferences.getString("ssid", "");
        creds.password = preferences.getString("pass", "");
        creds.autoConnect = preferences.getBool("auto", true);
        return creds;
    }

    /**
     * @brief Проверка, настроены ли WiFi credentials
     * @return true если credentials сохранены
     */
    bool isConfigured() { return configured; }

    /**
     * @brief Удалить все сохранённые credentials
     * @return true если успешно
     */
    bool clear() {
        esp_err_t ret;

        ret = preferences.remove("ssid");
        if (ret != 0) {
            Serial.print("WiFiSettings: Failed to remove SSID, error: ");
            Serial.println(ret);
        }

        ret = preferences.remove("pass");
        if (ret != 0) {
            Serial.print("WiFiSettings: Failed to remove password, error: ");
            Serial.println(ret);
        }

        ret = preferences.remove("auto");
        if (ret != 0) {
            Serial.print("WiFiSettings: Failed to remove auto, error: ");
            Serial.println(ret);
        }

        configured = false;
        Serial.println("WiFiSettings: Credentials cleared");
        return true;
    }

    /**
     * @brief Получить информацию о хранилище
     * @return Строка с информацией
     */
    String getInfo() {
        String info = "WiFiSettings: ";
        info += configured ? "Configured" : "Not configured";

        if (configured) {
            info += " | SSID: ";
            info += getSSID();
            info += " | Auto: ";
            info += getAutoConnect() ? "Yes" : "No";
        }

        return info;
    }
};

} // namespace HAL

#endif // BANYA_HAL_WIFI_SETTINGS_H
