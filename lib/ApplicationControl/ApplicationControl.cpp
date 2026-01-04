#include "ApplicationControl.h"
#include <Arduino.h>
#include "MessageBroker.h"
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

static u8 timer_interval_min = 51;

// ###########################################################################
// # Private function declarations
// ###########################################################################
static void prv_msg_broker_callback(const msg_t* const message);

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
}

void applicationcontrol_run(void)
{
    if (g_mailbox.is_person_present)
    {
        static bool do_once = false;
        if (do_once == false)
        {
            msg_t timer_msg;
            timer_msg.msg_id = 
        }
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
        default:
            // Unknown message ID
            ASSERT(false);
            break;
    }
}
