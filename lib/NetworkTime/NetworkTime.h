#ifndef NETWORKTIME_H
#define NETWORKTIME_H

// FreeRTOS includes
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     * @brief Creates and starts the NetworkTime task
     * @return Task handle for the created task
     */
    TaskHandle_t networktime_create_task(void);

    /**
     * @brief Get the current hour (0-23)
     * @return Current hour or -1 if time not synchronized
     */
    int networktime_get_current_hour(void);

    /**
     * @brief Get the current weekday (0=Sunday, 6=Saturday)
     * @return Current weekday or -1 if time not synchronized
     */
    int networktime_get_current_weekday(void);

    /**
     * @brief Check if the time is synchronized with NTP server
     * @return true if time is synchronized, false otherwise
     */
    bool networktime_is_synchronized(void);

    /**
     * @brief Set WiFi credentials
     * @param ssid WiFi SSID
     * @param password WiFi password
     */
    void networktime_set_wifi_credentials(const char* ssid, const char* password);

    /**
     * @brief Get WiFi credentials
     * @param ssid Buffer to store SSID (must be at least 33 bytes)
     * @param password Buffer to store password (must be at least 65 bytes)
     * @return true if credentials exist, false otherwise
     */
    bool networktime_get_wifi_credentials(char* ssid, char* password);

    /**
     * @brief Get WiFi connection status
     * @return true if WiFi is connected, false otherwise
     */
    bool networktime_is_wifi_connected(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // NETWORKTIME_H
