#ifndef BANYA_HAL_WEBSERVER_H
#define BANYA_HAL_WEBSERVER_H

#include <Arduino.h>
#include <WebServer.h>
#include "WiFi.h"
#include "WiFiSettings.h"
#include "RGBLED.h"

namespace HAL {

/**
 * @brief Конфигурация веб-сервера
 */
struct WebServerConfig {
    uint16_t port;        // Порт сервера (по умолчанию 80)

    WebServerConfig(uint16_t p = 80) : port(p) {}
};

/**
 * @brief Данные статуса для веб-интерфейса
 */
struct BanyaStatus {
    float temp1;          // Температура DS18B20 #1
    float temp2;          // Температура DS18B20 #2
    float temp3;          // Температура BME280
    float humidity;       // Влажность BME280
    float pressure;       // Давление BME280 (мм рт.ст.)
    bool sensor1Connected;
    bool sensor2Connected;
    String wifiIP;
    String wifiStatus;

    BanyaStatus() : temp1(0), temp2(0), temp3(0), humidity(0), pressure(0),
                   sensor1Connected(false), sensor2Connected(false) {}
};

/**
 * @brief Веб-сервер для Banya Controller
 *
 * Предоставляет:
 * - HTML страницу со статусом сауны
 * - JSON API для получения данных
 * - Автоматическое обновление данных
 * - WiFi конфигурационный портал
 */
class BanyaWebServer {
private:
    WebServerConfig config;
    std::unique_ptr<WebServer> server;
    std::function<BanyaStatus()> statusProvider;
    RGBLED* ledStrip;
    WiFiManager* wifiManager;
    WiFiSettings* wifiSettings;
    bool running;

public:
    /**
     * @brief Конструктор веб-сервера
     * @param cfg Конфигурация
     * @param led Указатель на RGBLED
     */
    explicit BanyaWebServer(const WebServerConfig& cfg = WebServerConfig(), RGBLED* led = nullptr)
        : config(cfg), server(nullptr), statusProvider(nullptr), ledStrip(led),
          wifiManager(nullptr), wifiSettings(nullptr),
          running(false) {}

    ~BanyaWebServer() {
        stop();
    }

    /**
     * @brief Инициализация сервера
     * @return true если успешно
     */
    bool begin() {
        server = std::unique_ptr<WebServer>(new WebServer(config.port));

        // Регистрация обработчиков
        server->on("/", std::bind(&BanyaWebServer::handleRoot, this));
        server->on("/status", std::bind(&BanyaWebServer::handleStatus, this));
        server->on("/style.css", std::bind(&BanyaWebServer::handleStyle, this));
        server->on("/led", std::bind(&BanyaWebServer::handleLEDPage, this));
        server->on("/led/status", std::bind(&BanyaWebServer::handleLEDStatus, this));
        server->on("/led/set", std::bind(&BanyaWebServer::handleLEDSet, this));
        server->on("/led/brightness", std::bind(&BanyaWebServer::handleLEDBrightness, this));

        // WiFi configuration endpoints
        server->on("/wifi", std::bind(&BanyaWebServer::handleWiFiPage, this));
        server->on("/wifi/save", std::bind(&BanyaWebServer::handleWiFiSave, this));
        server->on("/wifi/ap-enable", std::bind(&BanyaWebServer::handleEnableAP, this));
        server->on("/wifi/ap-disable", std::bind(&BanyaWebServer::handleDisableAP, this));
        server->on("/wifi/ap-status", std::bind(&BanyaWebServer::handleAPStatus, this));

        // System endpoints
        server->on("/reboot", std::bind(&BanyaWebServer::handleReboot, this));

        server->onNotFound(std::bind(&BanyaWebServer::handleNotFound, this));

        running = true;
        return true;
    }

    /**
     * @brief Запуск сервера
     */
    void start() {
        if (server) {
            server->begin();
            Serial.println("WebServer: Started on port " + String(config.port));
        }
    }

    /**
     * @brief Остановка сервера
     */
    void stop() {
        if (server) {
            server->stop();
            running = false;
            Serial.println("WebServer: Stopped");
        }
    }

    /**
     * @brief Обработка клиентских запросов (вызывать в loop)
     */
    void handleLoop() {
        if (server && running) {
            server->handleClient();
        }
    }

    /**
     * @brief Установить провайдер статуса
     * @param provider Функция, возвращающая BanyaStatus
     */
    void setStatusProvider(std::function<BanyaStatus()> provider) {
        statusProvider = provider;
    }

    /**
     * @brief Установить указатель на RGBLED
     * @param led Указатель на RGBLED
     */
    void setLEDStrip(RGBLED* led) {
        ledStrip = led;
    }

    /**
     * @brief Установить менеджер WiFi для конфигурации
     * @param manager Указатель на WiFiManager
     */
    void setWiFiManager(WiFiManager* manager) {
        wifiManager = manager;
    }

    /**
     * @brief Установить хранилище настроек WiFi
     * @param settings Указатель на WiFiSettings
     */
    void setWiFiSettings(WiFiSettings* settings) {
        wifiSettings = settings;
    }

