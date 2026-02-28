#ifndef SAUNA_HAL_WIFI_H
#define SAUNA_HAL_WIFI_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

namespace HAL {

/**
 * @brief Конфигурация WiFi
 */
struct WiFiConfig {
    const char* ssid;             // SSID сети
    const char* password;         // Пароль
    uint32_t connectionTimeout;   // Таймаут подключения (мс)
    bool autoReconnect;           // Авто-переподключение

    WiFiConfig(
        const char* ssid = "",
        const char* password = "",
        uint32_t timeout = 10000,
        bool autoReconn = true
    ) : ssid(ssid), password(password), 
        connectionTimeout(timeout), autoReconnect(autoReconn) {}
};

/**
 * @brief Статус WiFi подключения
 */
enum class WiFiStatus {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    FAILED,
    LOST_CONNECTION
};

/**
 * @brief Hardware Access Layer для WiFi
 * 
 * Поддерживает:
 * - Подключение к WiFi сети
 * - Получение IP адреса
 * - Мониторинг статуса подключения
 * - Авто-переподключение
 */
class WiFiManager {
private:
    WiFiConfig config;
    WiFiStatus status;
    IPAddress ipAddress;
    unsigned long connectionStartTime;
    bool connectionAttempted;
    
    // Callback для обновления статуса
    std::function<void(WiFiStatus)> statusCallback;

public:
    /**
     * @brief Конструктор WiFiManager
     * @param cfg Конфигурация WiFi
     */
    explicit WiFiManager(const WiFiConfig& cfg = WiFiConfig())
        : config(cfg),
          status(WiFiStatus::DISCONNECTED),
          ipAddress(INADDR_NONE),
          connectionStartTime(0),
          connectionAttempted(false),
          statusCallback(nullptr) {}

    /**
     * @brief Инициализация WiFi
     * @return true если успешно
     */
    bool begin() {
        // Отключаем режим точки доступа для экономии энергии
        WiFi.mode(WIFI_STA);
        
        // Настраиваем авто-переподключение
        WiFi.setAutoReconnect(config.autoReconnect);
        
        status = WiFiStatus::DISCONNECTED;
        return true;
    }

    /**
     * @brief Подключение к WiFi сети
     * @return true если успешно подключились
     */
    bool connect() {
        if (!config.ssid || strlen(config.ssid) == 0) {
            Serial.println("WiFi: SSID is empty");
            status = WiFiStatus::FAILED;
            return false;
        }

        Serial.print("WiFi: Connecting to '");
        Serial.print(config.ssid);
        Serial.println("'...");

        status = WiFiStatus::CONNECTING;
        connectionStartTime = millis();
        connectionAttempted = true;

        // Начинаем подключение
        WiFi.begin(config.ssid, config.password);

        // Ждём подключения
        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");

            // Проверка таймаута
            if (millis() - startTime > config.connectionTimeout) {
                Serial.println("\nWiFi: Connection timeout");
                status = WiFiStatus::FAILED;
                return false;
            }

            // Проверка на ошибку
            if (WiFi.status() == WL_CONNECT_FAILED) {
                Serial.println("\nWiFi: Connection failed");
                status = WiFiStatus::FAILED;
                return false;
            }
        }

        // Успешное подключение
        ipAddress = WiFi.localIP();
        status = WiFiStatus::CONNECTED;

        Serial.println("\nWiFi: Connected!");
        Serial.print("WiFi: IP address: ");
        Serial.println(ipAddress);

        return true;
    }

    /**
     * @brief Отключение от WiFi
     */
    void disconnect() {
        WiFi.disconnect(true);
        status = WiFiStatus::DISCONNECTED;
        ipAddress = INADDR_NONE;
        Serial.println("WiFi: Disconnected");
    }

    /**
     * @brief Проверка статуса подключения
     * @return true если подключены
     */
    bool isConnected() {
        if (WiFi.status() != WL_CONNECTED) {
            if (status == WiFiStatus::CONNECTED) {
                status = WiFiStatus::LOST_CONNECTION;
                Serial.println("WiFi: Connection lost");
            }
            return false;
        }
        
        // Обновляем IP если был потерян
        if (ipAddress == INADDR_NONE && WiFi.status() == WL_CONNECTED) {
            ipAddress = WiFi.localIP();
        }
        
        return true;
    }

    /**
     * @brief Получить IP адрес
     * @return IP адрес или INADDR_NONE если не подключены
     */
    IPAddress getIPAddress() {
        if (!isConnected()) {
            return INADDR_NONE;
        }
        return ipAddress;
    }

    /**
     * @brief Получить IP адрес в виде строки
     * @return Строка с IP адресом
     */
    String getIPAddressString() {
        if (!isConnected()) {
            return "Not connected";
        }
        return ipAddress.toString();
    }

    /**
     * @brief Получить статус подключения
     * @return WiFiStatus
     */
    WiFiStatus getStatus() const { return status; }

    /**
     * @brief Получить строковое представление статуса
     * @return Строка со статусом
     */
    String getStatusString() const {
        switch (status) {
            case WiFiStatus::DISCONNECTED: return "Disconnected";
            case WiFiStatus::CONNECTING: return "Connecting...";
            case WiFiStatus::CONNECTED: return "Connected";
            case WiFiStatus::FAILED: return "Failed";
            case WiFiStatus::LOST_CONNECTION: return "Connection Lost";
            default: return "Unknown";
        }
    }

    /**
     * @brief Получить RSSI (уровень сигнала)
     * @return RSSI в dBm
     */
    int getRSSI() {
        if (!isConnected()) return 0;
        return WiFi.RSSI();
    }

    /**
     * @brief Получить MAC адрес
     * @return Строка с MAC адресом
     */
    String getMACAddress() const {
        return WiFi.macAddress();
    }

    /**
     * @brief Получить качество сигнала в процентах
     * @return Качество 0-100%
     */
    int getSignalQuality() {
        int rssi = getRSSI();
        if (rssi <= -100) return 0;
        if (rssi >= -50) return 100;
        return map(rssi, -100, -50, 0, 100);
    }

    /**
     * @brief Получить WiFi канал
     * @return Номер канала (1-14) или 0 если не подключены
     */
    int getChannel() {
        if (!isConnected()) return 0;
        return WiFi.channel();
    }

    /**
     * @brief Получить время подключения (мс)
     * @return Время в мс с начала подключения
     */
    unsigned long getConnectionTime() const {
        if (!connectionAttempted) return 0;
        return millis() - connectionStartTime;
    }

    /**
     * @brief Переподключение к WiFi
     * @return true если успешно
     */
    bool reconnect() {
        disconnect();
        delay(1000);
        return connect();
    }

    /**
     * @brief Установить callback изменения статуса
     * @param callback Функция обратного вызова
     */
    void setStatusCallback(std::function<void(WiFiStatus)> callback) {
        statusCallback = callback;
    }

    /**
     * @brief Получить конфигурацию
     */
    const WiFiConfig& getConfig() const { return config; }

    /**
     * @brief Получить информацию о WiFi
     */
    String getInfo() const {
        String info = "WiFi: ";
        info += getStatusString();
        
        if (status == WiFiStatus::CONNECTED) {
            info += " | IP: ";
            info += ipAddress.toString();
            info += " | RSSI: ";
            info += WiFi.RSSI();
            info += " dBm";
        }
        
        return info;
    }
};

} // namespace HAL

#endif // SAUNA_HAL_WIFI_H
