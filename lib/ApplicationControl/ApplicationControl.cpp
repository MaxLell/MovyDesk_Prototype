#include "ApplicationControl.h"
#include <Arduino.h>
#include <Preferences.h>
#include "MessageBroker.h"
#include "MessageDefinitions.h"
#include "NetworkTime.h"
#include "custom_assert.h"
#include "custom_types.h"

// ###########################################################################
// # Internal Configuration
// ###########################################################################

#define TIME_RESTRICTION_START_HOUR 7  // 07:00 AM
#define TIME_RESTRICTION_END_HOUR   18 // 06:00 PM (18:00)

typedef struct
{
    bool is_person_present;
    bool is_countdown_expired;
} prv_mailbox_t;

static prv_mailbox_t g_mailbox = {
    .is_person_present = false,
    .is_countdown_expired = false,
};
#define DEFAULT_MINUTES 20
static u32 timer_interval_ms = DEFAULT_MINUTES * 60 * 1000; // 20 minutes default
static bool g_run_sequence_once = false;
static bool g_timer_stop_sent = false;   // Flag to prevent repeated timer stop messages
static u32 timer_start_timestamp_ms = 0; // Timestamp when countdown timer started
static Preferences prv_preferences;      // Preferences object for NVS storage

// ###########################################################################
// # Private function declarations
// ###########################################################################
static void prv_applicationcontrol_task(void* parameter);
static void prv_applicationcontrol_init(void);
static void prv_applicationcontrol_run(void);
static void prv_msg_broker_callback(const msg_t* const message);
static void prv_reset_sequence(void);
static void prv_load_settings_from_flash(void);
static void prv_save_timer_interval_to_flash(void);
static bool prv_is_desk_movement_allowed(void);

// ###########################################################################
// # Private variables
// ###########################################################################

// Logging control
static bool prv_logging_enabled = false;

// TODO: Add application state variables here

// ###########################################################################
// # Public function implementations
// ###########################################################################

TaskHandle_t applicationcontrol_create_task(void)
{
    TaskHandle_t task_handle = NULL;

    xTaskCreate(prv_applicationcontrol_task, // Task function
                "ApplicationControlTask",    // Task name
                4096,                        // Stack size (words)
                NULL,                        // Task parameters
                2,                           // Task priority
                &task_handle                 // Task handle
    );

    return task_handle;
}

// ###########################################################################
// # Private function implementations
// ###########################################################################

static void prv_applicationcontrol_task(void* parameter)
{
    (void)parameter; // Unused parameter

    // Initialize application control
    prv_applicationcontrol_init();

    // Task main loop
    while (1)
    {
        // Run the application control processing
        prv_applicationcontrol_run();

        delay(5);
    }
}

static void prv_applicationcontrol_init(void)
{
    // Load settings from flash
    prv_load_settings_from_flash();

    // Subscribe to 2001, 2002, 3003
    messagebroker_subscribe(MSG_2001, prv_msg_broker_callback); // Presence Detected
    messagebroker_subscribe(MSG_2002, prv_msg_broker_callback); // No Presence Detected
    messagebroker_subscribe(MSG_3003, prv_msg_broker_callback); // Countdown finished
    messagebroker_subscribe(MSG_0003, prv_msg_broker_callback); // Set Logging State
    messagebroker_subscribe(MSG_4001, prv_msg_broker_callback); // Set Timer Interval
    messagebroker_subscribe(MSG_4002, prv_msg_broker_callback); // Get Timer Interval
    messagebroker_subscribe(MSG_4003, prv_msg_broker_callback); // Get Elapsed Timer Time
}

static void prv_applicationcontrol_run(void)
{
    if (g_mailbox.is_person_present)
    {
        if (g_run_sequence_once == false)
        {
            if (prv_logging_enabled)
            {
                Serial.print("[AppCtrl] Starting countdown timer for ");
                Serial.print(timer_interval_ms / 60000);
                Serial.println(" minutes");
            }

            msg_t timer_msg;
            timer_msg.msg_id = MSG_3001;
            timer_msg.data_size = sizeof(u32);
            timer_msg.data_bytes = (u8*)&timer_interval_ms;
            messagebroker_publish(&timer_msg);

            // Store timestamp when timer starts
            timer_start_timestamp_ms = millis();

            g_run_sequence_once = true;
        }

        if (g_mailbox.is_countdown_expired)
        { // Check if desk movement is allowed based on time
            if (!prv_is_desk_movement_allowed())
            {
                if (prv_logging_enabled)
                {
                    Serial.println("[AppCtrl] Desk movement not allowed at this time (allowed: 07:00-19:00)");
                }

                // Reset sequence but don't move desk
                prv_reset_sequence();
                return;
            }

            if (prv_logging_enabled)
            {
                Serial.println("[AppCtrl] Action: Toggling desk position");
            }

            // Move desk up (toggle functionality)
            static desk_command_e toggle_command = DESK_CMD_TOGGLE;
            msg_t desk_msg;
            desk_msg.msg_id = MSG_1000;
            desk_msg.data_size = sizeof(desk_command_e);
            desk_msg.data_bytes = (u8*)&toggle_command;
            messagebroker_publish(&desk_msg);

            if (prv_logging_enabled)
            {
                Serial.println("[AppCtrl] Desk position toggled, resetting timer state, resetting sequence");
            }

            prv_reset_sequence();

            // Reset timer timestamp since the cycle is complete
            timer_start_timestamp_ms = 0;
        }
    }
    else
    {
        // Stop the countdown timer if running (only send once)
        if (!g_timer_stop_sent && g_run_sequence_once)
        {
            msg_t timer_msg;
            timer_msg.msg_id = MSG_3002; // Stop Countdown
            timer_msg.data_size = 0;
            timer_msg.data_bytes = NULL;
            messagebroker_publish(&timer_msg);
            g_timer_stop_sent = true;

            if (prv_logging_enabled)
            {
                Serial.println("[AppCtrl] Timer stopped due to no presence");
            }
        }

        // Wait before checking again
        delay(10000);
    }
}

