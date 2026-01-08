#include <Arduino.h>
#include "ApplicationControl.h"
#include "BlinkLed.h"
#include "Console.h"
#include "DeskControl.h"
#include "MessageBroker.h"
#include "PresenceDetector.h"
#include "TimerManager.h"
#include "custom_assert.h"

// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ###########################################################################
// # Private function declarations
// ###########################################################################
void console_task(void* parameter);
void deskcontrol_task(void* parameter);
void presencedetector_task(void* parameter);
void applicationcontrol_task(void* parameter);
void timermanager_task(void* parameter);
static void prv_assert_failed(const char* file, uint32_t line, const char* expr);
static void msg_broker_callback(const msg_t* const message);

// ###########################################################################
// # Task handles
// ###########################################################################
TaskHandle_t console_task_handle = NULL;
TaskHandle_t deskcontrol_task_handle = NULL;
TaskHandle_t presencedetector_task_handle = NULL;
TaskHandle_t applicationcontrol_task_handle = NULL;
TaskHandle_t timermanager_task_handle = NULL;

// ###########################################################################
// # Private Data
// ###########################################################################
constexpr int LED_PIN = 2;
static bool g_assert_was_triggered = false;
static bool g_person_is_present = false;

// ###########################################################################
// # Setup and Loop
// ###########################################################################

void setup()
{
    // Initialize custom assert
    custom_assert_init(prv_assert_failed);

    messagebroker_init();

    // Create console task
    xTaskCreate(console_task,        // Task function
                "ConsoleTask",       // Task name
                4096,                // Stack size (words)
                NULL,                // Task parameters
                1,                   // Task priority (0 = lowest, configMAX_PRIORITIES - 1 = highest)
                &console_task_handle // Task handle
    );

    // Create desk control task
    xTaskCreate(deskcontrol_task,        // Task function
                "DeskControlTask",       // Task name
                4096,                    // Stack size (words)
                NULL,                    // Task parameters
                2,                       // Task priority (higher than console)
                &deskcontrol_task_handle // Task handle
    );

    // Create application control task
    xTaskCreate(applicationcontrol_task,        // Task function
                "ApplicationControlTask",       // Task name
                4096,                           // Stack size (words)
                NULL,                           // Task parameters
                2,                              // Task priority (higher than console)
                &applicationcontrol_task_handle // Task handle
    );

    // Create timer manager task
    xTaskCreate(timermanager_task,        // Task function
                "TimerManagerTask",       // Task name
                4096,                     // Stack size (words)
                NULL,                     // Task parameters
                2,                        // Task priority (higher than console)
                &timermanager_task_handle // Task handle
    );

    // Stabilize the power on the system to avoid brownout issues on ESP32
    // The presence detector task requires more power during bluetooth scanning
    delay(1000);

    // Create presence detector task
    xTaskCreate(presencedetector_task,        // Task function
                "PresenceDetectorTask",       // Task name
                8192,                         // Stack size (words) - increased for BLE
                NULL,                         // Task parameters
                1,                            // Task priority (same as console)
                &presencedetector_task_handle // Task handle
    );

    // Initialize BlinkLed module
    blinkled_init(LED_PIN);

    // Subscribe to the presense detected message
    messagebroker_subscribe(MSG_2001, msg_broker_callback);
    messagebroker_subscribe(MSG_2002, msg_broker_callback);
}

void loop()
{
    if (g_assert_was_triggered)
    {
        return;
    }

    // Blink the LED based on presence state
    if (g_person_is_present)
    {
        blinkled_toggle();
        delay(30);
    }
    else // No person present
    {
        blinkled_disable();
    }
}

// ###########################################################################
// # Task implementations
// ###########################################################################

void console_task(void* parameter)
{
    (void)parameter; // Unused parameter

    // Initialize console
    console_init();

    // Task main loop
    while (1)
    {
        // Run the console processing
        console_run();

        delay(5);
    }
}

void deskcontrol_task(void* parameter)
{
    (void)parameter; // Unused parameter

    // Initialize desk control
    deskcontrol_init();

    // Task main loop
    while (1)
    {
        // Run the desk control processing
        deskcontrol_run();

        delay(5);
    }
}

void presencedetector_task(void* parameter)
{
    (void)parameter; // Unused parameter

    // Initialize presence detector
    presencedetector_init();

    // Task main loop
    while (1)
    {
        // Run the presence detector processing
        presencedetector_run();

        delay(5);
    }
}

void applicationcontrol_task(void* parameter)
{
    (void)parameter; // Unused parameter

    // Initialize application control
    applicationcontrol_init();

    // Task main loop
    while (1)
    {
        // Run the application control processing
        applicationcontrol_run();

        delay(5);
    }
}

void timermanager_task(void* parameter)
{
    (void)parameter; // Unused parameter

    // Initialize timer manager
    timermanager_init();

    // Task main loop
    while (1)
    {
        // Run the timer manager processing
        timermanager_run();

        delay(10000000);
    }
}

// ###########################################################################
// # Private function implementations
// ###########################################################################

static void prv_assert_failed(const char* file, uint32_t line, const char* expr)
{
    g_assert_was_triggered = true;
    Serial.printf("[ASSERT FAILED]: %s:%u - %s\n", file, line, expr);
    // In embedded systems, we might want to reset instead of infinite loop

    // Stop all tasks
    vTaskSuspend(console_task_handle);
    vTaskSuspend(deskcontrol_task_handle);
    vTaskSuspend(presencedetector_task_handle);
    vTaskSuspend(applicationcontrol_task_handle);
    vTaskSuspend(timermanager_task_handle);

    while (1)
    {
        blinkled_toggle();
        delay(700); // Keep watchdog happy if enabled
    }
}

static void msg_broker_callback(const msg_t* const message)
{
    ASSERT(message != NULL);

    switch (message->msg_id)
    {
        case MSG_2001: // Presence Detected
            g_person_is_present = true;
            break;
        case MSG_2002: // No Presence Detected
            g_person_is_present = false;
            break;
        default:
            // Unknown message ID
            ASSERT(false);
            break;
    }
}