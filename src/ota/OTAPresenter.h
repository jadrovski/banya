#ifndef BANYA_OTA_PRESENTER_H
#define BANYA_OTA_PRESENTER_H

#include <Arduino.h>

namespace HAL {

/**
 * @brief OTA Progress data structure
 */
struct OTAProgress {
    uint8_t percent;        ///< Progress percentage (0-100)
    uint16_t current;       ///< Current bytes
    uint16_t total;         ///< Total bytes
};

/**
 * @brief OTA Error types
 */
enum class OTAErrorType {
    UNKNOWN,
    AUTH,
    BEGIN,
    CONNECT,
    RECEIVE,
    END
};

/**
 * @brief Presenter interface for OTA updates
 *
 * Following Clean Architecture principles, this interface abstracts
 * the display/presentation layer from the OTA business logic.
 * The OTAManager depends on this abstraction, not on concrete LCD.
 */
class OTAPresenter {
public:
    virtual ~OTAPresenter() = default;

    /**
     * @brief Called when OTA update starts
     */
    virtual void showStart() = 0;

    /**
     * @brief Called when OTA update completes
     */
    virtual void showEnd() = 0;

    /**
     * @brief Called with OTA progress updates
     * @param progress Progress data
     */
    virtual void showProgress(const OTAProgress& progress) = 0;

    /**
     * @brief Called when OTA error occurs
     * @param errorType Type of error
     * @param message Error message string
     */
    virtual void showError(OTAErrorType errorType, const String& message) = 0;
};

} // namespace HAL

#endif // BANYA_OTA_PRESENTER_H