// ###########################################################################
// # Private function implementations
// ###########################################################################

static void prv_msg_broker_callback(const msg_t* const message)
{
    ASSERT(message != NULL);

    switch (message->msg_id)
    {
        case MSG_2001: // Presence Detected
            g_mailbox.is_person_present = true;
            g_timer_stop_sent = false; // Allow timer stop to be sent again if needed
            if (prv_logging_enabled)
            {
                Serial.println("[AppCtrl] Event: Presence Detected");
            }
            break;
        case MSG_2002: // No Presence Detected
            g_mailbox.is_person_present = false;
            // Reset the entire sequence when presence is lost
            prv_reset_sequence();
            timer_start_timestamp_ms = 0;
            if (prv_logging_enabled)
            {
                Serial.println("[AppCtrl] Event: No Presence Detected - Sequence reset");
            }
            break;
        case MSG_3003: // Countdown finished
            g_mailbox.is_countdown_expired = true;
            if (prv_logging_enabled)
            {
                Serial.println("[AppCtrl] Event: Countdown Finished");
            }
            break;
        case MSG_0003: // Set Logging State
            if (message->data_size == sizeof(bool))
            {
                prv_logging_enabled = *(bool*)(message->data_bytes);
                Serial.print("[AppCtrl] Logging ");
                Serial.println(prv_logging_enabled ? "enabled" : "disabled");
            }
            break;
        case MSG_4001: // Set Timer Interval
            if (message->data_size == sizeof(u32))
            {
                timer_interval_ms = *(u32*)(message->data_bytes);
                prv_save_timer_interval_to_flash(); // Save to flash
                Serial.print("[AppCtrl] Timer interval set to ");
                Serial.print(timer_interval_ms / 60000);
                Serial.println(" minutes");

                // reset the sequence and timer timestamp
                g_mailbox.is_countdown_expired = false;
                g_run_sequence_once = false;
                timer_start_timestamp_ms = 0;
            }
            break;
        case MSG_4002: // Get Timer Interval
            Serial.print("[AppCtrl] Current timer interval: ");
            Serial.print(timer_interval_ms / 60000);
            Serial.println(" minutes");
            break;
        case MSG_4003: // Get Elapsed Time Since Timer Started
            if (timer_start_timestamp_ms == 0)
            {
                Serial.println("[AppCtrl] Timer is not currently running");
            }
            else
            {
                u32 elapsed_ms = millis() - timer_start_timestamp_ms;
                u32 elapsed_seconds = elapsed_ms / 1000;
                u32 elapsed_minutes = elapsed_seconds / 60;
                u32 remaining_seconds = elapsed_seconds % 60;

                Serial.print("[AppCtrl] Timer running for: ");
                Serial.print(elapsed_minutes);
                Serial.print(" minutes, ");
                Serial.print(remaining_seconds);
                Serial.print(" seconds (of ");
                Serial.print(timer_interval_ms / 60000);
                Serial.println(" minutes total)");
            }
            break;
        default:
            // Unknown message ID
            ASSERT(false);
            break;
    }
}

static void prv_reset_sequence(void)
{
    g_mailbox.is_countdown_expired = false;
    g_run_sequence_once = false;
}

// ###########################################################################
// # Flash Storage Functions
// ###########################################################################

static void prv_load_settings_from_flash(void)
{
    prv_preferences.begin("appctrl", true); // Read-only mode

    // Load timer interval (default to DEFAULT_MINUTES if not found)
    timer_interval_ms = prv_preferences.getUInt("timer_ms", DEFAULT_MINUTES * 60 * 1000);

    prv_preferences.end();

    Serial.print("[AppCtrl] Loaded timer interval from flash: ");
    Serial.print(timer_interval_ms / 60000);
    Serial.println(" minutes");
}

static void prv_save_timer_interval_to_flash(void)
{
    prv_preferences.begin("appctrl", false); // Read-write mode

    prv_preferences.putUInt("timer_ms", timer_interval_ms);

    prv_preferences.end();

    Serial.println("[AppCtrl] Timer interval saved to flash");
}

static bool prv_is_desk_movement_allowed(void)
{
    // If time is not synchronized, allow movement (fail-safe)
    if (!networktime_is_synchronized())
    {
        if (prv_logging_enabled)
        {
            Serial.println("[AppCtrl] Time not synchronized, allowing desk movement");
        }
        return true;
    }

    int current_hour = networktime_get_current_hour();

    // Check if current hour is within allowed range (07:00 to 19:00)
    if (current_hour >= TIME_RESTRICTION_START_HOUR && current_hour < TIME_RESTRICTION_END_HOUR)
    {
        if (prv_logging_enabled)
        {
            Serial.print("[AppCtrl] Current hour ");
            Serial.print(current_hour);
            Serial.println(" - Desk movement allowed");
        }
        return true;
    }
    else
    {
        if (prv_logging_enabled)
        {
            Serial.print("[AppCtrl] Current hour ");
            Serial.print(current_hour);
            Serial.println(" - Desk movement NOT allowed (outside 07:00-19:00)");
        }
        return false;
    }
}