    /**
     * @brief Проверка работает ли сервер
     */
    bool isRunning() const { return running; }

private:
    /**
     * @brief Обработка корневого запроса (HTML страница)
     */
    void handleRoot() {
        String html = getHTMLPage();
        server->send(200, "text/html", html);
    }

    /**
     * @brief Обработка запроса статуса (JSON)
     */
    void handleStatus() {
        if (statusProvider) {
            BanyaStatus status = statusProvider();

            String json = "{";
            json += "\"temp1\":" + String(status.temp1, 1) + ",";
            json += "\"temp2\":" + String(status.temp2, 1) + ",";
            json += "\"temp3\":" + String(status.temp3, 1) + ",";
            json += "\"humidity\":" + String(status.humidity, 1) + ",";
            json += "\"pressure\":" + String(status.pressure, 1) + ",";
            json += "\"sensor1\":" + String(status.sensor1Connected ? "true" : "false") + ",";
            json += "\"sensor2\":" + String(status.sensor2Connected ? "true" : "false") + ",";
            json += "\"wifi\":\"" + status.wifiIP + "\"";
            json += "}";

            server->send(200, "application/json", json);
        } else {
            server->send(500, "application/json", "{\"error\":\"No status provider\"}");
        }
    }

    /**
     * @brief Обработка запроса CSS
     */
    void handleStyle() {
        server->send(200, "text/css", getCSS());
    }

    /**
     * @brief Обработка страницы LED настроек
     */
    void handleLEDPage() {
        String html = getLEDPageHTML();
        server->send(200, "text/html", html);
    }

    /**
     * @brief Обработка запроса статуса LED
     */
    void handleLEDStatus() {
        if (!ledStrip) {
            server->send(500, "application/json", "{\"error\":\"No LED strip\"}");
            return;
        }
        
        RGB color = ledStrip->getCurrentColor();
        String json = "{";
        json += "\"r\":" + String(color.red) + ",";
        json += "\"g\":" + String(color.green) + ",";
        json += "\"b\":" + String(color.blue);
        json += ",\"brightness\":" + String(ledStrip->getBrightness(), 2);
        json += "}";
        server->send(200, "application/json", json);
    }

    /**
     * @brief Обработка запроса установки яркости LED
     */
    void handleLEDBrightness() {
        if (!ledStrip) {
            server->send(500, "application/json", "{\"error\":\"No LED strip\"}");
            return;
        }

        if (server->hasArg("brightness")) {
            float brightness = server->arg("brightness").toFloat();
            // Clamp brightness to 0.0-1.0 range
            if (brightness < 0.0f) brightness = 0.0f;
            if (brightness > 1.0f) brightness = 1.0f;
            ledStrip->setBrightness(brightness);
            server->send(200, "application/json", "{\"success\":true,\"brightness\":" + String(brightness, 2) + "}");
        } else {
            // Return current brightness
            float brightness = ledStrip->getBrightness();
            server->send(200, "application/json", "{\"brightness\":" + String(brightness, 2) + "}");
        }
    }

    /**
     * @brief Обработка запроса установки LED цвета
     */
    void handleLEDSet() {
        if (!ledStrip) {
            server->send(500, "application/json", "{\"error\":\"No LED strip\"}");
            return;
        }
        
        if (server->hasArg("r") && server->hasArg("g") && server->hasArg("b")) {
            uint8_t r = server->arg("r").toInt();
            uint8_t g = server->arg("g").toInt();
            uint8_t b = server->arg("b").toInt();
            ledStrip->setColor(RGB(r, g, b));
            server->send(200, "application/json", "{\"success\":true}");
        } else {
            server->send(400, "application/json", "{\"error\":\"Invalid request\"}");
        }
    }

    /**
     * @brief Обработка неизвестных запросов
     */
    void handleNotFound() {
        server->send(404, "text/plain", "Not Found");
    }

    /**
     * @brief Обработка страницы настройки WiFi
     */
    void handleWiFiPage() {
        String html = getWiFiPageHTML();
        server->send(200, "text/html", html);
    }

    /**
     * @brief Обработка сохранения WiFi настроек
     */
    void handleWiFiSave() {
        if (!wifiSettings) {
            server->send(500, "application/json", "{\"error\":\"No WiFi settings\"}");
            return;
        }

        if (server->hasArg("ssid") && server->hasArg("password")) {
            String ssid = server->arg("ssid");
            String password = server->arg("password");

            Serial.print("WiFi: Saving credentials - SSID: ");
            Serial.println(ssid);

            // Сохраняем в NVS
            if (wifiSettings->save(ssid, password)) {
                // Обновляем credentials в WiFiManager
                if (wifiManager) {
                    wifiManager->setCredentials(ssid.c_str(), password.c_str());
                }

                server->send(200, "application/json", "{\"success\":true,\"message\":\"Saved to NVS\"}");
            } else {
                server->send(500, "application/json", "{\"error\":\"Failed to save\"}");
            }
        } else {
            server->send(400, "application/json", "{\"error\":\"Missing ssid or password\"}");
        }
    }

    /**
     * @brief Обработка перезагрузки устройства
     */
    void handleReboot() {
        Serial.println("System: Reboot requested via web interface");
        server->send(200, "application/json", "{\"success\":true,\"message\":\"Rebooting...\"}");
        delay(100); // Даем время на отправку ответа
        ESP.restart();
    }

