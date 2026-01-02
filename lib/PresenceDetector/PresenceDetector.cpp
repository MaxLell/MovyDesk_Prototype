#include "PresenceDetector.h"
#include "custom_assert.h"
#include "MessageBroker.h"
#include "custom_types.h"
#include <Arduino.h>

// ###########################################################################
// # Internal Configuration
// ###########################################################################

// ###########################################################################
// # Private Data
// ###########################################################################

static bool is_initialized = false;

// ###########################################################################
// # Private Function Declarations
// ###########################################################################

static void prv_msg_broker_callback(const msg_t *const message);

// ###########################################################################
// # Public Function Implementations
// ###########################################################################

void presencedetector_init(void)
{
    ASSERT(!is_initialized);

    // Initialize presence detector hardware and state here

    // Subscribe to relevant messages if needed
    messagebroker_subscribe(MSG_2003, prv_msg_broker_callback);

    is_initialized = true;
}

void presencedetector_run(void)
{
    ASSERT(is_initialized);

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
        // Handle List Close Devices
        // Implement logic to list close devices here
        break;
    default:
        // Unknown message ID
        ASSERT(false);
        break;
    }
}