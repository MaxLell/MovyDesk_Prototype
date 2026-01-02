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

// ###########################################################################
// # Private Data
// ###########################################################################

static bool is_initialized = false;
static NimBLEScan *pBLEScan = nullptr;
static bool list_devices_requested = false;

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
static void prv_list_close_devices(void);

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

    is_initialized = true;

    Serial.println("PresenceDetector: Initialized with BLE scanning");
}

void presencedetector_run(void)
{
    ASSERT(is_initialized);

    // Check if device listing was requested
    if (list_devices_requested)
    {
        list_devices_requested = false; // Reset flag
        prv_list_close_devices();
    }

    // Run presence detection logic here
    // Publish messages based on presence detection
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
        // Handle List Close Devices - just set flag, don't block
        list_devices_requested = true;
        break;
    case MSG_2004:
        // Handle Stop Listing Close Devices - just clear flag, don't block
        list_devices_requested = false;
        break;
    default:
        // Unknown message ID
        ASSERT(false);
        break;
    }
}

// Estimate distance based on RSSI
// This is a rough approximation and can vary significantly based on environment
static float prv_estimate_distance(int rssi)
{
    // Assuming measured power at 1m is -59 dBm (typical for BLE devices)
    int txPower = -59;

    if (rssi == 0)
    {
        return -1.0; // Unknown distance
    }

    // Path loss formula: distance = 10 ^ ((txPower - rssi) / (10 * n))
    // where n is the path loss exponent (typically 2-4, we use 2 for free space)
    float ratio = (txPower - rssi) / (10.0 * 2.0);
    return pow(10, ratio);
}

// Get distance category as string
static const char *prv_get_distance_category(float distance)
{
    if (distance < 0)
        return "Unknown";
    if (distance < 1.0)
        return "Immediate";
    if (distance < 3.0)
        return "Near";
    if (distance < 10.0)
        return "Far";
    return "Very Far";
}

// List all close devices (blocking operation - called from presencedetector_run)
static void prv_list_close_devices(void)
{
    Serial.printf("\nScanning for %d seconds...\n", SCAN_TIME);

    // Start scan and get results
    NimBLEScanResults results = pBLEScan->getResults(SCAN_TIME * 1000, true);

    int device_count = results.getCount();

    // Create vector for sorting
    std::vector<DeviceInfo> devices;

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

    Serial.println("Close devices found:\n");

    int displayedCount = 0;

    for (size_t i = 0; i < devices.size(); i++)
    {
        const NimBLEAdvertisedDevice *device = devices[i].device;
        int rssi = device->getRSSI();
        float distance = prv_estimate_distance(rssi);

        // Only display devices that are Immediate or Near (< 3.0m)
        if (distance >= 3.0 && distance >= 0)
        {
            continue;
        }

        displayedCount++;
        Serial.printf("Device %d:\n", displayedCount);
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

    Serial.printf("Scan complete. %d devices found (%d nearby).\n", device_count, displayedCount);
}