    /**
     * @brief Обработка включения AP режима
     */
    void handleEnableAP() {
        if (!wifiManager) {
            server->send(500, "application/json", "{\"error\":\"No WiFi manager\"}");
            return;
        }

        Serial.println("WiFi: Enabling AP mode...");
        
        if (wifiManager->enableAP()) {
            String json = "{\"success\":true,\"ip\":\"" + wifiManager->getAPIPAddressString() + "\"}";
            server->send(200, "application/json", json);
        } else {
            server->send(500, "application/json", "{\"error\":\"Failed to enable AP\"}");
        }
    }

    /**
     * @brief Обработка выключения AP режима
     */
    void handleDisableAP() {
        if (!wifiManager) {
            server->send(500, "application/json", "{\"error\":\"No WiFi manager\"}");
            return;
        }

        Serial.println("WiFi: Disabling AP mode...");

        if (wifiManager->disableAP()) {
            server->send(200, "application/json", "{\"success\":true}");
        } else {
            server->send(500, "application/json", "{\"error\":\"Failed to disable AP\"}");
        }
    }

    /**
     * @brief Обработка запроса статуса AP режима
     */
    void handleAPStatus() {
        if (!wifiManager) {
            server->send(500, "application/json", "{\"error\":\"No WiFi manager\"}");
            return;
        }

        bool apEnabled = wifiManager->isAPEnabled();
        String json = "{";
        json += "\"enabled\":" + String(apEnabled ? "true" : "false");
        if (apEnabled) {
            json += ",\"ip\":\"" + wifiManager->getAPIPAddressString() + "\"";
            json += ",\"ssid\":\"Banya-Ctl\"";
        }
        json += "}";
        server->send(200, "application/json", json);
    }

    /**
     * @brief Генерация HTML страницы
     */
    String getHTMLPage() {
        String html = R"rawliteral(
<!DOCTYPE html>
<html lang="ru">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Banya Controller</title>
    <link rel="stylesheet" href="/style.css">
</head>
<body>
    <div class="container">
        <h1>🧖 Banya Controller</h1>
        
        <div class="status-bar">
            <span id="wifi-status">WiFi: --</span>
        </div>
        
        <div class="sensors-grid">
            <div class="sensor-card">
                <h2>🌡️ T1</h2>
                <div class="value" id="temp1">--.-</div>
                <div class="unit">°C</div>
                <div class="status" id="sensor1-status">--</div>
            </div>
            
            <div class="sensor-card">
                <h2>🌡️ T2</h2>
                <div class="value" id="temp2">--.-</div>
                <div class="unit">°C</div>
                <div class="status" id="sensor2-status">--</div>
            </div>
            
            <div class="sensor-card">
                <h2>🌡️ T3</h2>
                <div class="value" id="temp3">--.-</div>
                <div class="unit">°C</div>
                <div class="status">BME280</div>
            </div>
            
            <div class="sensor-card">
                <h2>💧 Humidity</h2>
                <div class="value" id="humidity">--.-</div>
                <div class="unit">%</div>
                <div class="status">BME280</div>
            </div>
            
            <div class="sensor-card full-width">
                <h2>📊 Pressure</h2>
                <div class="value" id="pressure">----</div>
                <div class="unit">mmHg</div>
                <div class="status">BME280</div>
            </div>
        </div>
        
        <nav class="top-nav">
            <a href="/led" class="nav-item">
                <span class="nav-icon">🎨</span>
                <span class="nav-text">LED Settings</span>
            </a>
            <a href="/wifi" class="nav-item">
                <span class="nav-icon">📶</span>
                <span class="nav-text">WiFi Settings</span>
            </a>
        </nav>

        <div class="footer">
            <p>Last update: <span id="last-update">--:--:--</span></p>
            <p>Auto-refresh: <span id="refresh-status">ON</span></p>
        </div>
    </div>
    
    <script>
        function updateStatus() {
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('temp1').textContent = data.temp1.toFixed(1);
                    document.getElementById('temp2').textContent = data.temp2.toFixed(1);
                    document.getElementById('temp3').textContent = data.temp3.toFixed(1);
                    document.getElementById('humidity').textContent = data.humidity.toFixed(1);
                    document.getElementById('pressure').textContent = data.pressure.toFixed(0);
                    
                    document.getElementById('sensor1-status').textContent = data.sensor1 ? 'OK' : '---';
                    document.getElementById('sensor2-status').textContent = data.sensor2 ? 'OK' : '---';
                    document.getElementById('sensor1-status').className = 'status ' + (data.sensor1 ? 'ok' : 'error');
                    document.getElementById('sensor2-status').className = 'status ' + (data.sensor2 ? 'ok' : 'error');

                    document.getElementById('wifi-status').textContent = 'WiFi: ' + data.wifi;

                    const now = new Date();
                    document.getElementById('last-update').textContent = now.toLocaleTimeString();
                })
                .catch(error => {
                    console.error('Error fetching status:', error);
                    document.getElementById('refresh-status').textContent = 'OFF';
                    document.getElementById('refresh-status').style.color = 'red';
                });
        }
        
