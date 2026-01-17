/**
 * MIT License
 *
 * Copyright (c) <2025> <Max Koell (maxkoell@proton.me)>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @file NetworkTime.cpp
 * @brief Network Time module that retrieves current time and weekday from the internet via WiFi.
 */

#include "NetworkTime.h"
#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <time.h>
#include "MessageBroker.h"
#include "MessageDefinitions.h"
#include "custom_assert.h"
#include "custom_types.h"

// ###########################################################################
// # Internal Configuration
// ###########################################################################

#define WIFI_MAX_SSID_LEN          32
#define WIFI_MAX_PASSWORD_LEN      64
#define WIFI_CONNECTION_TIMEOUT_MS 10000
#define TIME_SYNC_INTERVAL_MS      3600000 // Sync every hour
#define NTP_SERVER                 "pool.ntp.org"
#define GMT_OFFSET_SEC             3600 // GMT+1 (adjust for your timezone)
#define DAYLIGHT_OFFSET_SEC        3600 // Daylight saving time offset

typedef struct
{
    char ssid[WIFI_MAX_SSID_LEN + 1];
    char password[WIFI_MAX_PASSWORD_LEN + 1];
    bool credentials_exist;
} wifi_credentials_t;

// ###########################################################################
// # Private function declarations
// ###########################################################################
static void prv_networktime_task(void* parameter);
static void prv_networktime_init(void);
static void prv_networktime_run(void);
static void prv_load_wifi_credentials_from_flash(void);
static void prv_save_wifi_credentials_to_flash(void);
static bool prv_connect_to_wifi(void);
static void prv_sync_time_with_ntp(void);
static void prv_msg_broker_callback(const msg_t* const message);

// ###########################################################################
// # Private variables
// ###########################################################################

static wifi_credentials_t g_wifi_credentials = {0};
static bool g_wifi_connected = false;
static bool g_time_synchronized = false;
static bool prv_logging_enabled = false;
static Preferences prv_preferences;
static unsigned long last_sync_time = 0;

// ###########################################################################
// # Public function implementations
// ###########################################################################

TaskHandle_t networktime_create_task(void)
{
    TaskHandle_t task_handle = NULL;

    xTaskCreate(prv_networktime_task, // Task function
                "NetworkTimeTask",    // Task name
                4096,                 // Stack size (words)
                NULL,                 // Task parameters
                2,                    // Task priority
                &task_handle          // Task handle
    );

    return task_handle;
}

int networktime_get_current_hour(void)
{
    if (!g_time_synchronized)
    {
        return -1;
    }

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        return -1;
    }

    return timeinfo.tm_hour;
}

int networktime_get_current_weekday(void)
{
    if (!g_time_synchronized)
    {
        return -1;
    }

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        return -1;
    }

    return timeinfo.tm_wday; // 0 = Sunday, 6 = Saturday
}

bool networktime_is_synchronized(void) { return g_time_synchronized; }

void networktime_set_wifi_credentials(const char* ssid, const char* password)
{
    if (ssid == NULL || password == NULL)
    {
        return;
    }

    strncpy(g_wifi_credentials.ssid, ssid, WIFI_MAX_SSID_LEN);
    g_wifi_credentials.ssid[WIFI_MAX_SSID_LEN] = '\0';

    strncpy(g_wifi_credentials.password, password, WIFI_MAX_PASSWORD_LEN);
    g_wifi_credentials.password[WIFI_MAX_PASSWORD_LEN] = '\0';

    g_wifi_credentials.credentials_exist = true;

    prv_save_wifi_credentials_to_flash();

    if (prv_logging_enabled)
    {
        Serial.println("[NetTime] WiFi credentials updated");
    }

    // Try to connect immediately
    if (prv_connect_to_wifi())
    {
        prv_sync_time_with_ntp();
    }
}

