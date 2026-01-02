#include "DeskControl.h"
#include "custom_assert.h"
#include "MessageBroker.h"

// ###########################################################################
// # Private function declarations
// ###########################################################################
static void prv_msg_callback(const msg_t *const message);

// ###########################################################################
// # Private variables
// ###########################################################################

// ###########################################################################
// # Public function implementations
// ###########################################################################

void deskcontrol_init(void)
{
    // Subscribe to relevant messages
    messagebroker_subscribe(MSG_1001, prv_msg_callback);
    messagebroker_subscribe(MSG_1002, prv_msg_callback);
    messagebroker_subscribe(MSG_1003, prv_msg_callback);
    messagebroker_subscribe(MSG_1004, prv_msg_callback);
}

void deskcontrol_run(void)
{
    // Execute the commands, which were issued via messages
}

// ###########################################################################
// # Private function implementations
// ###########################################################################
void prv_msg_callback(const msg_t *const message)
{
    ASSERT(message != NULL);

    switch (message->msg_id)
    {
    case MSG_1001:
        // Handle Desk Move Up
        break;
    case MSG_1002:
        // Handle Desk Move Down
        break;
    case MSG_1003:
        // Handle Desk move to P1 Preset
        break;
    case MSG_1004:
        // Handle Desk move to P2 Preset
        break;
    default:
        // Unknown message ID
        ASSERT(false);
        break;
    }
}