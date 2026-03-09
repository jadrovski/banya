#ifndef BANYA_HAL_WIFI_H
#define BANYA_HAL_WIFI_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

namespace HAL {

/**
 * @brief Режим работы WiFi
 */
enum class WiFiMode {
    STA,        // Station mode (client)
    AP,         // Access Point mode
    AP_STA,     // Both AP and Station
    OFF         // WiFi off
};

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
 * @brief Конфигурация Access Point
 */
struct APConfig {
    const char* ssid;             // SSID точки доступа
    const char* password;         // Пароль (минимум 8 символов)
    uint8_t channel;              // Канал (1-13)
    bool ssidHidden;              // Скрытый SSID
    uint8_t maxConnections;       // Максимум подключений (1-10)

    APConfig(
        const char* ssid = "Banya-Controller",
        const char* password = "banya1234",
        uint8_t ch = 1,
        bool hidden = false,
        uint8_t maxConn = 4
    ) : ssid(ssid), password(password), channel(ch),
        ssidHidden(hidden), maxConnections(maxConn) {}
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
 * - Подключение к WiFi сети (STA mode)
 * - Режим точки доступа (AP mode)
 * - Получение IP адреса
 * - Мониторинг статуса подключения
 * - Авто-переподключение
 * - DNS сервер для captive portal
 */
class WiFiManager {
private:
    WiFiConfig staConfig;
    APConfig apConfig;
    WiFiMode currentMode;
    WiFiStatus status;
    IPAddress ipAddress;
    IPAddress apIpAddress;
    unsigned long connectionStartTime;
    unsigned long reconnectStartTime;
    bool connectionAttempted;
    bool apModeEnabled;
    
    // DNS сервер для captive portal
    std::unique_ptr<DNSServer> dnsServer;
    uint8_t dnsPort;

    // Callback для обновления статуса
    std::function<void(WiFiStatus)> statusCallback;

public:
    /**
     * @brief Конструктор WiFiManager
     * @param cfg Конфигурация WiFi
     */
    explicit WiFiManager(const WiFiConfig& cfg = WiFiConfig())
        : staConfig(cfg),
          apConfig(),
          currentMode(WiFiMode::OFF),
          status(WiFiStatus::DISCONNECTED),
          ipAddress(INADDR_NONE),
          apIpAddress(INADDR_NONE),
          connectionStartTime(0),
          reconnectStartTime(0),
          connectionAttempted(false),
          apModeEnabled(false),
          dnsServer(nullptr),
          dnsPort(53),
          statusCallback(nullptr) {}

    /**
     * @brief Инициализация WiFi
     * @return true если успешно
     */
    bool begin() {
        // По умолчанию режим станции
        WiFi.mode(WIFI_STA);
        currentMode = WiFiMode::STA;

        // Настраиваем авто-переподключение
        WiFi.setAutoReconnect(staConfig.autoReconnect);

        status = WiFiStatus::DISCONNECTED;
        return true;
    }

    /**
     * @brief Включить режим точки доступа (AP)
     * @param cfg Конфигурация AP
     * @return true если успешно
     */
    bool enableAP(const APConfig& cfg = APConfig()) {
        apConfig = cfg;
        
        Serial.print("WiFi: Enabling AP mode '");
        Serial.print(apConfig.ssid);
        Serial.println("'...");

        // Включаем режим AP или AP_STA
        if (status == WiFiStatus::CONNECTED) {
            WiFi.mode(WIFI_AP_STA);
            currentMode = WiFiMode::AP_STA;
        } else {
            WiFi.mode(WIFI_AP);
            currentMode = WiFiMode::AP;
        }

        // Настраиваем точку доступа
        bool ret = WiFi.softAP(
            apConfig.ssid,
            apConfig.password,
            apConfig.channel,
            apConfig.ssidHidden,
            apConfig.maxConnections
        );

        if (ret) {
            apIpAddress = WiFi.softAPIP();
            apModeEnabled = true;
            status = WiFiStatus::CONNECTED;
            
            Serial.print("WiFi: AP started, IP: ");
            Serial.println(apIpAddress);
            
            // Запускаем DNS сервер для captive portal
            startDNSServer();
            
            return true;
        } else {
            Serial.println("WiFi: Failed to start AP");
            apModeEnabled = false;
            return false;
        }
    }

    /**
     * @brief Включить режим точки доступа с заданными параметрами
     * @param ssid SSID точки доступа
     * @param password Пароль (минимум 8 символов)
     * @return true если успешно
     */
    bool enableAP(const char* ssid, const char* password = "") {
        return enableAP(APConfig(ssid, password));
    }

    /**
     * @brief Выключить режим точки доступа
     * @return true если успешно
     */
    bool disableAP() {
        if (!apModeEnabled) {
            return true;
        }

        Serial.println("WiFi: Disabling AP mode...");
        
        // Останавливаем DNS сервер
        stopDNSServer();

        // Возвращаемся в режим STA если были подключены
        if (status == WiFiStatus::CONNECTED) {
            WiFi.mode(WIFI_STA);
            currentMode = WiFiMode::STA;
        } else {
            WiFi.mode(WIFI_OFF);
            currentMode = WiFiMode::OFF;
        }

        apModeEnabled = false;
        apIpAddress = INADDR_NONE;
        Serial.println("WiFi: AP mode disabled");
        return true;
    }

