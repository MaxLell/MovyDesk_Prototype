#include "ApplicationControl.h"
#include "custom_assert.h"
#include "MessageBroker.h"
#include <Arduino.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// ###########################################################################
// # Internal Configuration
// ###########################################################################

// TODO: Add application-specific configuration here

// ###########################################################################
// # Private function declarations
// ###########################################################################
static void prv_msg_broker_callback(const msg_t *const message);

// ###########################################################################
// # Private variables
// ###########################################################################

// TODO: Add application state variables here

// ###########################################################################
// # Public function implementations
// ###########################################################################

void applicationcontrol_init(void)
{
    // TODO: Initialize application-specific hardware/state

    // Subscribe to relevant messages
    // TODO: Subscribe to specific message IDs when defined
    // messagebroker_subscribe(MSG_XXXX, prv_msg_broker_callback);
}

void applicationcontrol_run(void)
{
    // TODO: Implement main application control logic
}

// ###########################################################################
// # Private function implementations
// ###########################################################################

static void prv_msg_broker_callback(const msg_t *const message)
{
    ASSERT(message != NULL);

    switch (message->msg_id)
    {
    // TODO: Add message handling cases here
    default:
        // Unknown message ID
        ASSERT(false);
        break;
    }
}
