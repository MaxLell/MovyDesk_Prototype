#include "DeskControl.h"
#include <Arduino.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "MessageBroker.h"
#include "custom_assert.h"

// ###########################################################################
// # Internal Configuration and Protocol Constants
// ###########################################################################

// ===== UART Pins Configuration =====
#define UART_TX_PIN          TX2
#define UART_RX_PIN          RX2
#define SERIAL_INTERFACE     Serial2

// Optional display pin
#define WAKEUP_PIN           4

// ===== Protocol Definitions =====
#define FRAME_LENGTH         8
#define REQUEST_FRAME_LENGTH 6
#define DEFAULT_REPEATS      5

// ===== Command Types =====
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
    DESK_CMD_PRESET4
} desk_command_e;

// Request frame (incoming from desk)
const uint8_t REQ_FRAME[REQUEST_FRAME_LENGTH] = {0x9B, 0x04, 0x11, 0x7C, 0xC3, 0x9D};

// Command frames (outgoing to desk)
const uint8_t CMD_WAKE[FRAME_LENGTH] = {0x9B, 0x06, 0x02, 0x00, 0x00, 0x6C, 0xA1, 0x9D};
const uint8_t CMD_UP[FRAME_LENGTH] = {0x9B, 0x06, 0x02, 0x01, 0x00, 0xFC, 0xA0, 0x9D};
const uint8_t CMD_DOWN[FRAME_LENGTH] = {0x9B, 0x06, 0x02, 0x02, 0x00, 0x0C, 0xA0, 0x9D};
const uint8_t CMD_M[FRAME_LENGTH] = {0x9B, 0x06, 0x02, 0x20, 0x00, 0xAC, 0xB8, 0x9D};
const uint8_t CMD_PRESET1[FRAME_LENGTH] = {0x9B, 0x06, 0x02, 0x04, 0x00, 0xAC, 0xA3, 0x9D};
const uint8_t CMD_PRESET2[FRAME_LENGTH] = {0x9B, 0x06, 0x02, 0x08, 0x00, 0xAC, 0xA6, 0x9D};
const uint8_t CMD_PRESET3[FRAME_LENGTH] = {0x9B, 0x06, 0x02, 0x10, 0x00, 0xAC, 0xAC, 0x9D};
const uint8_t CMD_PRESET4[FRAME_LENGTH] = {0x9B, 0x06, 0x02, 0x00, 0x01, 0xAC, 0x60, 0x9D};

// ###########################################################################
// # Private function declarations
// ###########################################################################
static void prv_msg_broker_callback(const msg_t* const message);
static void prv_set_frame(const uint8_t* f);
static void prv_disarm(void);
static void prv_arm_with(const uint8_t* f);
static void prv_push_req_byte(uint8_t byte);
static bool prv_req_match(void);
static const uint8_t* prv_get_command_frame(desk_command_e cmd);
static void prv_execute_command(desk_command_e cmd);

// ###########################################################################
// # Private variables
// ###########################################################################

// State variables
static uint8_t current_frame[FRAME_LENGTH];
static bool armed = false;
static int default_repeats = DEFAULT_REPEATS;
static int repeats_remaining = 0;

// Request detection ring buffer
static uint8_t req_window[REQUEST_FRAME_LENGTH];
static size_t req_idx = 0;
static bool req_filled = false;

// ###########################################################################
// # Public function implementations
// ###########################################################################

void deskcontrol_init(void)
{
    // Initialize UART for desk communication
    pinMode(WAKEUP_PIN, OUTPUT);
    digitalWrite(WAKEUP_PIN, LOW);

    // Initialize UART: 9600 baud, 8N1
    SERIAL_INTERFACE.begin(9600, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);

    // Initialize state
    armed = false;
    repeats_remaining = 0;
    req_idx = 0;
    req_filled = false;
    memset(req_window, 0, sizeof(req_window));
    memset(current_frame, 0, sizeof(current_frame));

    // Subscribe to relevant messages
    messagebroker_subscribe(MSG_1001, prv_msg_broker_callback);
    messagebroker_subscribe(MSG_1002, prv_msg_broker_callback);
    messagebroker_subscribe(MSG_1003, prv_msg_broker_callback);
    messagebroker_subscribe(MSG_1004, prv_msg_broker_callback);
    messagebroker_subscribe(MSG_1005, prv_msg_broker_callback);
    messagebroker_subscribe(MSG_1006, prv_msg_broker_callback);
    messagebroker_subscribe(MSG_1007, prv_msg_broker_callback);
    messagebroker_subscribe(MSG_1008, prv_msg_broker_callback);
}

