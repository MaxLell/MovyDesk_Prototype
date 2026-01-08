/**
 * MIT License
 *
 * Copyright (c) <2025> <Max Koell (maxkoell@proton.me)>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @file Console.c
 * @brief Console module that initializes the CLI and registers demo commands for Arduino/RTOS environment.
 *
 * This file provides console functionality using the CLI library adapted for
 * Arduino framework with RTOS task support.
 */

#include "Console.h"
#include "Cli.h"
#include "MessageBroker.h"
#include "MessageDefinitions.h"
#include "custom_assert.h"
#include "custom_types.h"

// Include Arduino Serial for I/O
#include <Arduino.h>

// ###########################################################################
// # Private function declarations
// ###########################################################################
static int prv_console_put_char(char in_char);
static char prv_console_get_char(void);

// System Commands
static int prv_cmd_system_info(int argc, char* argv[], void* context);
static int prv_cmd_reset_system(int argc, char* argv[], void* context);

// Message Broker Test commands
static void prv_msg_broker_callback(const msg_t* const message);
static int prv_cmd_msgbroker_can_subscribe_and_publish(int argc, char* argv[], void* context);

// Desk Control Test Commands
static int prv_cmd_deskcontrol_move_command(int argc, char* argv[], void* context);

// Generic Logging Commands
static int prv_cmd_log_control(int argc, char* argv[], void* context);

// Presence Detector Test Commands
static int prv_cmd_pd_start_logging(int argc, char* argv[], void* context);
static int prv_cmd_pd_stop_logging(int argc, char* argv[], void* context);

// Timer Manager Test Commands
static int prv_cmd_timer_start_countdown(int argc, char* argv[], void* context);

// Application Control Commands
static int prv_cmd_appctrl_set_timer_interval(int argc, char* argv[], void* context);
static int prv_cmd_appctrl_get_timer_interval(int argc, char* argv[], void* context);

// ###########################################################################
// # Private Variables
// ###########################################################################

static bool is_initialized = false;

// embedded cli object - contains all data. This memory is to be managed by the user
static cli_cfg_t g_cli_cfg = {0};

/**
 * 'command name' - 'command handler' - 'pointer to context' - 'help string'
 *
 * - The command name is the ... name of the command
 * - The 'command handler' is the function pointer to the function that is to be called, when the command was successfully entered
 * - The 'pointer to context' is the context which the user can provide to his handler function - this can also be NULL
 * - The 'help string' is the string that is printed when the help command is executed. (Have a look at the Readme.md file for an example)
 */
static cli_binding_t cli_bindings[] = {
    // System Commands
    {"system_info", prv_cmd_system_info, NULL, "Show system information"},
    {"restart", prv_cmd_reset_system, NULL, "Restart the system"},

    // Message Broker Test Commands
    {"msgbroker_test", prv_cmd_msgbroker_can_subscribe_and_publish, NULL, "Test Message Broker subscribe and publish"},

    // Logging Commands
    {"log", prv_cmd_log_control, NULL, "Control module logging: log <on|off> <appctrl|deskctrl|presence>"},

    // Desk Control Commands
    {"move_desk", prv_cmd_deskcontrol_move_command, NULL, "Move desk: move_desk <up|down|p1|p2|p3|p4|wake|memory>"},

    // Timer Manager Commands
    {"test_timer", prv_cmd_timer_start_countdown, NULL, "Start countdown timer: test_timer <seconds>"},

    // Application Control Commands
    {"appctrl_set_time", prv_cmd_appctrl_set_timer_interval, NULL,
     "Sets a new timer interval: appctrl_set_time <minutes>"},
    {"appctrl_get_time", prv_cmd_appctrl_get_timer_interval, NULL,
     "Gets the current timer interval: appctrl_get_time"},

};

// ###########################################################################
// # Public function implementations
// ###########################################################################

void console_init(void)
{
    ASSERT(!is_initialized);

    // Initialize Serial communication
    Serial.begin(115200);

    cli_init(&g_cli_cfg, prv_console_put_char);

    // Register all commands
    for (size_t i = 0; i < CLI_GET_ARRAY_SIZE(cli_bindings); i++)
    {
        cli_register(&cli_bindings[i]);
    }

    // Subscribe to timer done message
    messagebroker_subscribe(MSG_3003, prv_msg_broker_callback);

    // Subscribe to test message
    messagebroker_subscribe(MSG_0001, prv_msg_broker_callback);

    is_initialized = true;
}

void console_run(void)
{
    ASSERT(is_initialized);

    // Check if there are characters available from Serial
    if (Serial.available() > 0)
    {
        // Get a character entered by the user
        char c = prv_console_get_char();

        // Add the character to a queue and process it
        cli_receive_and_process(c);
    }
}

