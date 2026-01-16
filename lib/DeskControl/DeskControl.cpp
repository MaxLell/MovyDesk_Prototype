#include "DeskControl.h"
#include <Arduino.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "MessageBroker.h"
#include "MessageDefinitions.h"
#include "custom_assert.h"

// ###########################################################################
// # Internal Configuration and Protocol Constants
// ###########################################################################

// ===== UART Pins Configuration =====
#define UART_TX_PIN          D6
#define UART_RX_PIN          D7
#define SERIAL_INTERFACE     Serial1

// Optional display pin
#define WAKEUP_PIN           D9

// ===== Protocol Definitions =====
#define FRAME_LENGTH         8
#define REQUEST_FRAME_LENGTH 6
#define DEFAULT_REPEATS      5
#define HEIGHT_MSG_ID        0x12
#define MAX_MSG_LENGTH       32

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
static void prv_deskcontrol_task(void* parameter);
static void prv_deskcontrol_init(void);
static void prv_deskcontrol_run(void);
static void prv_msg_broker_callback(const msg_t* const message);
static void prv_set_frame(const uint8_t* f);
static void prv_disarm(void);
static void prv_arm_with(const uint8_t* f);
static void prv_push_req_byte(uint8_t byte);
static bool prv_req_match(void);
static const uint8_t* prv_get_command_frame(desk_command_e cmd);
static void prv_execute_command(desk_command_e cmd);
static int prv_decode_digit(uint8_t byte);
static bool prv_has_decimal_point(uint8_t byte);
static void prv_parse_height_message(const uint8_t* msg, size_t len);

// ###########################################################################
// # Private variables
// ###########################################################################

// Logging control
static bool prv_logging_enabled = false;

// State variables
static uint8_t current_frame[FRAME_LENGTH];
static bool armed = false;
static int default_repeats = DEFAULT_REPEATS;
static int repeats_remaining = 0;

// Request detection ring buffer
static uint8_t req_window[REQUEST_FRAME_LENGTH];
static size_t req_idx = 0;
static bool req_filled = false;

// Last command executed
static desk_command_e g_last_toggle_position = DESK_CMD_PRESET1;

// Height tracking
static float g_current_height_cm = 0.0f;
static bool g_height_valid = false;
static uint8_t g_msg_buffer[MAX_MSG_LENGTH];
static size_t g_msg_buffer_idx = 0;
static bool g_in_message = false;

// ###########################################################################
// # Public function implementations
// ###########################################################################

TaskHandle_t deskcontrol_create_task(void)
{
    TaskHandle_t task_handle = NULL;

    xTaskCreate(prv_deskcontrol_task, // Task function
                "DeskControlTask",    // Task name
                4096,                 // Stack size (words)
                NULL,                 // Task parameters
                2,                    // Task priority
                &task_handle          // Task handle
    );

    return task_handle;
}

// ###########################################################################
// # Private function implementations
// ###########################################################################

static void prv_deskcontrol_task(void* parameter)
{
    (void)parameter; // Unused parameter

    // Initialize desk control
    prv_deskcontrol_init();

    // Task main loop
    while (1)
    {
        // Run the desk control processing
        prv_deskcontrol_run();

        delay(5);
    }
}

static void prv_deskcontrol_init(void)
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

    // Initialize height tracking
    g_current_height_cm = 0.0f;
    g_height_valid = false;
    g_msg_buffer_idx = 0;
    g_in_message = false;
    memset(g_msg_buffer, 0, sizeof(g_msg_buffer));

    // Subscribe to relevant messages
    messagebroker_subscribe(MSG_0004, prv_msg_broker_callback); // Logging control
    messagebroker_subscribe(MSG_1000, prv_msg_broker_callback); // desk command
    messagebroker_subscribe(MSG_1002, prv_msg_broker_callback); // get desk height
}