bool networktime_get_wifi_credentials(char* ssid, char* password)
{
    if (ssid == NULL || password == NULL || !g_wifi_credentials.credentials_exist)
    {
        return false;
    }

    strncpy(ssid, g_wifi_credentials.ssid, WIFI_MAX_SSID_LEN);
    ssid[WIFI_MAX_SSID_LEN] = '\0';

    strncpy(password, g_wifi_credentials.password, WIFI_MAX_PASSWORD_LEN);
    password[WIFI_MAX_PASSWORD_LEN] = '\0';

    return true;
}

bool networktime_is_wifi_connected(void) { return g_wifi_connected; }

// ###########################################################################
// # Private function implementations
// ###########################################################################

static void prv_networktime_task(void* parameter)
{
    (void)parameter;

    prv_networktime_init();

    while (1)
    {
        prv_networktime_run();
        delay(1000); // Check every second
    }
}

static void prv_networktime_init(void)
{
    // Load WiFi credentials from flash
    prv_load_wifi_credentials_from_flash();

    // Subscribe to logging control messages
    messagebroker_subscribe(MSG_0006, prv_msg_broker_callback); // Enable/Disable Logging
    messagebroker_subscribe(MSG_5001, prv_msg_broker_callback); // Set WiFi Credentials
    messagebroker_subscribe(MSG_5002, prv_msg_broker_callback); // Get WiFi Credentials
    messagebroker_subscribe(MSG_5003, prv_msg_broker_callback); // Get WiFi Status
    messagebroker_subscribe(MSG_5004, prv_msg_broker_callback); // Get Time Info

    // Try to connect to WiFi if credentials exist
    if (g_wifi_credentials.credentials_exist)
    {
        if (prv_connect_to_wifi())
        {
            prv_sync_time_with_ntp();
        }
    }
    else
    {
        Serial.println("[NetTime] No WiFi credentials found. Use console to set credentials.");
    }
}

static void prv_networktime_run(void)
{
    // Check WiFi connection status
    if (g_wifi_credentials.credentials_exist)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            if (!g_wifi_connected)
            {
                g_wifi_connected = true;
                if (prv_logging_enabled)
                {
                    Serial.println("[NetTime] WiFi connected");
                }
            }

            // Periodic time sync
            if (millis() - last_sync_time > TIME_SYNC_INTERVAL_MS)
            {
                prv_sync_time_with_ntp();
            }
        }
        else
        {
            if (g_wifi_connected)
            {
                g_wifi_connected = false;
                g_time_synchronized = false;
                if (prv_logging_enabled)
                {
                    Serial.println("[NetTime] WiFi disconnected, attempting reconnect...");
                }
            }

            // Try to reconnect
            prv_connect_to_wifi();
        }
    }
}

static void prv_load_wifi_credentials_from_flash(void)
{
    prv_preferences.begin("nettime", true); // Read-only mode

    String ssid = prv_preferences.getString("ssid", "");
    String password = prv_preferences.getString("password", "");

    if (ssid.length() > 0)
    {
        strncpy(g_wifi_credentials.ssid, ssid.c_str(), WIFI_MAX_SSID_LEN);
        g_wifi_credentials.ssid[WIFI_MAX_SSID_LEN] = '\0';

        strncpy(g_wifi_credentials.password, password.c_str(), WIFI_MAX_PASSWORD_LEN);
        g_wifi_credentials.password[WIFI_MAX_PASSWORD_LEN] = '\0';

        g_wifi_credentials.credentials_exist = true;

        Serial.print("[NetTime] Loaded WiFi SSID from flash: ");
        Serial.println(g_wifi_credentials.ssid);
    }
    else
    {
        g_wifi_credentials.credentials_exist = false;
        Serial.println("[NetTime] No WiFi credentials found in flash");
    }

    prv_preferences.end();
}

static void prv_save_wifi_credentials_to_flash(void)
{
    prv_preferences.begin("nettime", false); // Read-write mode

    prv_preferences.putString("ssid", g_wifi_credentials.ssid);
    prv_preferences.putString("password", g_wifi_credentials.password);

    prv_preferences.end();

    Serial.println("[NetTime] WiFi credentials saved to flash");
}

