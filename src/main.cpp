#include <Arduino.h>
#include "ApplicationControl.h"
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
bool assert_was_triggered = false;

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

    // Create presence detector task
    xTaskCreate(presencedetector_task,        // Task function
                "PresenceDetectorTask",       // Task name
                4096,                         // Stack size (words)
                NULL,                         // Task parameters
                1,                            // Task priority (same as console)
                &presencedetector_task_handle // Task handle
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

    // Setup LED pin
    pinMode(LED_PIN, OUTPUT);
}

void loop()
{
    if (!assert_was_triggered)
    {
        // Toggle LED - for a visual heartbeat
        static bool led_state = false;
        led_state = !led_state;
        digitalWrite(LED_PIN, led_state ? HIGH : LOW);
        delay(1000);
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
    assert_was_triggered = true;
    Serial.printf("[ASSERT FAILED]: %s:%u - %s\n", file, line, expr);
    // In embedded systems, we might want to reset instead of infinite loop
    while (1)
    {
        delay(1000); // Keep watchdog happy if enabled
    }
}