        // Обновление каждые 2 секунды
        setInterval(updateStatus, 2000);
        updateStatus(); // Первое обновление сразу
    </script>
</body>
</html>
)rawliteral";
        return html;
    }

    /**
     * @brief Генерация CSS стилей
     */
    String getCSS() {
        String css = R"rawliteral(
* {
    margin: 0;
    padding: 0;
    box-sizing: border-box;
}

body {
    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, sans-serif;
    background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
    min-height: 100vh;
    padding: 20px;
    color: #fff;
}

.container {
    max-width: 800px;
    margin: 0 auto;
}

h1 {
    text-align: center;
    margin-bottom: 20px;
    font-size: 2em;
    color: #00d9ff;
    text-shadow: 0 0 10px rgba(0, 217, 255, 0.5);
}

.status-bar {
    display: flex;
    justify-content: space-between;
    background: rgba(255, 255, 255, 0.1);
    padding: 15px 20px;
    border-radius: 10px;
    margin-bottom: 20px;
    backdrop-filter: blur(10px);
}

#wifi-status {
    font-size: 0.9em;
    color: #aaa;
}

.sensors-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
    gap: 15px;
    margin-bottom: 20px;
}

.sensor-card {
    background: rgba(255, 255, 255, 0.1);
    border-radius: 15px;
    padding: 20px;
    text-align: center;
    backdrop-filter: blur(10px);
    border: 1px solid rgba(255, 255, 255, 0.1);
    transition: transform 0.3s ease, box-shadow 0.3s ease;
}

.sensor-card:hover {
    transform: translateY(-5px);
    box-shadow: 0 10px 30px rgba(0, 217, 255, 0.2);
}

.sensor-card.full-width {
    grid-column: 1 / -1;
}

.sensor-card h2 {
    font-size: 1.2em;
    margin-bottom: 15px;
    color: #00d9ff;
}

.sensor-card .value {
    font-size: 2.5em;
    font-weight: bold;
    color: #fff;
    text-shadow: 0 0 20px rgba(255, 255, 255, 0.3);
}

.sensor-card .unit {
    font-size: 1.2em;
    color: #888;
    margin-top: 5px;
}

.sensor-card .status {
    font-size: 0.8em;
    color: #666;
    margin-top: 10px;
    padding: 3px 10px;
    border-radius: 5px;
    display: inline-block;
}

.sensor-card .status.ok {
    background: rgba(0, 255, 100, 0.2);
    color: #00ff64;
}

.sensor-card .status.error {
    background: rgba(255, 50, 50, 0.2);
    color: #ff3232;
}

/* Top Navigation Menu */
.top-nav {
    display: flex;
    justify-content: center;
    gap: 15px;
    margin-bottom: 20px;
    padding: 10px;
    background: rgba(255, 255, 255, 0.05);
    border-radius: 15px;
    backdrop-filter: blur(10px);
    border: 1px solid rgba(255, 255, 255, 0.1);
}

.nav-item {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 12px 25px;
    background: linear-gradient(135deg, rgba(0, 217, 255, 0.15), rgba(0, 255, 100, 0.15));
    border-radius: 12px;
    text-decoration: none;
    color: #fff;
    font-weight: 600;
    font-size: 0.95em;
    transition: all 0.3s ease;
    border: 1px solid rgba(255, 255, 255, 0.1);
    box-shadow: 0 4px 15px rgba(0, 0, 0, 0.2);
}

.nav-item:hover {
    transform: translateY(-3px);
    background: linear-gradient(135deg, rgba(0, 217, 255, 0.3), rgba(0, 255, 100, 0.3));
    box-shadow: 0 8px 25px rgba(0, 217, 255, 0.3);
    border-color: rgba(0, 217, 255, 0.5);
}

.nav-item:active {
    transform: translateY(-1px);
}

.nav-icon {
    font-size: 1.3em;
    filter: drop-shadow(0 2px 4px rgba(0, 0, 0, 0.3));
}

.nav-text {
    letter-spacing: 0.3px;
}

.footer {
    text-align: center;
    color: #666;
    font-size: 0.85em;
    margin-top: 30px;
}

.footer p {
    margin: 5px 0;
}

#refresh-status {
    font-weight: bold;
    color: #00ff64;
}