static bool prv_connect_to_wifi(void)
{
    if (!g_wifi_credentials.credentials_exist)
    {
        return false;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        return true;
    }

    Serial.print("[NetTime] Connecting to WiFi: ");
    Serial.println(g_wifi_credentials.ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(g_wifi_credentials.ssid, g_wifi_credentials.password);

    unsigned long start_time = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start_time) < WIFI_CONNECTION_TIMEOUT_MS)
    {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println();
        Serial.print("[NetTime] WiFi connected. IP: ");
        Serial.println(WiFi.localIP());
        g_wifi_connected = true;
        return true;
    }
    else
    {
        Serial.println();
        Serial.println("[NetTime] WiFi connection failed");
        g_wifi_connected = false;
        return false;
    }
}

static void prv_sync_time_with_ntp(void)
{
    if (!g_wifi_connected)
    {
        return;
    }

    Serial.println("[NetTime] Synchronizing time with NTP server...");

    // Configure time with NTP server
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);

    // Wait for time to be set (up to 10 seconds)
    int retry = 0;
    const int retry_count = 10;
    struct tm timeinfo;

    while (!getLocalTime(&timeinfo) && retry < retry_count)
    {
        Serial.print(".");
        delay(1000);
        retry++;
    }

    if (retry < retry_count)
    {
        Serial.println();
        Serial.println("[NetTime] Time synchronized successfully");
        Serial.print("[NetTime] Current time: ");
        Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
        g_time_synchronized = true;
        last_sync_time = millis();
    }
    else
    {
        Serial.println();
        Serial.println("[NetTime] Failed to synchronize time");
        g_time_synchronized = false;
    }
}

static void prv_msg_broker_callback(const msg_t* const message)
{
    ASSERT(message != NULL);

    switch (message->msg_id)
    {
        case MSG_0006: // Set Logging State
            if (message->data_size == sizeof(bool))
            {
                prv_logging_enabled = *(bool*)(message->data_bytes);
                Serial.print("[NetTime] Logging ");
                Serial.println(prv_logging_enabled ? "enabled" : "disabled");
            }
            break;

        case MSG_5001: // Set WiFi Credentials (should not be used directly, use console command)
            // This is handled through the console command
            break;

        case MSG_5002: // Get WiFi Credentials
            if (g_wifi_credentials.credentials_exist)
            {
                Serial.print("[NetTime] WiFi SSID: ");
                Serial.println(g_wifi_credentials.ssid);
                Serial.println("[NetTime] Password: ********");
            }
            else
            {
                Serial.println("[NetTime] No WiFi credentials configured");
            }
            break;

        case MSG_5003: // Get WiFi Status
            Serial.print("[NetTime] WiFi Status: ");
            if (g_wifi_connected)
            {
                Serial.print("Connected to ");
                Serial.print(g_wifi_credentials.ssid);
                Serial.print(" (IP: ");
                Serial.print(WiFi.localIP());
                Serial.println(")");
            }
            else
            {
                Serial.println("Disconnected");
            }
            break;

        case MSG_5004: // Get Time Info
            if (g_time_synchronized)
            {
                struct tm timeinfo;
                if (getLocalTime(&timeinfo))
                {
                    Serial.print("[NetTime] Current time: ");
                    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
                    Serial.print("[NetTime] Hour: ");
                    Serial.print(timeinfo.tm_hour);
                    Serial.print(", Weekday: ");
                    const char* weekdays[] = {"Sunday",   "Monday", "Tuesday", "Wednesday",
                                              "Thursday", "Friday", "Saturday"};
                    Serial.println(weekdays[timeinfo.tm_wday]);
                }
                else
                {
                    Serial.println("[NetTime] Failed to get local time");
                }
            }
            else
            {
                Serial.println("[NetTime] Time not synchronized");
            }
            break;

        default: break;
    }
}
