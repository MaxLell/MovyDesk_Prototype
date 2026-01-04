#include "ApplicationControl.h"
#include <Arduino.h>
#include "MessageBroker.h"
#include "MessageDefinitions.h"
#include "custom_assert.h"
#include "custom_types.h"

// ###########################################################################
// # Internal Configuration
// ###########################################################################

typedef struct
{
    bool is_person_present;
    bool is_countdown_expired;
} prv_mailbox_t;

static prv_mailbox_t g_mailbox = {
    .is_person_present = false,
    .is_countdown_expired = false,
};

static u32 timer_interval_ms = 1 * 60 * 1000;
static bool g_run_sequence_once = false;

// ###########################################################################
// # Private function declarations
// ###########################################################################
static void prv_msg_broker_callback(const msg_t* const message);
static void prv_reset_sequence(void);

// ###########################################################################
// # Private variables
// ###########################################################################

// Logging control
static bool prv_logging_enabled = false;

// TODO: Add application state variables here

// ###########################################################################
// # Public function implementations
// ###########################################################################

void applicationcontrol_init(void)
{
    // Subscribe to 2001, 2002, 3003
    messagebroker_subscribe(MSG_2001, prv_msg_broker_callback); // Presence Detected
    messagebroker_subscribe(MSG_2002, prv_msg_broker_callback); // No Presence Detected
    messagebroker_subscribe(MSG_3003, prv_msg_broker_callback); // Countdown finished
    messagebroker_subscribe(MSG_0003, prv_msg_broker_callback); // Set Logging State
    messagebroker_subscribe(MSG_4001, prv_msg_broker_callback); // Set Timer Interval
}

void applicationcontrol_run(void)
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
            g_run_sequence_once = true;
        }

        if (g_mailbox.is_countdown_expired)
        {
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
        }
    }
    else
    {
        // Stop the countdown timer if running
        msg_t timer_msg;
        timer_msg.msg_id = MSG_3002; // Stop Countdown
        timer_msg.data_size = 0;
        timer_msg.data_bytes = NULL;
        messagebroker_publish(&timer_msg);

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
            if (prv_logging_enabled)
            {
                Serial.println("[AppCtrl] Event: Presence Detected");
            }
            break;
        case MSG_2002: // No Presence Detected
            g_mailbox.is_person_present = false;
            if (prv_logging_enabled)
            {
                Serial.println("[AppCtrl] Event: No Presence Detected");
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
                Serial.print("[AppCtrl] Timer interval set to ");
                Serial.print(timer_interval_ms / 60000);
                Serial.println(" minutes");

                // reset the sequence
                g_mailbox.is_countdown_expired = false;
                g_run_sequence_once = false;
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