#include "OTA.h"

namespace HAL {

OTAManager::OTAManager(const OTAConfig& config) : config(config) {
}

bool OTAManager::begin() {
    if (initialized) {
        return true;
    }

    // Set hostname if provided
    if (config.hostname != nullptr) {
        ArduinoOTA.setHostname(config.hostname);
    }

    // Set port if not default
    if (config.port != 3232) {
        ArduinoOTA.setPort(config.port);
    }

    // Set password if provided
    if (config.password != nullptr) {
        ArduinoOTA.setPassword(config.password);
    }

    // Setup callbacks
    setupCallbacks();

    // Start OTA service
    ArduinoOTA.begin();

    initialized = true;
    status = OTAStatus::IDLE;

    if (config.enableDebug) {
        Serial.println("OTA: Initialized");
        Serial.print("OTA: Hostname: ");
        Serial.println(config.hostname ? config.hostname : ArduinoOTA.getHostname().c_str());
        Serial.print("OTA: Port: ");
        Serial.println(config.port);
        Serial.print("OTA: Password: ");
        Serial.println(config.password ? "***" : "none");
    }

    return true;
}

void OTAManager::handleLoop() {
    if (!initialized) {
        return;
    }

    ArduinoOTA.handle();
}

const char* OTAManager::getStatusString() const {
    switch (status) {
        case OTAStatus::IDLE:
            return "Idle";
        case OTAStatus::RUNNING:
            return "Updating";
        case OTAStatus::SUCCESS:
            return "Success";
        case OTAStatus::ERROR_GENERAL:
            return "Error: General";
        case OTAStatus::ERROR_AUTH:
            return "Error: Auth";
        case OTAStatus::ERROR_BEGIN:
            return "Error: Begin";
        case OTAStatus::ERROR_END:
            return "Error: End";
        case OTAStatus::ERROR_ABORT:
            return "Error: Aborted";
        case OTAStatus::ERROR_SPIFLASH:
            return "Error: Flash";
        default:
            return "Unknown";
    }
}

String OTAManager::getInfo() const {
    String info = "OTA: ";
    info += getStatusString();
    info += " (";
    info += config.hostname ? config.hostname : ArduinoOTA.getHostname().c_str();
    info += ":";
    info += String(config.port);
    info += ")";

    if (isRunning()) {
        info += " Progress: ";
        info += String(progress);
        info += "%";
    }

    return info;
}

void OTAManager::onStart(std::function<void()> callback) {
    onStartCallback = callback;
}

void OTAManager::onEnd(std::function<void()> callback) {
    onEndCallback = callback;
}

void OTAManager::onProgress(std::function<void(unsigned int, unsigned int)> callback) {
    onProgressCallback = callback;
}

void OTAManager::onError(std::function<void(ota_error_t)> callback) {
    onErrorCallback = callback;
}

void OTAManager::setupCallbacks() {
    // Capture this for access to config in lambdas
    auto configPtr = &config;

    // Start callback
    ArduinoOTA.onStart([]() {
        Serial.println("OTA: Update started");
    });

    // End callback
    ArduinoOTA.onEnd([]() {
        Serial.println("OTA: Update completed");
    });

    // Progress callback
    ArduinoOTA.onProgress([configPtr](unsigned int progress, unsigned int total) {
        if (configPtr->enableProgress) {
            uint8_t percent = (progress / (total / 100));
            Serial.printf("OTA: Progress: %u%%\r", percent);
        }
    });

    // Error callback
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("OTA: Error [%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
            Serial.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
            Serial.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
            Serial.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
            Serial.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
            Serial.println("End Failed");
        }
    });
}

void OTAManager::handleError(ota_error_t error) {
    switch (error) {
        case OTA_AUTH_ERROR:
            status = OTAStatus::ERROR_AUTH;
            break;
        case OTA_BEGIN_ERROR:
            status = OTAStatus::ERROR_BEGIN;
            break;
        case OTA_CONNECT_ERROR:
            status = OTAStatus::ERROR_GENERAL;
            break;
        case OTA_RECEIVE_ERROR:
            status = OTAStatus::ERROR_GENERAL;
            break;
        case OTA_END_ERROR:
            status = OTAStatus::ERROR_END;
            break;
        default:
            status = OTAStatus::ERROR_GENERAL;
            break;
    }

    if (onErrorCallback) {
        onErrorCallback(error);
    }

    // Reset to idle after error
    status = OTAStatus::IDLE;
}

} // namespace HAL
