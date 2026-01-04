#include "PresenceDetector.h"
#include <Arduino.h>
#include <NimBLEDevice.h>
#include <vector>
#include "MessageBroker.h"
#include "custom_assert.h"
#include "custom_types.h"

// ###########################################################################
// # Internal Configuration
// ###########################################################################

#define SCAN_TIME                 5 // Scan duration in seconds

// Distance estimation constants
#define BLE_TX_POWER_AT_1M        -59  // Measured power at 1m in dBm (typical for BLE)
#define PATH_LOSS_EXPONENT        2.0  // Path loss exponent (2 = free space, 2-4 typical)
#define DISTANCE_FORMULA_BASE     10.0 // Base for distance calculation formula

// Distance category thresholds (in meters)
#define DISTANCE_CLOSE_DEVICE_MAX 4.0 // Maximum distance to consider a device "close" (4 meters)

// Presence detection configuration
#define PRESENCE_THRESHOLD        3    // Minimum number of close devices to detect presence
#define SCAN_INTERVAL_MS          5000 // 5 seconds between scans

// ###########################################################################
// # Private Data
// ###########################################################################

static bool is_initialized = false;
static NimBLEScan* pBLEScan = nullptr;
static bool is_logging_enabled = false;
static unsigned long last_scan_time = 0;

// Presence detection state
static bool presence_detected = false;

// Helper structure for device info
struct DeviceInfo
{
    const NimBLEAdvertisedDevice* device;
    int rssi;
};

// ###########################################################################
// # Private Function Declarations
// ###########################################################################

static void prv_msg_broker_callback(const msg_t* const message);
static float prv_estimate_distance(int rssi);
static std::vector<DeviceInfo> prv_create_device_list(const NimBLEScanResults& results);
static int prv_count_close_devices(const std::vector<DeviceInfo>& devices);
static void prv_process_scan_results(void);
static void prv_check_and_publish_presence_state(int close_device_count);

// ###########################################################################
// # Public Function Implementations
// ###########################################################################

void presencedetector_init(void)
{
    ASSERT(!is_initialized);

    // Initialize BLE
    NimBLEDevice::init("");

    // Create scanner
    pBLEScan = NimBLEDevice::getScan();
    pBLEScan->setActiveScan(true); // Active scan for more information
    pBLEScan->setInterval(100);    // Scan interval in ms
    pBLEScan->setWindow(99);       // Scan window in ms

    // Subscribe to logging control messages
    messagebroker_subscribe(MSG_0005, prv_msg_broker_callback);

    // Start continuous scanning
    pBLEScan->start(0, false, false); // 0 = continuous scan, no callback, don't restart

    is_initialized = true;
}

void presencedetector_run(void)
{
    ASSERT(is_initialized);

    // Process scan results at regular intervals
    unsigned long current_time = millis();
    if (current_time - last_scan_time >= SCAN_INTERVAL_MS)
    {
        last_scan_time = current_time;
        prv_process_scan_results();
    }
}

// ###########################################################################
// # Private Function Implementations
// ###########################################################################

static void prv_msg_broker_callback(const msg_t* const message)
{
    ASSERT(message != NULL);

    switch (message->msg_id)
    {
        case MSG_0005: // Set Logging State
            if (message->data_size == sizeof(bool))
            {
                is_logging_enabled = *(bool*)(message->data_bytes);
                Serial.print("[PresenceDetect] Logging ");
                Serial.println(is_logging_enabled ? "enabled" : "disabled");
            }
            break;
        default:
            // Unknown message ID
            break;
    }
}

// Estimate distance based on RSSI
// This is a rough approximation and can vary significantly based on environment
static float prv_estimate_distance(int rssi)
{
    if (rssi == 0)
    {
        return -1.0; // Unknown distance
    }

    // Path loss formula: distance = 10 ^ ((txPower - rssi) / (10 * n))
    // where n is the path loss exponent
    float ratio = (BLE_TX_POWER_AT_1M - rssi) / (DISTANCE_FORMULA_BASE * PATH_LOSS_EXPONENT);
    return pow(DISTANCE_FORMULA_BASE, ratio);
}

// Create device list from scan results (no sorting)
static std::vector<DeviceInfo> prv_create_device_list(const NimBLEScanResults& results)
{
    std::vector<DeviceInfo> devices;
    int device_count = results.getCount();

    // Convert scan results to vector
    for (int i = 0; i < device_count; i++)
    {
        const NimBLEAdvertisedDevice* device = results.getDevice(i);
        DeviceInfo info;
        info.device = device;
        info.rssi = device->getRSSI();
        devices.push_back(info);
    }

    return devices;
}

// Count close devices (no logging)
static int prv_count_close_devices(const std::vector<DeviceInfo>& devices)
{
    int close_device_count = 0;

    for (size_t i = 0; i < devices.size(); i++)
    {
        const NimBLEAdvertisedDevice* device = devices[i].device;
        int rssi = device->getRSSI();
        float distance = prv_estimate_distance(rssi);

        // Only count devices that are close
        if (distance >= DISTANCE_CLOSE_DEVICE_MAX && distance >= 0)
        {
            continue;
        }

        close_device_count++;
    }

    return close_device_count;
}

// Check presence state and publish messages
static void prv_check_and_publish_presence_state(int close_device_count)
{
    bool is_person_currently_present = (close_device_count >= PRESENCE_THRESHOLD);

    // Update presence state and always publish
    bool state_changed = (is_person_currently_present != presence_detected);
    presence_detected = is_person_currently_present;

    msg_t presence_msg;
    presence_msg.data_size = 0;
    presence_msg.data_bytes = NULL;

    if (presence_detected)
    {
        presence_msg.msg_id = MSG_2001; // Presence Detected
        if (is_logging_enabled)
        {
            Serial.print("[PresenceDetect] Person ");
            Serial.print(state_changed ? "DETECTED" : "PRESENT");
            Serial.print(" (");
            Serial.print(close_device_count);
            Serial.println(" devices)");
        }
    }
    else
    {
        presence_msg.msg_id = MSG_2002; // No Presence Detected
        if (is_logging_enabled)
        {
            Serial.print("[PresenceDetect] Person ");
            Serial.print(state_changed ? "LOST" : "ABSENT");
            Serial.print(" (");
            Serial.print(close_device_count);
            Serial.println(" devices)");
        }
    }

    messagebroker_publish(&presence_msg);
}

// Process current scan results (non-blocking)
static void prv_process_scan_results(void)
{
    // Get current scan results (non-blocking, returns immediately)
    NimBLEScanResults results = pBLEScan->getResults(SCAN_INTERVAL_MS, true);

    // Create device list
    std::vector<DeviceInfo> devices = prv_create_device_list(results);

    // Count close devices
    int close_device_count = prv_count_close_devices(devices);

    // Check and publish presence state
    prv_check_and_publish_presence_state(close_device_count);

    // Clear old results to prepare for next interval
    pBLEScan->clearResults();
}
