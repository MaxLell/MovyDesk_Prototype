#include "PresenceDetector.h"
#include "custom_assert.h"
#include "MessageBroker.h"
#include "custom_types.h"
#include <Arduino.h>
#include <NimBLEDevice.h>
#include <vector>
#include <algorithm>

// ###########################################################################
// # Internal Configuration
// ###########################################################################

#define SCAN_TIME 5 // Scan duration in seconds
#define FANCY_LOGGING_ENABLED 0

// Distance estimation constants
#define BLE_TX_POWER_AT_1M -59     // Measured power at 1m in dBm (typical for BLE)
#define PATH_LOSS_EXPONENT 2.0     // Path loss exponent (2 = free space, 2-4 typical)
#define DISTANCE_FORMULA_BASE 10.0 // Base for distance calculation formula

// Distance category thresholds (in meters)
#define DISTANCE_IMMEDIATE_MAX 1.0
#define DISTANCE_NEAR_MAX 3.0
#define DISTANCE_FAR_MAX 10.0
#define DISTANCE_CLOSE_DEVICE_MAX 3.0 // Maximum distance to consider a device "close"

// ###########################################################################
// # Private Data
// ###########################################################################

static bool is_initialized = false;
static NimBLEScan *pBLEScan = nullptr;
static bool is_logging_enabled = false;
static unsigned long last_scan_time = 0;
static const unsigned long SCAN_INTERVAL_MS = 5000; // 5 seconds between scans

// Helper structure for sorting
struct DeviceInfo
{
    const NimBLEAdvertisedDevice *device;
    int rssi;
};

// ###########################################################################
// # Private Function Declarations
// ###########################################################################

static void prv_msg_broker_callback(const msg_t *const message);
static float prv_estimate_distance(int rssi);
static const char *prv_get_distance_category(float distance);
static void prv_list_close_devices_one_shot(void);
static NimBLEScanResults prv_perform_ble_scan(void);
static std::vector<DeviceInfo> prv_create_sorted_device_list(const NimBLEScanResults &results);
static int prv_count_and_log_close_devices(const std::vector<DeviceInfo> &devices);
static void prv_log_single_device(const NimBLEAdvertisedDevice *device, int device_number, float distance);

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

    // Subscribe to relevant messages if needed
    messagebroker_subscribe(MSG_2003, prv_msg_broker_callback);
    messagebroker_subscribe(MSG_2004, prv_msg_broker_callback);

    is_initialized = true;

    Serial.println("PresenceDetector: Initialized with BLE scanning");
}

void presencedetector_run(void)
{
    ASSERT(is_initialized);

    prv_list_close_devices_one_shot();
}

// ###########################################################################
// # Private Function Implementations
// ###########################################################################

static void prv_msg_broker_callback(const msg_t *const message)
{
    ASSERT(message != NULL);

    switch (message->msg_id)
    {
    case MSG_2003:
        // Enable logging of scan results
        is_logging_enabled = true;
        Serial.println("PresenceDetector: Logging enabled");
        break;
    case MSG_2004:
        // Disable logging of scan results
        is_logging_enabled = false;
        Serial.println("PresenceDetector: Logging disabled");
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

// Get distance category as string
static const char *prv_get_distance_category(float distance)
{
    if (distance < 0)
        return "Unknown";
    if (distance < DISTANCE_IMMEDIATE_MAX)
        return "Immediate";
    if (distance < DISTANCE_NEAR_MAX)
        return "Near";
    if (distance < DISTANCE_FAR_MAX)
        return "Far";
    return "Very Far";
}

// Perform BLE scan and return results
static NimBLEScanResults prv_perform_ble_scan(void)
{
    return pBLEScan->getResults(SCAN_TIME * 1000, true);
}

// Create sorted device list from scan results
static std::vector<DeviceInfo> prv_create_sorted_device_list(const NimBLEScanResults &results)
{
    std::vector<DeviceInfo> devices;
    int device_count = results.getCount();

    // Convert scan results to vector
    for (int i = 0; i < device_count; i++)
    {
        const NimBLEAdvertisedDevice *device = results.getDevice(i);
        DeviceInfo info;
        info.device = device;
        info.rssi = device->getRSSI();
        devices.push_back(info);
    }

    // Sort by RSSI (descending - closest first)
    std::sort(devices.begin(), devices.end(), [](const DeviceInfo &a, const DeviceInfo &b)
              { return a.rssi > b.rssi; });

    return devices;
}

// Log detailed information about a single device
static void prv_log_single_device(const NimBLEAdvertisedDevice *device, int device_number, float distance)
{
    int rssi = device->getRSSI();

    Serial.printf("Device %d:\n", device_number);
    Serial.printf("  Address:  %s\n", device->getAddress().toString().c_str());
    Serial.printf("  RSSI:     %d dBm\n", rssi);
    Serial.printf("  Distance: %.2f m (%s)\n", distance, prv_get_distance_category(distance));

    // If the device has a name, display it
    if (device->haveName())
    {
        Serial.printf("  Name:     %s\n", device->getName().c_str());
    }

    // Show manufacturer data if available
    if (device->haveManufacturerData())
    {
        std::string manufacturerData = device->getManufacturerData();
        Serial.printf("  Manufacturer data: ");
        for (size_t j = 0; j < manufacturerData.length(); j++)
        {
            Serial.printf("%02X ", (uint8_t)manufacturerData[j]);
        }
        Serial.println();
    }

    Serial.println("---");
}

// Count and log close devices
static int prv_count_and_log_close_devices(const std::vector<DeviceInfo> &devices)
{
    int close_device_count = 0;

    for (size_t i = 0; i < devices.size(); i++)
    {
        const NimBLEAdvertisedDevice *device = devices[i].device;
        int rssi = device->getRSSI();
        float distance = prv_estimate_distance(rssi);

        // Only count devices that are close
        if (distance >= DISTANCE_CLOSE_DEVICE_MAX && distance >= 0)
        {
            continue;
        }

        close_device_count++;

        // Log device details if fancy logging is enabled
        if (is_logging_enabled && FANCY_LOGGING_ENABLED)
        {
            prv_log_single_device(device, close_device_count, distance);
        }
    }

    return close_device_count;
}

// List all close devices (blocking operation - called from presencedetector_run)
static void prv_list_close_devices_one_shot(void)
{
    // Perform BLE scan
    NimBLEScanResults results = prv_perform_ble_scan();
    int total_nof_devices = results.getCount();

    // Create and sort device list
    std::vector<DeviceInfo> devices = prv_create_sorted_device_list(results);

    // Count and log close devices
    int close_device_count = prv_count_and_log_close_devices(devices);

    // Log summary
    if (is_logging_enabled && FANCY_LOGGING_ENABLED)
    {
        Serial.printf("Scan complete. %d devices found (%d nearby).\n", total_nof_devices, close_device_count);
    }
    else if (is_logging_enabled && !FANCY_LOGGING_ENABLED)
    {
        Serial.printf("%d,", close_device_count);
    }
}