static void prv_deskcontrol_run(void)
{
    // Process incoming UART data for request detection and height messages
    while (SERIAL_INTERFACE.available())
    {
        uint8_t byte = (uint8_t)SERIAL_INTERFACE.read();

        // Check for start of message (0x9B)
        if (byte == 0x9B && !g_in_message)
        {
            g_in_message = true;
            g_msg_buffer_idx = 0;
            g_msg_buffer[g_msg_buffer_idx++] = byte;
        }
        else if (g_in_message)
        {
            if (g_msg_buffer_idx < MAX_MSG_LENGTH)
            {
                g_msg_buffer[g_msg_buffer_idx++] = byte;

                // Check if we have at least 2 bytes (start + length)
                if (g_msg_buffer_idx == 2)
                {
                    // We now know the expected message length
                }
                else if (g_msg_buffer_idx >= 2)
                {
                    uint8_t expected_len = g_msg_buffer[1];
                    // Full message received (start + length + data + end marker)
                    if (g_msg_buffer_idx >= (size_t)(expected_len + 2))
                    {
                        // Parse the message
                        if (expected_len >= 1 && g_msg_buffer[2] == HEIGHT_MSG_ID)
                        {
                            prv_parse_height_message(g_msg_buffer, g_msg_buffer_idx);
                        }

                        // Reset for next message
                        g_in_message = false;
                        g_msg_buffer_idx = 0;
                    }
                }
            }
            else
            {
                // Buffer overflow, reset
                g_in_message = false;
                g_msg_buffer_idx = 0;
            }
        }

        // Also feed into request detection
        prv_push_req_byte(byte);

        if (prv_req_match())
        {
            if (armed && repeats_remaining > 0)
            {
                if (prv_logging_enabled)
                {
                    Serial.print("[DeskCtrl] Sending frame, repeats left: ");
                    Serial.println(repeats_remaining - 1);
                }
                SERIAL_INTERFACE.write(current_frame, FRAME_LENGTH);
                repeats_remaining--;

                // After last repeat, disarm and drop DISPL_HIGH
                if (repeats_remaining == 0)
                {
                    if (prv_logging_enabled)
                    {
                        Serial.println("[DeskCtrl] Command sequence completed, disarming");
                    }
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
        case MSG_0004: // Set Logging State
            if (message->data_size == sizeof(bool))
            {
                prv_logging_enabled = *(bool*)(message->data_bytes);
                Serial.print("[DeskCtrl] Logging ");
                Serial.println(prv_logging_enabled ? "enabled" : "disabled");
            }
            break;

        case MSG_1000: // Unified desk command
            if (message->data_size == sizeof(desk_command_e) && message->data_bytes != NULL)
            {

                desk_command_e cmd = *(desk_command_e*)(message->data_bytes);

                ASSERT(cmd > DESK_CMD_NONE);
                ASSERT(cmd < DESK_CMD_LAST);

                if (cmd == DESK_CMD_TOGGLE)
                {
                    cmd = (g_last_toggle_position == DESK_CMD_PRESET1) ? DESK_CMD_PRESET2 : DESK_CMD_PRESET1;
                    g_last_toggle_position = cmd;
                }
                if (prv_logging_enabled)
                {
                    Serial.print("[DeskCtrl] Command: ");
                    Serial.println(cmd);
                }

                prv_execute_command(cmd);
            }
            break;

        case MSG_1002: // Get desk height
            if (g_height_valid)
            {
                Serial.print("[DeskCtrl] Current height: ");
                Serial.print(g_current_height_cm);
                Serial.println(" cm");
            }
            else
            {
                Serial.println("[DeskCtrl] Height not available yet");
            }
            break;

        default:
            // Unknown message ID
            if (prv_logging_enabled)
            {
                Serial.print("[DeskCtrl] Unknown message ID: ");
                Serial.println(message->msg_id);
            }
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

// ###########################################################################
// # Height Parsing Functions
// ###########################################################################

// Decode 7-segment display byte to digit (0-9)
static int prv_decode_digit(uint8_t b)
{
    // Extract 7-segment bits (ignore bit 7 which is decimal point)
    bool seg[8];
    for (int i = 0; i < 8; i++)
    {
        seg[i] = (b & (0x01 << i)) != 0;
    }

    // Decode based on 7-segment pattern
    if (seg[0] && seg[1] && seg[2] && seg[3] && seg[4] && seg[5] && !seg[6])
    {
        return 0;
    }
    if (!seg[0] && seg[1] && seg[2] && !seg[3] && !seg[4] && !seg[5] && !seg[6])
    {
        return 1;
    }
    if (seg[0] && seg[1] && !seg[2] && seg[3] && seg[4] && !seg[5] && seg[6])
    {
        return 2;
    }
    if (seg[0] && seg[1] && seg[2] && seg[3] && !seg[4] && !seg[5] && seg[6])
    {
        return 3;
    }
    if (!seg[0] && seg[1] && seg[2] && !seg[3] && !seg[4] && seg[5] && seg[6])
    {
        return 4;
    }
    if (seg[0] && !seg[1] && seg[2] && seg[3] && !seg[4] && seg[5] && seg[6])
    {
        return 5;
    }
    if (seg[0] && !seg[1] && seg[2] && seg[3] && seg[4] && seg[5] && seg[6])
    {
        return 6;
    }
    if (seg[0] && seg[1] && seg[2] && !seg[3] && !seg[4] && !seg[5] && !seg[6])
    {
        return 7;
    }
    if (seg[0] && seg[1] && seg[2] && seg[3] && seg[4] && seg[5] && seg[6])
    {
        return 8;
    }
    if (seg[0] && seg[1] && seg[2] && seg[3] && !seg[4] && seg[5] && seg[6])
    {
        return 9;
    }

    return -1; // Invalid digit
}

// Check if byte has decimal point set (bit 7)
static bool prv_has_decimal_point(uint8_t byte) { return (byte & 0x80) != 0; }

// Parse height message: 0x9B, len, 0x12, digit1, digit2, digit3, ...
static void prv_parse_height_message(const uint8_t* msg, size_t len)
{
    // Message format: [0]=0x9B, [1]=length, [2]=0x12, [3]=digit1, [4]=digit2, [5]=digit3
    if (len < 6)
    {
        return; // Message too short
    }

    int digit1 = prv_decode_digit(msg[3]);
    int digit2 = prv_decode_digit(msg[4]);
    int digit3 = prv_decode_digit(msg[5]);

    if (digit1 >= 0 && digit2 >= 0 && digit3 >= 0)
    {
        float height = digit1 * 100.0f + digit2 * 10.0f + digit3;

        // Check for decimal point in digit2
        if (prv_has_decimal_point(msg[4]))
        {
            height = height / 10.0f;
        }

        g_current_height_cm = height;
        g_height_valid = true;

        if (prv_logging_enabled)
        {
            Serial.print("[DeskCtrl] Height updated: ");
            Serial.print(g_current_height_cm);
            Serial.println(" cm");
        }
    }
    else
    {
        if (prv_logging_enabled)
        {
            Serial.println("[DeskCtrl] Failed to decode height digits");
        }
    }
}