// ###########################################################################
// # Private function implementations
// ###########################################################################

// ============================
// = Console I/O (Arduino Serial)
// ============================

static int prv_console_put_char(char in_char)
{
    Serial.write(in_char);
    return 1; // Success
}

static char prv_console_get_char(void)
{
    if (Serial.available() > 0)
    {
        return (char)Serial.read();
    }
    return 0; // No character available
}

// ============================
// = Commands
// ============================

static int prv_cmd_system_info(int argc, char* argv[], void* context)
{
    (void)argc;
    (void)argv;
    (void)context;

    // Basic system info
    cli_print("* Uptime: %lu ms", millis());
    cli_print("* Free Heap: %d bytes", ESP.getFreeHeap());
    cli_print("* CPU Frequency: %d MHz", ESP.getCpuFreqMHz());

    // Memory details
    cli_print("* Heap Size: %d bytes", ESP.getHeapSize());
    cli_print("* Min Free Heap: %d bytes", ESP.getMinFreeHeap());
    cli_print("* Max Alloc Heap: %d bytes", ESP.getMaxAllocHeap());

    // Chip information
    cli_print("* Chip Model: %s", ESP.getChipModel());
    cli_print("* Chip Revision: %d", ESP.getChipRevision());
    cli_print("* CPU Cores: %d", ESP.getChipCores());

    cli_print("* Temperature: %.1f°C", temperatureRead());
    return CLI_OK_STATUS;
}

static int prv_cmd_reset_system(int argc, char* argv[], void* context)
{
    (void)argc;
    (void)argv;
    (void)context;

    cli_print("Restarting system in ");
    for (int i = 3; i > 0; i--)
    {
        cli_print("%d... ", i);
        delay(1000);
    }
    cli_print("\n");
    ESP.restart();

    return CLI_OK_STATUS;
}

static void prv_msg_broker_callback(const msg_t* const message)
{
    switch (message->msg_id)
    {
        case MSG_0001:
            cli_print("Message was received\n...");
            cli_print("Received message ID: %d, Size: %d", message->msg_id, message->data_size);
            cli_print("Message Content: %s", message->data_bytes);
            break;

        case MSG_3003: // Timer finished
            cli_print("*** Countdown timer completed successfully! ***");
            break;

        default: break;
    }
}

static int prv_cmd_msgbroker_can_subscribe_and_publish(int argc, char* argv[], void* context)
{
    (void)argc;
    (void)argv;
    (void)context;

    // Subscribe to a test message

    cli_print("Subscribed to MSG_0001 \n... \nNow publishing a test message. \n...");
    // Publish a test message
    msg_t test_msg;
    test_msg.msg_id = MSG_0001;
    const char* test_data = "The elephant has been tickled!";
    test_msg.data_size = strlen(test_data) + 1; // Including null terminator
    test_msg.data_bytes = (u8*)test_data;

    messagebroker_publish(&test_msg);

    return CLI_OK_STATUS;
}

// Desk Control Command Handlers
static int prv_cmd_deskcontrol_move_command(int argc, char* argv[], void* context)
{
    (void)context;

    if (argc != 2)
    {
        cli_print("Usage: move_desk <up|down|p1|p2|p3|p4|wake|memory|toggle>");
        return CLI_FAIL_STATUS;
    }

    const char* command = argv[1];
    static desk_command_e desk_cmd; // Static to persist after function returns

    // Parse command string to enum
    if (strcmp(command, "up") == 0)
    {
        desk_cmd = DESK_CMD_UP;
    }
    else if (strcmp(command, "down") == 0)
    {
        desk_cmd = DESK_CMD_DOWN;
    }
    else if (strcmp(command, "p1") == 0)
    {
        desk_cmd = DESK_CMD_PRESET1;
    }
    else if (strcmp(command, "p2") == 0)
    {
        desk_cmd = DESK_CMD_PRESET2;
    }
    else if (strcmp(command, "p3") == 0)
    {
        desk_cmd = DESK_CMD_PRESET3;
    }
    else if (strcmp(command, "p4") == 0)
    {
        desk_cmd = DESK_CMD_PRESET4;
    }
    else if (strcmp(command, "wake") == 0)
    {
        desk_cmd = DESK_CMD_WAKE;
    }
    else if (strcmp(command, "memory") == 0)
    {
        desk_cmd = DESK_CMD_MEMORY;
    }
    else if (strcmp(command, "toggle") == 0)
    {
        desk_cmd = DESK_CMD_TOGGLE;
    }
    else
    {
        cli_print("Unknown command: %s", command);
        cli_print("Valid commands: up, down, p1, p2, p3, p4, wake, memory, toggle");
        return CLI_FAIL_STATUS;
    }

    // Use MSG_1000 with the command enum as data
    msg_t desk_msg;
    desk_msg.msg_id = MSG_1000;
    desk_msg.data_size = sizeof(desk_command_e);
    desk_msg.data_bytes = (u8*)&desk_cmd;

    messagebroker_publish(&desk_msg);
    cli_print("Moving desk: %s", command);
    return CLI_OK_STATUS;
}