    /**
     * @brief Запустить DNS сервер для captive portal
     */
    void startDNSServer() {
        if (dnsServer == nullptr) {
            dnsServer = std::unique_ptr<DNSServer>(new DNSServer());
        }
        
        // Перенаправляем все запросы на наш IP (captive portal)
        dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
        dnsServer->start(dnsPort, "*", apIpAddress);
        
        Serial.println("WiFi: DNS server started for captive portal");
    }

    /**
     * @brief Остановить DNS сервер
     */
    void stopDNSServer() {
        if (dnsServer != nullptr) {
            dnsServer->stop();
            dnsServer = nullptr;
            Serial.println("WiFi: DNS server stopped");
        }
    }

    /**
     * @brief Обработка DNS сервера (вызывать в loop)
     */
    void handleDNSServer() {
        if (dnsServer != nullptr) {
            dnsServer->processNextRequest();
        }
    }

    /**
     * @brief Подключение к WiFi сети
     * @return true если успешно подключились
     */
    bool connect() {
        if (!staConfig.ssid || strlen(staConfig.ssid) == 0) {
            Serial.println("WiFi: SSID is empty");
            status = WiFiStatus::FAILED;
            return false;
        }

        Serial.print("WiFi: Connecting to '");
        Serial.print(staConfig.ssid);
        Serial.println("'...");

        status = WiFiStatus::CONNECTING;
        connectionStartTime = millis();
        connectionAttempted = true;

        // Начинаем подключение
        WiFi.begin(staConfig.ssid, staConfig.password);

        // Ждём подключения
        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");

            // Проверка таймаута
            if (millis() - startTime > staConfig.connectionTimeout) {
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
        currentMode = apModeEnabled ? WiFiMode::AP_STA : WiFiMode::STA;

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
     * @brief Начать переподключение к WiFi (неблокирующее)
     * @return true если процесс запущен
     */
    bool reconnect() {
        if (status == WiFiStatus::CONNECTING) {
            return false; // Уже в процессе переподключения
        }
        disconnect();
        reconnectStartTime = millis();
        status = WiFiStatus::CONNECTING;
        connectionAttempted = true;
        WiFi.begin(staConfig.ssid, staConfig.password);
        Serial.println("WiFi: Reconnection started");
        return true;
    }

    /**
     * @brief Обновить конфигурацию STA (credentials)
     * @param ssid SSID сети
     * @param password Пароль
     */
    void setCredentials(const char* ssid, const char* password) {
        staConfig.ssid = ssid;
        staConfig.password = password;
        Serial.print("WiFi: Credentials updated. SSID: ");
        Serial.println(ssid);
    }

    /**
     * @brief Получить текущий режим работы WiFi
     * @return WiFiMode
     */
    WiFiMode getCurrentMode() const { return currentMode; }

    /**
     * @brief Проверка, включён ли AP режим
     * @return true если AP активен
     */
    bool isAPEnabled() const { return apModeEnabled; }

    /**
     * @brief Получить IP адрес точки доступа
     * @return IP адрес AP или INADDR_NONE
     */
    IPAddress getAPIPAddress() const { return apIpAddress; }

    /**
     * @brief Получить IP адрес AP в виде строки
     * @return Строка с IP адресом
     */
    String getAPIPAddressString() const {
        if (apIpAddress == INADDR_NONE) return "Not available";
        return apIpAddress.toString();
    }

    /**
     * @brief Получить количество подключённых клиентов к AP
     * @return Количество станций
     */
    int getAPStationCount() const {
        if (!apModeEnabled) return 0;
        return WiFi.softAPgetStationNum();
    }

    /**
     * @brief Получить конфигурацию STA
     */
    const WiFiConfig& getSTAConfig() const { return staConfig; }

    /**
     * @brief Получить конфигурацию AP
     */
    const APConfig& getAPConfig() const { return apConfig; }

    /**
     * @brief Обработка процесса переподключения (вызывать в loop)
     * @return true если подключились, false если ещё в процессе или ошибка
     */
    bool handleLoop() {
        if (status != WiFiStatus::CONNECTING) {
            return false;
        }

        // Обработка DNS запросов если AP включён
        handleDNSServer();

        if (WiFi.status() == WL_CONNECTED) {
            // Успешное подключение
            ipAddress = WiFi.localIP();
            status = WiFiStatus::CONNECTED;
            currentMode = apModeEnabled ? WiFiMode::AP_STA : WiFiMode::STA;
            Serial.println("\nWiFi: Reconnected!");
            Serial.print("WiFi: IP address: ");
            Serial.println(ipAddress);
            return true;
        }

        // Проверка таймаута
        if (millis() - reconnectStartTime > staConfig.connectionTimeout) {
            Serial.println("\nWiFi: Reconnection timeout");
            status = WiFiStatus::FAILED;
            return false;
        }

        // Проверка на ошибку
        if (WiFi.status() == WL_CONNECT_FAILED) {
            Serial.println("\nWiFi: Reconnection failed");
            status = WiFiStatus::FAILED;
            return false;
        }

        // Всё ещё в процессе
        return false;
    }

    /**
     * @brief Полное переподключение (блокирующее, для совместимости)
     * @return true если успешно
     * @deprecated Используйте reconnect() + processReconnect()
     */
    bool reconnectBlocking() {
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

        if (apModeEnabled) {
            info += " | AP: ";
            info += apConfig.ssid;
            info += " @ ";
            info += apIpAddress.toString();
            info += " Clients: ";
            info += WiFi.softAPgetStationNum();
        }

        return info;
    }
};

} // namespace HAL

#endif // BANYA_HAL_WIFI_H
