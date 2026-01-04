#ifndef MESSAGE_DEFINITIONS_H
#define MESSAGE_DEFINITIONS_H

#include "custom_types.h"

/*********************************************
 * Desk Control Command Types
 ********************************************/
typedef enum
{
    DESK_CMD_NONE = 0,
    DESK_CMD_WAKE,
    DESK_CMD_UP,
    DESK_CMD_DOWN,
    DESK_CMD_MEMORY,
    DESK_CMD_PRESET1,
    DESK_CMD_PRESET2,
    DESK_CMD_PRESET3,
    DESK_CMD_PRESET4,
    DESK_CMD_TOGGLE,
    DESK_CMD_LAST
} desk_command_e;

/*********************************************
 * Countdown Timer Message Protocol
 ********************************************/
typedef struct
{
    u32 timestamp_sec; // Time stamp in seconds
} msg_countdown_timestamp_t;

#endif // MESSAGE_DEFINITIONS_H
