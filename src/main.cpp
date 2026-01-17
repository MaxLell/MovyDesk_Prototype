#include <Arduino.h>
#include "ApplicationControl.h"
#include "BlinkLed.h"
#include "Console.h"
#include "DeskControl.h"
#include "MessageBroker.h"
#include "NetworkTime.h"
#include "PresenceDetector.h"
#include "TimerManager.h"
#include "custom_assert.h"

// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ###########################################################################
// # Private function declarations
// ###########################################################################
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
TaskHandle_t networktime_task_handle = NULL;

// ###########################################################################
// # Private Data
// ###########################################################################
constexpr int LED_PIN = 15;
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

    // Create all tasks using module-specific functions
    console_task_handle = console_create_task();
    deskcontrol_task_handle = deskcontrol_create_task();
    applicationcontrol_task_handle = applicationcontrol_create_task();
    timermanager_task_handle = timermanager_create_task();
    networktime_task_handle = networktime_create_task();

    // Stabilize the power on the system to avoid brownout issues on ESP32
    // The presence detector task requires more power during bluetooth scanning
    delay(1000);

    presencedetector_task_handle = presencedetector_create_task();

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
        delay(50);
    }
    else // No person present
    {
        blinkled_disable();
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
    vTaskSuspend(networktime_task_handle);

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