void deskcontrol_run(void)
{
    // Process incoming UART data for request detection
    while (SERIAL_INTERFACE.available())
    {
        uint8_t byte = (uint8_t)SERIAL_INTERFACE.read();
        prv_push_req_byte(byte);

        if (prv_req_match())
        {
            if (armed && repeats_remaining > 0)
            {
                SERIAL_INTERFACE.write(current_frame, FRAME_LENGTH);
                repeats_remaining--;

                // After last repeat, disarm and drop DISPL_HIGH
                if (repeats_remaining == 0)
                {
                    prv_disarm();
                }
            }
            // Note: We don't print "REQ detected (not armed)" to avoid spam
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
        case MSG_1001:
            // Handle Desk Move Up
            prv_execute_command(DESK_CMD_UP);
            break;
        case MSG_1002:
            // Handle Desk Move Down
            prv_execute_command(DESK_CMD_DOWN);
            break;
        case MSG_1003:
            // Handle Desk move to P1 Preset
            prv_execute_command(DESK_CMD_PRESET1);
            break;
        case MSG_1004:
            // Handle Desk move to P2 Preset
            prv_execute_command(DESK_CMD_PRESET2);
            break;
        case MSG_1005:
            // Handle Desk Wake/Enable
            prv_execute_command(DESK_CMD_WAKE);
            break;
        case MSG_1006:
            // Handle Desk Memory/Store Position
            prv_execute_command(DESK_CMD_MEMORY);
            break;
        case MSG_1007:
            // Handle Desk move to P3 Preset
            prv_execute_command(DESK_CMD_PRESET3);
            break;
        case MSG_1008:
            // Handle Desk move to P4 Preset
            prv_execute_command(DESK_CMD_PRESET4);
            break;
        default:
            // Unknown message ID
            ASSERT(false);
            break;
    }
}

static void prv_set_frame(const uint8_t* f) { memcpy(current_frame, f, FRAME_LENGTH); }

static void prv_disarm(void)
{
    armed = false;
    repeats_remaining = 0;
    digitalWrite(WAKEUP_PIN, LOW);
}

static void prv_arm_with(const uint8_t* f)
{
    prv_set_frame(f);
    armed = true;
    repeats_remaining = default_repeats;
    digitalWrite(WAKEUP_PIN, HIGH);
}

static void prv_push_req_byte(uint8_t byte)
{
    req_window[req_idx] = byte;
    req_idx = (req_idx + 1) % REQUEST_FRAME_LENGTH;
    if (req_idx == 0)
    {
        req_filled = true;
    }
}

static bool prv_req_match(void)
{
    if (!req_filled)
    {
        return false;
    }

    // Compare in correct chronological order:
    // req_idx points to the oldest element position (next to overwrite)
    for (size_t i = 0; i < REQUEST_FRAME_LENGTH; i++)
    {
        size_t idx = (req_idx + i) % REQUEST_FRAME_LENGTH;
        if (req_window[idx] != REQ_FRAME[i])
        {
            return false;
        }
    }
    return true;
}

static const uint8_t* prv_get_command_frame(desk_command_e cmd)
{
    switch (cmd)
    {
        case DESK_CMD_WAKE: return CMD_WAKE;
        case DESK_CMD_UP: return CMD_UP;
        case DESK_CMD_DOWN: return CMD_DOWN;
        case DESK_CMD_MEMORY: return CMD_M;
        case DESK_CMD_PRESET1: return CMD_PRESET1;
        case DESK_CMD_PRESET2: return CMD_PRESET2;
        case DESK_CMD_PRESET3: return CMD_PRESET3;
        case DESK_CMD_PRESET4: return CMD_PRESET4;
        default: return NULL;
    }
}

static void prv_execute_command(desk_command_e cmd)
{
    const uint8_t* frame = prv_get_command_frame(cmd);
    if (frame != NULL)
    {
        prv_arm_with(frame);
    }
}