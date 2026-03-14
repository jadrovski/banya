#ifndef BANYA_OTA_H
#define BANYA_OTA_H

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <functional>
#include "OTAPresenter.h"

namespace HAL {

/**
 * @brief OTA update status enumeration
 */
enum class OTAStatus {
    IDLE,           ///< No OTA in progress
    RUNNING,        ///< OTA update in progress
    SUCCESS,        ///< OTA update completed successfully
    ERROR_GENERAL,  ///< General OTA error
    ERROR_AUTH,     ///< Authentication error
    ERROR_BEGIN,    ///< Error starting OTA
    ERROR_END,      ///< Error ending OTA
    ERROR_ABORT,    ///< OTA aborted
    ERROR_SPIFLASH  ///< SPI flash error
};

/**
 * @brief OTA Manager configuration
 */
struct OTAConfig {
    const char* hostname = nullptr;     ///< OTA hostname (nullptr = use default)
    const char* password = nullptr;     ///< OTA password (nullptr = no auth)
    uint16_t port = 3232;               ///< OTA port (default: 3232)
    bool enableProgress = true;         ///< Enable progress callbacks
    bool enableDebug = false;           ///< Enable debug output
};

/**
 * @brief OTA Manager for ESP32
 * 
 * Handles Over-The-Air firmware updates using ArduinoOTA.
 * Supports progress tracking, error handling, and callbacks.
 */
class OTAManager {
public:
    /**
     * @brief Construct a new OTA Manager
     * @param config OTA configuration
     */
    explicit OTAManager(const OTAConfig& config = OTAConfig());

    /**
     * @brief Initialize OTA manager
     * @return true if successful
     */
    bool begin();

    /**
     * @brief Handle OTA client connections (call in loop)
     */
    void handleLoop();

    /**
     * @brief Get current OTA status
     * @return OTAStatus
     */
    OTAStatus getStatus() const { return status; }

    /**
     * @brief Get status as human-readable string
     * @return Status string
     */
    const char* getStatusString() const;

    /**
     * @brief Get progress percentage (0-100)
     * @return Progress percentage
     */
    uint8_t getProgress() const { return progress; }

    /**
     * @brief Check if OTA is currently running
     * @return true if OTA in progress
     */
    bool isRunning() const { return status == OTAStatus::RUNNING; }

    /**
     * @brief Get diagnostic information
     * @return Info string
     */
    String getInfo() const;

    /**
     * @brief Set callback for OTA start
     * @param callback Callback function
     */
    void onStart(std::function<void()> callback);

    /**
     * @brief Set callback for OTA end
     * @param callback Callback function
     */
    void onEnd(std::function<void()> callback);

    /**
     * @brief Set callback for OTA progress
     * @param callback Callback function (progress, total)
     */
    void onProgress(std::function<void(unsigned int, unsigned int)> callback);

    /**
     * @brief Set callback for OTA error
     * @param callback Callback function (error code)
     */
    void onError(std::function<void(ota_error_t)> callback);

    /**
     * @brief Set presenter for displaying OTA status
     * @param presenter Pointer to OTAPresenter implementation
     */
    void setPresenter(OTAPresenter* presenter);

private:
    OTAConfig config;
    OTAStatus status = OTAStatus::IDLE;
    uint8_t progress = 0;
    bool initialized = false;
    OTAPresenter* presenter = nullptr;  // Presenter for OTA status display

    // Callbacks
    std::function<void()> onStartCallback = nullptr;
    std::function<void()> onEndCallback = nullptr;
    std::function<void(unsigned int, unsigned int)> onProgressCallback = nullptr;
    std::function<void(ota_error_t)> onErrorCallback = nullptr;

    // Internal handlers
    void setupCallbacks();
    void handleError(ota_error_t error);
};

} // namespace HAL

#endif // BANYA_OTA_H