@media (max-width: 600px) {
    h1 {
        font-size: 1.5em;
    }
    
    .sensor-card .value {
        font-size: 2em;
    }
    
    .status-bar {
        flex-direction: column;
        gap: 10px;
        text-align: center;
    }
}
)rawliteral";
        return css;
    }

    /**
     * @brief Генерация HTML страницы LED настроек
     */
    String getLEDPageHTML() {
        String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>LED Settings - Banya Controller</title>
    <link rel="stylesheet" href="/style.css">
    <style>
        .led-controls {
            background: rgba(255, 255, 255, 0.1);
            border-radius: 15px;
            padding: 30px;
            margin-bottom: 20px;
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255, 255, 255, 0.1);
        }

        .slider-group {
            margin-bottom: 25px;
        }

        .slider-group label {
            display: block;
            font-size: 1.1em;
            margin-bottom: 10px;
            color: #00d9ff;
        }

        .slider-group input[type="range"] {
            width: 100%;
            height: 10px;
            border-radius: 5px;
            outline: none;
            -webkit-appearance: none;
        }

        .slider-group input[type="range"]::-webkit-slider-thumb {
            -webkit-appearance: none;
            width: 25px;
            height: 25px;
            border-radius: 50%;
            cursor: pointer;
            box-shadow: 0 0 10px rgba(0, 0, 0, 0.3);
        }

        .slider-group input[type="range"]::-webkit-slider-runnable-track {
            background: rgba(255, 255, 255, 0.2);
            border-radius: 5px;
        }

        .slider-group.red input[type="range"]::-webkit-slider-thumb {
            background: #ff3232;
        }

        .slider-group.green input[type="range"]::-webkit-slider-thumb {
            background: #00ff64;
        }

        .slider-group.blue input[type="range"]::-webkit-slider-thumb {
            background: #00d9ff;
        }

        .slider-group.brightness input[type="range"]::-webkit-slider-thumb {
            background: linear-gradient(135deg, #ff9900, #ff6600);
        }

        .slider-value {
            text-align: right;
            font-size: 1.5em;
            font-weight: bold;
            color: #fff;
            margin-top: 5px;
            text-shadow: 0 0 10px rgba(255, 255, 255, 0.3);
        }

        .color-preview {
            width: 100%;
            height: 100px;
            border-radius: 15px;
            margin-bottom: 20px;
            box-shadow: 0 5px 20px rgba(0, 0, 0, 0.3);
            border: 2px solid rgba(255, 255, 255, 0.2);
        }

        .btn-group {
            display: flex;
            gap: 15px;
            margin-top: 20px;
        }

        .btn {
            flex: 1;
            padding: 15px 20px;
            border: none;
            border-radius: 10px;
            font-size: 1em;
            font-weight: bold;
            cursor: pointer;
            transition: transform 0.2s, box-shadow 0.2s;
        }

        .btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(0, 0, 0, 0.3);
        }

        .btn:active {
            transform: translateY(0);
        }

        .btn-save {
            background: linear-gradient(135deg, #00d9ff, #00ff64);
            color: #1a1a2e;
        }

        .btn-off {
            background: rgba(255, 50, 50, 0.3);
            color: #ff3232;
            border: 1px solid rgba(255, 50, 50, 0.5);
        }

        .btn-back {
            background: rgba(255, 255, 255, 0.1);
            color: #fff;
            border: 1px solid rgba(255, 255, 255, 0.2);
        }

        .top-nav {
            display: flex;
            justify-content: center;
            gap: 15px;
            margin-bottom: 20px;
            padding: 10px;
            background: rgba(255, 255, 255, 0.05);
            border-radius: 15px;
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255, 255, 255, 0.1);
        }

        .nav-item {
            display: flex;
            align-items: center;
            gap: 8px;
            padding: 12px 25px;
            background: linear-gradient(135deg, rgba(0, 217, 255, 0.15), rgba(0, 255, 100, 0.15));
            border-radius: 12px;
            text-decoration: none;
            color: #fff;
            font-weight: 600;
            font-size: 0.95em;
            transition: all 0.3s ease;
            border: 1px solid rgba(255, 255, 255, 0.1);
            box-shadow: 0 4px 15px rgba(0, 0, 0, 0.2);
        }

        .nav-item:hover {
            transform: translateY(-3px);
            background: linear-gradient(135deg, rgba(0, 217, 255, 0.3), rgba(0, 255, 100, 0.3));
            box-shadow: 0 8px 25px rgba(0, 217, 255, 0.3);
            border-color: rgba(0, 217, 255, 0.5);
        }

        .nav-item:active {
            transform: translateY(-1px);
        }

        .nav-icon {
            font-size: 1.3em;
            filter: drop-shadow(0 2px 4px rgba(0, 0, 0, 0.3));
        }

        .nav-text {
            letter-spacing: 0.3px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🎨 LED Settings</h1>

        <nav class="top-nav">
            <a href="/" class="nav-item">
                <span class="nav-icon">⬅️</span>
                <span class="nav-text">Back</span>
            </a>
            <a href="/wifi" class="nav-item">
                <span class="nav-icon">📶</span>
                <span class="nav-text">WiFi Settings</span>
            </a>
        </nav>

        <div class="color-preview" id="colorPreview"></div>

        <div class="led-controls">
            <div class="slider-group red">
                <label>🔴 Red</label>
                <input type="range" id="redSlider" min="0" max="255" value="0">
                <div class="slider-value" id="redValue">0</div>
            </div>

            <div class="slider-group green">
                <label>🟢 Green</label>
                <input type="range" id="greenSlider" min="0" max="255" value="0">
                <div class="slider-value" id="greenValue">0</div>
            </div>

            <div class="slider-group blue">
                <label>🔵 Blue</label>
                <input type="range" id="blueSlider" min="0" max="255" value="0">
                <div class="slider-value" id="blueValue">0</div>
            </div>

            <div class="slider-group brightness">
                <label>💡 Brightness</label>
                <input type="range" id="brightnessSlider" min="0" max="100" value="50">
                <div class="slider-value" id="brightnessValue">50%</div>
            </div>

            <div class="btn-group">
                <button class="btn btn-back" onclick="window.location.href='/'">⬅ Back</button>
                <button class="btn btn-off" onclick="setLED(0, 0, 0)">Turn Off</button>
            </div>
        </div>
    </div>

    <script>
        let currentR = 0, currentG = 0, currentB = 0;
        let currentBrightness = 0.5;
        let pendingUpdate = null;      // Pending update data
        let isRequestInProgress = false;
        const MIN_UPDATE_INTERVAL = 80; // ms - minimum time between updates

        // Загрузка текущих значений
        function loadLEDStatus() {
            fetch('/led/status')
                .then(response => response.json())
                .then(data => {
                    currentR = data.r;
                    currentG = data.g;
                    currentB = data.b;
                    if (data.brightness !== undefined) {
                        currentBrightness = data.brightness;
                    }

                    document.getElementById('redSlider').value = currentR;
                    document.getElementById('greenSlider').value = currentG;
                    document.getElementById('blueSlider').value = currentB;
                    document.getElementById('brightnessSlider').value = Math.round(currentBrightness * 100);

                    document.getElementById('redValue').textContent = currentR;
                    document.getElementById('greenValue').textContent = currentG;
                    document.getElementById('blueValue').textContent = currentB;
                    document.getElementById('brightnessValue').textContent = Math.round(currentBrightness * 100) + '%';

                    updatePreview();
                })
                .catch(error => {
                    console.error('Error loading LED status:', error);
                });
        }

        // Обновление предпросмотра цвета (мгновенно, без запроса к серверу)
        function updatePreview() {
            const r = document.getElementById('redSlider').value;
            const g = document.getElementById('greenSlider').value;
            const b = document.getElementById('blueSlider').value;
            const brightness = document.getElementById('brightnessSlider').value / 100;

            document.getElementById('redValue').textContent = r;
            document.getElementById('greenValue').textContent = g;
            document.getElementById('blueValue').textContent = b;
            document.getElementById('brightnessValue').textContent = Math.round(brightness * 100) + '%';

            // Apply brightness to preview (darken the color)
            const adjustedR = Math.round(r * brightness);
            const adjustedG = Math.round(g * brightness);
            const adjustedB = Math.round(b * brightness);

            document.getElementById('colorPreview').style.backgroundColor =
                'rgb(' + adjustedR + ', ' + adjustedG + ', ' + adjustedB + ')';
        }

        // Process pending update (throttled)
        function processPendingUpdate() {
            if (!pendingUpdate || isRequestInProgress) {
                return;
            }

            isRequestInProgress = true;
            const update = pendingUpdate;
            pendingUpdate = null;

            fetch(update.url)
                .then(response => response.json())
                .then(data => {
                    if (update.type === 'color' && data.success) {
                        currentR = update.r;
                        currentG = update.g;
                        currentB = update.b;
                    } else if (update.type === 'brightness' && data.success) {
                        currentBrightness = data.brightness;
                    }
                })
                .catch(error => {
                    console.error('Error updating LED:', error);
                })
                .finally(() => {
                    isRequestInProgress = false;
                    // Process any pending update that arrived during the request
                    if (pendingUpdate) {
                        setTimeout(processPendingUpdate, MIN_UPDATE_INTERVAL);
                    }
                });
        }

        // Установка цвета на сервер (throttled - sends after previous completes)
        function setLED(r, g, b) {
            pendingUpdate = {
                type: 'color',
                url: '/led/set?r=' + r + '&g=' + g + '&b=' + b,
                r: r,
                g: g,
                b: b
            };
            
            if (!isRequestInProgress) {
                setTimeout(processPendingUpdate, MIN_UPDATE_INTERVAL);
            }
        }

        // Установка яркости (throttled - sends after previous completes)
        function setBrightness(brightnessPercent) {
            pendingUpdate = {
                type: 'brightness',
                url: '/led/brightness?brightness=' + (brightnessPercent / 100)
            };
            
            if (!isRequestInProgress) {
                setTimeout(processPendingUpdate, MIN_UPDATE_INTERVAL);
            }
        }

        // Обработчики изменений слайдеров:
        // - input: обновляем предпросмотр + отправляем запрос (с throttle)
        document.getElementById('redSlider').addEventListener('input', function() {
            updatePreview();
            setLED(this.value, document.getElementById('greenSlider').value, document.getElementById('blueSlider').value);
        });
        
        document.getElementById('greenSlider').addEventListener('input', function() {
            updatePreview();
            setLED(document.getElementById('redSlider').value, this.value, document.getElementById('blueSlider').value);
        });
        
        document.getElementById('blueSlider').addEventListener('input', function() {
            updatePreview();
            setLED(document.getElementById('redSlider').value, document.getElementById('greenSlider').value, this.value);
        });
        
        document.getElementById('brightnessSlider').addEventListener('input', function() {
            updatePreview();
            setBrightness(this.value);
        });

        // Загрузка при старте
        loadLEDStatus();
    </script>
</body>
</html>
)rawliteral";
        return html;
    }

    /**
     * @brief Генерация HTML страницы настройки WiFi
     */
    String getWiFiPageHTML() {
        String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>WiFi Settings - Banya Controller</title>
    <link rel="stylesheet" href="/style.css">
    <style>
        .wifi-controls {
            background: rgba(255, 255, 255, 0.1);
            border-radius: 15px;
            padding: 30px;
            margin-bottom: 20px;
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255, 255, 255, 0.1);
        }

        .form-group {
            margin-bottom: 25px;
        }

        .form-group label {
            display: block;
            font-size: 1.1em;
            margin-bottom: 10px;
            color: #00d9ff;
        }

        .form-group input[type="text"],
        .form-group input[type="password"] {
            width: 100%;
            padding: 12px 15px;
            border-radius: 8px;
            border: 2px solid rgba(255, 255, 255, 0.2);
            background: rgba(255, 255, 255, 0.1);
            color: #fff;
            font-size: 1em;
            outline: none;
            transition: border-color 0.3s;
        }

        .form-group input:focus {
            border-color: #00d9ff;
        }

        .form-group input::placeholder {
            color: #666;
        }

        .network-list {
            max-height: 300px;
            overflow-y: auto;
            margin-bottom: 20px;
            background: rgba(255, 255, 255, 0.05);
            border-radius: 10px;
            padding: 10px;
        }

        .network-item {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 12px 15px;
            margin: 5px 0;
            background: rgba(255, 255, 255, 0.1);
            border-radius: 8px;
            cursor: pointer;
            transition: background 0.3s;
        }

        .network-item:hover {
            background: rgba(0, 217, 255, 0.2);
        }

        .network-ssid {
            font-weight: bold;
            color: #fff;
        }

        .network-rssi {
            font-size: 0.85em;
            color: #aaa;
        }

        .network-secure {
            color: #00ff64;
            margin-left: 10px;
        }

        .network-open {
            color: #ff3232;
            margin-left: 10px;
        }

        .btn-group {
            display: flex;
            gap: 15px;
            margin-top: 20px;
            flex-wrap: wrap;
        }

        .btn {
            flex: 1;
            min-width: 120px;
            padding: 15px 20px;
            border: none;
            border-radius: 10px;
            font-size: 1em;
            font-weight: bold;
            cursor: pointer;
            transition: transform 0.2s, box-shadow 0.2s;
        }

        .btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(0, 0, 0, 0.3);
        }

        .btn:active {
            transform: translateY(0);
        }

        .btn-save {
            background: linear-gradient(135deg, #00d9ff, #00ff64);
            color: #1a1a2e;
        }

        .btn-ap {
            background: linear-gradient(135deg, #ff9900, #ff6600);
            color: #fff;
        }

        .btn-back {
            background: rgba(255, 255, 255, 0.1);
            color: #fff;
            border: 1px solid rgba(255, 255, 255, 0.2);
        }

        .status-message {
            padding: 15px;
            border-radius: 10px;
            margin-bottom: 20px;
            text-align: center;
            font-weight: bold;
        }

        .status-message.success {
            background: rgba(0, 255, 100, 0.2);
            color: #00ff64;
        }

        .status-message.error {
            background: rgba(255, 50, 50, 0.2);
            color: #ff3232;
        }

        .status-message.info {
            background: rgba(0, 217, 255, 0.2);
            color: #00d9ff;
        }

        .checkbox-group {
            display: flex;
            align-items: center;
            gap: 10px;
            margin-top: 10px;
        }

        .checkbox-group input[type="checkbox"] {
            width: 20px;
            height: 20px;
            cursor: pointer;
        }

        .checkbox-group label {
            margin: 0;
            cursor: pointer;
            color: #aaa;
        }

        .ap-status {
            text-align: center;
            padding: 15px;
            margin-bottom: 20px;
            border-radius: 10px;
            background: rgba(255, 153, 0, 0.2);
            color: #ff9900;
            border: 1px solid rgba(255, 153, 0, 0.5);
        }

        .top-nav {
            display: flex;
            justify-content: center;
            gap: 15px;
            margin-bottom: 20px;
            padding: 10px;
            background: rgba(255, 255, 255, 0.05);
            border-radius: 15px;
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255, 255, 255, 0.1);
        }

        .nav-item {
            display: flex;
            align-items: center;
            gap: 8px;
            padding: 12px 25px;
            background: linear-gradient(135deg, rgba(0, 217, 255, 0.15), rgba(0, 255, 100, 0.15));
            border-radius: 12px;
            text-decoration: none;
            color: #fff;
            font-weight: 600;
            font-size: 0.95em;
            transition: all 0.3s ease;
            border: 1px solid rgba(255, 255, 255, 0.1);
            box-shadow: 0 4px 15px rgba(0, 0, 0, 0.2);
        }

        .nav-item:hover {
            transform: translateY(-3px);
            background: linear-gradient(135deg, rgba(0, 217, 255, 0.3), rgba(0, 255, 100, 0.3));
            box-shadow: 0 8px 25px rgba(0, 217, 255, 0.3);
            border-color: rgba(0, 217, 255, 0.5);
        }

        .nav-item:active {
            transform: translateY(-1px);
        }

        .nav-icon {
            font-size: 1.3em;
            filter: drop-shadow(0 2px 4px rgba(0, 0, 0, 0.3));
        }

        .nav-text {
            letter-spacing: 0.3px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>📶 WiFi Settings</h1>

        <nav class="top-nav">
            <a href="/" class="nav-item">
                <span class="nav-icon">⬅️</span>
                <span class="nav-text">Back</span>
            </a>
            <a href="/led" class="nav-item">
                <span class="nav-icon">🎨</span>
                <span class="nav-text">LED Settings</span>
            </a>
        </nav>

        <div id="apStatus" class="ap-status" style="display:none;">
            📡 AP Mode Active - Connect to: <strong id="apSSID">Banya-Ctl</strong>
            <br>IP: <strong id="apIP">192.168.4.1</strong>
        </div>

        <div class="wifi-controls">
            <div class="form-group">
                <label>📝 Network Name (SSID)</label>
                <input type="text" id="ssidInput" placeholder="Enter SSID manually or select from list">
            </div>

            <div class="form-group">
                <label>🔐 Password</label>
                <input type="password" id="passwordInput" placeholder="Enter password">
            </div>

            <div id="statusMessage" class="status-message" style="display:none;"></div>

            <div class="btn-group">
                <button class="btn btn-back" onclick="window.location.href='/'">⬅ Back</button>
                <button class="btn btn-ap" id="apToggleBtn" onclick="toggleAP()">📡 Enable AP Mode</button>
                <button class="btn btn-save" onclick="saveWiFi()">💾 Save & Reboot</button>
            </div>
        </div>
    </div>

    <script>
        let apEnabled = false;

        // Загрузка статуса AP с сервера
        function loadAPStatus() {
            fetch('/wifi/ap-status')
                .then(response => response.json())
                .then(data => {
                    apEnabled = data.enabled;
                    updateAPUI();
                })
                .catch(error => {
                    console.error('Error loading AP status:', error);
                });
        }

        // Обновление UI на основе статуса AP
        function updateAPUI() {
            const btn = document.getElementById('apToggleBtn');
            const apStatus = document.getElementById('apStatus');

            if (apEnabled) {
                btn.textContent = '📡 Disable AP Mode';
                apStatus.style.display = 'block';
            } else {
                btn.textContent = '📡 Enable AP Mode';
                apStatus.style.display = 'none';
            }
        }

        // Показать сообщение о статусе
        function showStatus(message, type) {
            const msgEl = document.getElementById('statusMessage');
            msgEl.textContent = message;
            msgEl.className = 'status-message ' + type;
            msgEl.style.display = 'block';
            setTimeout(() => { msgEl.style.display = 'none'; }, 5000);
        }

        // Переключение AP режима
        function toggleAP() {
            const btn = document.getElementById('apToggleBtn');
            const apStatus = document.getElementById('apStatus');

            if (apEnabled) {
                // Выключаем AP
                fetch('/wifi/ap-disable')
                    .then(response => response.json())
                    .then(data => {
                        if (data.success) {
                            apEnabled = false;
                            updateAPUI();
                            showStatus('AP Mode disabled', 'info');
                        } else {
                            showStatus('Failed to disable AP', 'error');
                        }
                    })
                    .catch(error => {
                        showStatus('Error: ' + error, 'error');
                    });
            } else {
                // Включаем AP
                fetch('/wifi/ap-enable')
                    .then(response => response.json())
                    .then(data => {
                        if (data.success) {
                            apEnabled = true;
                            document.getElementById('apIP').textContent = data.ip;
                            updateAPUI();
                            showStatus('AP Mode enabled. Connect to Banya-Ctl network.', 'success');
                        } else {
                            showStatus('Failed to enable AP', 'error');
                        }
                    })
                    .catch(error => {
                        showStatus('Error: ' + error, 'error');
                    });
            }
        }

        // Сохранение WiFi настроек
        function saveWiFi() {
            const ssid = document.getElementById('ssidInput').value.trim();
            const password = document.getElementById('passwordInput').value;

            if (!ssid) {
                showStatus('Please enter SSID', 'error');
                return;
            }

            const params = new URLSearchParams();
            params.append('ssid', ssid);
            params.append('password', password);

            fetch('/wifi/save', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded',
                },
                body: params.toString()
            })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    showStatus('Saved! Rebooting...', 'success');
                    // Reboot the device after saving
                    setTimeout(() => {
                        fetch('/reboot')
                            .then(() => {
                                showStatus('Device is rebooting...', 'info');
                            })
                            .catch(error => {
                                // Reboot may have interrupted the response, this is expected
                                showStatus('Saved! Device is rebooting...', 'success');
                            });
                    }, 1000);
                } else {
                    showStatus('Failed to save: ' + (data.error || 'Unknown error'), 'error');
                }
            })
            .catch(error => {
                showStatus('Error: ' + error, 'error');
            });
        }

        // Загрузка статуса при старте
        window.onload = function() {
            loadAPStatus();
        };
    </script>
</body>
</html>
)rawliteral";
        return html;
    }
};

} // namespace HAL

#endif // BANYA_HAL_WEBSERVER_H
