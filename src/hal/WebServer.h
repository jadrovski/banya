#ifndef BANYA_HAL_WEBSERVER_H
#define BANYA_HAL_WEBSERVER_H

#include <Arduino.h>
#include <WebServer.h>
#include "WiFi.h"

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
    String mode;
    
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
 */
class BanyaWebServer {
private:
    WebServerConfig config;
    std::unique_ptr<WebServer> server;
    std::function<BanyaStatus()> statusProvider;
    std::function<RGB()> getLedColor;
    std::function<void(RGB)> setLedColor;
    bool running;

public:
    /**
     * @brief Конструктор веб-сервера
     * @param cfg Конфигурация
     */
    explicit BanyaWebServer(const WebServerConfig& cfg = WebServerConfig())
        : config(cfg), server(nullptr), statusProvider(nullptr), getLedColor(nullptr), setLedColor(nullptr), running(false) {}

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
     * @brief Установить функции для управления LED
     * @param getColor Функция получения текущего цвета
     * @param setColor Функция установки цвета
     */
    void setLEDControl(std::function<RGB()> getColor, std::function<void(RGB)> setColor) {
        getLedColor = getColor;
        setLedColor = setColor;
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
            json += "\"wifi\":\"" + status.wifiIP + "\",";
            json += "\"mode\":\"" + status.mode + "\"";
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
        if (getLedColor) {
            RGB color = getLedColor();
            String json = "{";
            json += "\"r\":" + String(color.red) + ",";
            json += "\"g\":" + String(color.green) + ",";
            json += "\"b\":" + String(color.blue);
            json += "}";
            server->send(200, "application/json", json);
        } else {
            server->send(500, "application/json", "{\"error\":\"No LED control\"}");
        }
    }

    /**
     * @brief Обработка запроса установки LED цвета
     */
    void handleLEDSet() {
        if (setLedColor && server->hasArg("r") && server->hasArg("g") && server->hasArg("b")) {
            uint8_t r = server->arg("r").toInt();
            uint8_t g = server->arg("g").toInt();
            uint8_t b = server->arg("b").toInt();
            setLedColor(RGB(r, g, b));
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
            <span id="mode-display">Mode: --</span>
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
        
        <div class="footer">
            <p>Last update: <span id="last-update">--:--:--</span></p>
            <p>Auto-refresh: <span id="refresh-status">ON</span></p>
            <p><a href="/led" class="nav-link">🎨 LED Settings</a></p>
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
                    document.getElementById('mode-display').textContent = 'Mode: ' + data.mode;
                    
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

#wifi-status, #mode-display {
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

        .nav-link {
            color: #00d9ff;
            text-decoration: none;
            transition: color 0.3s;
        }

        .nav-link:hover {
            color: #00ff64;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🎨 LED Settings</h1>

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

            <div class="btn-group">
                <button class="btn btn-back" onclick="window.location.href='/'">⬅ Back</button>
                <button class="btn btn-off" onclick="setLED(0, 0, 0)">Turn Off</button>
            </div>
        </div>
    </div>

    <script>
        let currentR = 0, currentG = 0, currentB = 0;

        // Загрузка текущих значений
        function loadLEDStatus() {
            fetch('/led/status')
                .then(response => response.json())
                .then(data => {
                    currentR = data.r;
                    currentG = data.g;
                    currentB = data.b;

                    document.getElementById('redSlider').value = currentR;
                    document.getElementById('greenSlider').value = currentG;
                    document.getElementById('blueSlider').value = currentB;

                    document.getElementById('redValue').textContent = currentR;
                    document.getElementById('greenValue').textContent = currentG;
                    document.getElementById('blueValue').textContent = currentB;

                    updatePreview();
                })
                .catch(error => {
                    console.error('Error loading LED status:', error);
                });
        }

        // Обновление предпросмотра цвета
        function updatePreview() {
            const r = document.getElementById('redSlider').value;
            const g = document.getElementById('greenSlider').value;
            const b = document.getElementById('blueSlider').value;

            document.getElementById('redValue').textContent = r;
            document.getElementById('greenValue').textContent = g;
            document.getElementById('blueValue').textContent = b;

            document.getElementById('colorPreview').style.backgroundColor = 
                'rgb(' + r + ', ' + g + ', ' + b + ')';
        }

        // Установка цвета на сервер
        function setLED(r, g, b) {
            fetch('/led/set?r=' + r + '&g=' + g + '&b=' + b)
                .then(response => response.json())
                .then(data => {
                    if (data.success) {
                        currentR = r;
                        currentG = g;
                        currentB = b;
                    }
                })
                .catch(error => {
                    console.error('Error setting LED:', error);
                });
        }

        // Обработчики изменений слайдеров (применение цвета)
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

        // Загрузка при старте
        loadLEDStatus();
    </script>
</body>
</html>
)rawliteral";
        return html;
    }
};

} // namespace HAL

#endif // BANYA_HAL_WEBSERVER_H
