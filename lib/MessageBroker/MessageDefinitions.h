#ifndef MESSAGE_DEFINITIONS_H
#define MESSAGE_DEFINITIONS_H

#include "custom_types.h"

/*********************************************
 * Countdown Timer Message Protocol
 ********************************************/
typedef struct
{
    u32 timestamp_sec; // Time stamp in seconds
} msg_countdown_timestamp_t;

#endif // MESSAGE_DEFINITIONS_H
