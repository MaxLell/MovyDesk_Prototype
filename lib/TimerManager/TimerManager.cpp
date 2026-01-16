#include "TimerManager.h"
#include <Arduino.h>
#include "MessageBroker.h"
#include "MessageDefinitions.h"
#include "custom_assert.h"
#include "custom_types.h"

// ###########################################################################
// # Internal Configuration
// ###########################################################################

// ###########################################################################
// # Private function declarations
// ###########################################################################
static void prv_timermanager_task(void* parameter);
static void prv_timermanager_init(void);
static void prv_timermanager_run(void);
static void prv_msg_broker_callback(const msg_t* const message);
static void prv_timer_expired_callback(TimerHandle_t xTimer);

// ###########################################################################
// # Private variables
// ###########################################################################

static TimerHandle_t countdown_timer_handle = NULL;

// ###########################################################################
// # Public function implementations
// ###########################################################################

TaskHandle_t timermanager_create_task(void)
{
    TaskHandle_t task_handle = NULL;

    xTaskCreate(prv_timermanager_task, // Task function
                "TimerManagerTask",    // Task name
                4096,                  // Stack size (words)
                NULL,                  // Task parameters
                2,                     // Task priority
                &task_handle           // Task handle
    );

    return task_handle;
}

// ###########################################################################
// # Private function implementations
// ###########################################################################

static void prv_timermanager_task(void* parameter)
{
    (void)parameter; // Unused parameter

    // Initialize timer manager
    prv_timermanager_init();

    // Task main loop
    while (1)
    {
        // Run the timer manager processing
        prv_timermanager_run();

        delay(10000000);
    }
}

static void prv_timermanager_init(void)
{
    // Subscribe to relevant messages
    messagebroker_subscribe(MSG_3001, prv_msg_broker_callback); // Start Countdown with Time Stamp
    messagebroker_subscribe(MSG_3002, prv_msg_broker_callback); // Stop Countdown

    // Create the countdown timer (not started yet)
    countdown_timer_handle =
        xTimerCreate("CountdownTimer",          // Timer name
                     pdMS_TO_TICKS(1000),       // Default timer period (1 second, will be changed when started)
                     pdFALSE,                   // No auto-reload
                     NULL,                      // Timer ID
                     prv_timer_expired_callback // Callback function
        );
    ASSERT(countdown_timer_handle != NULL); // Not enough memory to create timer
}

static void prv_timermanager_run(void)
{
    // Nothing to do in the run loop for now
}

// ###########################################################################
// # Private function implementations
// ###########################################################################

static void prv_msg_broker_callback(const msg_t* const message)
{
    ASSERT(message != NULL);

    switch (message->msg_id)
    {
        case MSG_3001: // Start Countdown with Time Stamp from the Message Defintions
        {
            ASSERT(countdown_timer_handle != NULL);
            ASSERT(message->data_size == sizeof(uint32_t));

            uint32_t countdown_time_ms = 0;
            // Start a FreeRTOS timer here with the specified time from message->data_bytes
            memcpy(&countdown_time_ms, message->data_bytes, sizeof(uint32_t));

            ASSERT(countdown_time_ms > 0);

            // Change timer period and start the timer
            BaseType_t status = xTimerChangePeriod(countdown_timer_handle, pdMS_TO_TICKS(countdown_time_ms), 0);
            ASSERT(status == pdPASS);

            status = xTimerStart(countdown_timer_handle, 0);
            ASSERT(status == pdPASS);
        }

        break;

        case MSG_3002: // Stop Countdown Timer
        {
            // Stop the FreeRTOS timer here
            ASSERT(countdown_timer_handle != NULL);
            xTimerStop(countdown_timer_handle, 0);
        }
        break;

        default:
            // Unknown message ID
            ASSERT(false); // This must never happen
            break;
    }
}

static void prv_timer_expired_callback(TimerHandle_t xTimer)
{
    ASSERT(xTimer == countdown_timer_handle);

    msg_t msg;
    msg.msg_id = MSG_3003; // Countdown finished
    msg.data_size = 0;
    msg.data_bytes = NULL;

    messagebroker_publish(&msg);
}