// Generic Logging Commands
static int prv_cmd_log_control(int argc, char* argv[], void* context)
{
    (void)context;

    if (argc < 3)
    {
        cli_print("Usage: log <on|off> <appctrl|deskctrl|presence>");
        return CLI_FAIL_STATUS;
    }

    // Parse enable/disable
    static bool enable_logging; // Static so it persists after function returns
    if (strcmp(argv[1], "on") == 0)
    {
        enable_logging = true;
    }
    else if (strcmp(argv[1], "off") == 0)
    {
        enable_logging = false;
    }
    else
    {
        cli_print("Error: First argument must be 'on' or 'off'");
        return CLI_FAIL_STATUS;
    }

    // Parse module name and get corresponding message ID
    msg_id_e msg_id;
    const char* module_name;

    if (strcmp(argv[2], "appctrl") == 0)
    {
        msg_id = MSG_0003;
        module_name = "ApplicationControl";
    }
    else if (strcmp(argv[2], "deskctrl") == 0)
    {
        msg_id = MSG_0004;
        module_name = "DeskControl";
    }
    else if (strcmp(argv[2], "presence") == 0)
    {
        msg_id = MSG_0005;
        module_name = "PresenceDetector";
    }
    else
    {
        cli_print("Error: Unknown module. Use 'appctrl', 'deskctrl', or 'presence'");
        return CLI_FAIL_STATUS;
    }

    // Publish logging control message
    msg_t log_msg;
    log_msg.msg_id = msg_id;
    log_msg.data_size = sizeof(bool);
    log_msg.data_bytes = (u8*)&enable_logging;

    messagebroker_publish(&log_msg);

    return CLI_OK_STATUS;
}

// Timer Manager Commands
static int prv_cmd_timer_start_countdown(int argc, char* argv[], void* context)
{
    (void)context;

    if (argc != 2)
    {
        cli_print("Usage: test_timer <seconds>");
        return CLI_FAIL_STATUS;
    }

    // Parse the seconds argument
    int seconds = atoi(argv[1]);
    if (seconds <= 0)
    {
        cli_print("Error: seconds must be a positive number");
        return CLI_FAIL_STATUS;
    }

    // Convert seconds to milliseconds
    uint32_t countdown_time_ms = (uint32_t)seconds * 1000;

    msg_t timer_msg;
    timer_msg.msg_id = MSG_3001; // Start countdown
    timer_msg.data_size = sizeof(uint32_t);
    timer_msg.data_bytes = (u8*)&countdown_time_ms;

    messagebroker_publish(&timer_msg);
    cli_print("Starting %d second countdown timer...", seconds);
    return CLI_OK_STATUS;
}

// Application Control Commands
static int prv_cmd_appctrl_set_timer_interval(int argc, char* argv[], void* context)
{
    (void)context;

    if (argc != 2)
    {
        cli_print("Usage: set_timer <minutes>");
        return CLI_FAIL_STATUS;
    }

    // Parse the minutes argument
    int minutes = atoi(argv[1]);
    if (minutes <= 0 || minutes > 255)
    {
        cli_print("Error: minutes must be between 1 and 255");
        return CLI_FAIL_STATUS;
    }

    // Store as static to persist beyond function scope
    static u32 timer_interval = 0;
    timer_interval = (u32)minutes * 60 * 1000; // Convert minutes to milliseconds

    // Publish message to ApplicationControl
    msg_t timer_msg;
    timer_msg.msg_id = MSG_4001; // Set Timer Interval
    timer_msg.data_size = sizeof(u32);
    timer_msg.data_bytes = (u8*)&timer_interval;

    messagebroker_publish(&timer_msg);
    cli_print("Timer interval set to %d minutes", minutes);
    return CLI_OK_STATUS;
}

static int prv_cmd_appctrl_get_timer_interval(int argc, char* argv[], void* context)
{
    (void)argc;
    (void)argv;
    (void)context;

    // Publish message to ApplicationControl requesting current timer interval
    msg_t get_timer_msg;
    get_timer_msg.msg_id = MSG_4002; // Get Timer Interval
    get_timer_msg.data_size = 0;
    get_timer_msg.data_bytes = NULL;

    messagebroker_publish(&get_timer_msg);
    return CLI_OK_STATUS;
}