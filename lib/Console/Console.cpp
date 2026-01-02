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
#include "custom_assert.h"
#include "custom_types.h"

#include "Cli.h"

// Include Arduino Serial for I/O
#include <Arduino.h>

// ###########################################################################
// # Private function declarations
// ###########################################################################
static int prv_cmd_hello_world(int argc, char *argv[], void *context);
static int prv_cmd_echo_string(int argc, char *argv[], void *context);
static int prv_cmd_display_args(int argc, char *argv[], void *context);
static int prv_cmd_system_info(int argc, char *argv[], void *context);
static int prv_cmd_reset_system(int argc, char *argv[], void *context);

static int prv_console_put_char(char in_char);
static char prv_console_get_char(void);
static void prv_assert_failed(const char *file, uint32_t line, const char *expr);

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
    {"hello", prv_cmd_hello_world, NULL, "Say hello"},
    {"args", prv_cmd_display_args, NULL, "Displays the given cli arguments"},
    {"echo", prv_cmd_echo_string, NULL, "Echoes the given string"},
    {"system_info", prv_cmd_system_info, NULL, "Show system information"},
    {"restart", prv_cmd_reset_system, NULL, "Restart the system"},
};

// ###########################################################################
// # Public function implementations
// ###########################################################################

void console_init(void)
{
    ASSERT(!is_initialized);

    // Initialize Serial communication
    Serial.begin(115200);

    u16 wait_count_ms = 0;
    while (!Serial)
    {
        wait_count_ms += 10;
        delay(10);
        ASSERT(wait_count_ms < 5000); // Wait max 5 seconds for Serial to initialize
    }

    /**
     * Hands over the statically allocated cli_cfg_t struct.
     * The Cli's memory is to be managed by the user. Internally the cli only works with a
     * reference to this allocated memory. (There is a lot of sanity checking with ASSERTs
     * on the state of this memory).
     */
    cli_init(&g_cli_cfg, prv_console_put_char);

    /**
     * Register all external command bindings - these are the ones listed here in this demo
     * There are some internal command bindings too - like for example the clear, help and reset
     * binding.
     */
    for (size_t i = 0; i < CLI_GET_ARRAY_SIZE(cli_bindings); i++)
    {
        cli_register(&cli_bindings[i]);
    }

    cli_print("Console initialized. Type 'help' for available commands.\n");
    cli_print("> ");

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
// = Commands
// ============================

static int prv_cmd_hello_world(int argc, char *argv[], void *context)
{
    (void)argc;
    (void)argv;
    (void)context;
    cli_print("Hello World from MovyDesk!\n");
    return CLI_OK_STATUS;
}

static int prv_cmd_echo_string(int argc, char *argv[], void *context)
{
    if (argc != 2)
    {
        cli_print("Usage: echo <string>\n");
        cli_print("Give one argument to echo\n");
        return CLI_FAIL_STATUS;
    }
    (void)context;
    cli_print("-> %s\n", argv[1]);
    return CLI_OK_STATUS;
}

static int prv_cmd_display_args(int argc, char *argv[], void *context)
{
    cli_print("Number of arguments: %d\n", argc);
    for (int i = 0; i < argc; i++)
    {
        cli_print("argv[%d] --> \"%s\"\n", i, argv[i]);
    }

    (void)context;
    return CLI_OK_STATUS;
}

static int prv_cmd_system_info(int argc, char *argv[], void *context)
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

    cli_print("* Temperature: %.1f°C", (temperatureRead() - 32.0) * 5.0 / 9.0);
    return CLI_OK_STATUS;
}

static int prv_cmd_reset_system(int argc, char *argv[], void *context)
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
