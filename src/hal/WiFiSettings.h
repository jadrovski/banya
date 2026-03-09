#ifndef BANYA_HAL_WIFI_SETTINGS_H
#define BANYA_HAL_WIFI_SETTINGS_H

#include <Arduino.h>
#include <Preferences.h>
#include <nvs_flash.h>

namespace HAL {
    /**
     * @brief Конфигурация WiFi для хранения в NVS
     */
    struct WiFiCredentials {
        String ssid;
        String password;

        WiFiCredentials() : ssid(""), password("") {
        }

        WiFiCredentials(const String &s, const String &p)
            : ssid(s), password(p) {
        }
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
        const char *namespaceName = "wifi_creds";
        bool configured;
        bool nvsInitialized;

        /**
         * @brief Проверка наличия сохранённых настроек (внутренняя)
         * @return true если настройки сохранены
         */
        bool checkConfigured() const {
            // Preferences не имеет const методов, поэтому костыль через const_cast
            Preferences &prefs = const_cast<Preferences &>(preferences);
            return prefs.isKey("ssid");
        }

    public:
        /**
         * @brief Конструктор WiFiSettings
         */
        WiFiSettings() : configured(false), nvsInitialized(false) {
        }

        /**
         * @brief Инициализация хранилища настроек
         * @return true если успешно
         */
        bool begin() {
            // Initialize NVS flash storage if not already done
            esp_err_t ret = nvs_flash_init();
            Serial.print("WiFiSettings: nvs_flash_init() returned: ");
            Serial.println(ret);

            if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
                // NVS partition was truncated and needs to be erased
                Serial.println("WiFiSettings: NVS partition truncated, erasing...");
                ret = nvs_flash_erase();
                if (ret != ESP_OK) {
                    Serial.print("WiFiSettings: Failed to erase NVS, error: ");
                    Serial.println(ret);
                    return false;
                }
                ret = nvs_flash_init();
                Serial.print("WiFiSettings: After erase, nvs_flash_init() returned: ");
                Serial.println(ret);
            }
            if (ret != ESP_OK) {
                Serial.print("WiFiSettings: Failed to initialize NVS, error: ");
                Serial.println(ret);
                return false;
            }
            nvsInitialized = true;
            Serial.println("WiFiSettings: NVS initialized");

            ret = preferences.begin(namespaceName, false); // false = read/write

            // Handle namespace open failure - erase NVS and retry
            if (!ret) {
                Serial.print("WiFiSettings: Failed to open namespace.");
            }

            // Проверяем, есть ли сохранённые настройки
            configured = checkConfigured();

            Serial.print("WiFiSettings: After checkConfigured(), configured = ");
            Serial.println(configured ? "true" : "false");

            // Debug: list all keys in namespace
            Serial.println("WiFiSettings: Checking for keys in namespace...");
            if (preferences.isKey("ssid")) {
                Serial.println("WiFiSettings: Found key 'ssid'");
                Serial.print("WiFiSettings: SSID value = ");
                Serial.println(getSSID());
            } else {
                Serial.println("WiFiSettings: Key 'ssid' NOT found");
            }
            if (preferences.isKey("pass")) {
                Serial.println("WiFiSettings: Found key 'pass'");
            }
            if (preferences.isKey("auto")) {
                Serial.println("WiFiSettings: Found key 'auto'");
            }

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
        bool save(const WiFiCredentials &creds) {
            esp_err_t ret;

            Serial.print("WiFiSettings: Saving credentials for SSID: ");
            Serial.println(creds.ssid);

            // Сохраняем SSID
            ret = preferences.putString("ssid", creds.ssid);
            if (ret > 0) {
                Serial.print("WiFiSettings: Saved SSID: ");
                Serial.println(creds.ssid);
            } else {
                Serial.print("WiFiSettings: Failed to save SSID, error: ");
                Serial.println(ret);
                return false;
            }

            // Сохраняем пароль
            ret = preferences.putString("pass", creds.password);
            if (ret > 0) {
                Serial.println("WiFiSettings: Saved password");
            } else {
                Serial.print("WiFiSettings: Failed to save password, error: ");
                Serial.println(ret);
                return false;
            }

            // Preferences auto-commits, but verify the save
            configured = checkConfigured();
            Serial.print("WiFiSettings: configured flag after save = ");
            Serial.println(configured ? "true" : "false");

            Serial.println("WiFiSettings: All credentials saved successfully");
            return true;
        }

        /**
         * @brief Сохранить credentials (удобная версия)
         * @param ssid SSID сети
         * @param password Пароль
         * @return true если успешно
         */
        bool save(const String &ssid, const String &password) {
            return save(WiFiCredentials(ssid, password));
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
         * @brief Получить сохранённые credentials
         * @return WiFiCredentials struct
         */
        WiFiCredentials getCredentials() {
            WiFiCredentials creds;
            creds.ssid = preferences.getString("ssid", "");
            creds.password = preferences.getString("pass", "");
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
            if (!ret) {
                Serial.print("WiFiSettings: Failed to remove SSID, error: ");
            }

            ret = preferences.remove("pass");
            if (!ret) {
                Serial.print("WiFiSettings: Failed to remove password, error: ");
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
            }

            return info;
        }
    };
} // namespace HAL

#endif // BANYA_HAL_WIFI_SETTINGS_H
