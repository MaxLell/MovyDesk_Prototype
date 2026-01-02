#include <Arduino.h>
#include "custom_assert.h"
#include "Console.h"
#include "MessageBroker.h"

// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ###########################################################################
// # Private function declarations
// ###########################################################################
void console_task(void *parameter);
static void prv_assert_failed(const char *file, uint32_t line, const char *expr);

// ###########################################################################
// # Task handles
// ###########################################################################
TaskHandle_t console_task_handle = NULL;

// ###########################################################################
// # Setup and Loop
// ###########################################################################

constexpr int LED_PIN = 2;

void setup()
{
  // Initialize custom assert
  custom_assert_init(prv_assert_failed);

  messagebroker_init();

  // Create console task
  xTaskCreate(
      console_task,        // Task function
      "ConsoleTask",       // Task name
      4096,                // Stack size (words)
      NULL,                // Task parameters
      1,                   // Task priority (0 = lowest, configMAX_PRIORITIES - 1 = highest)
      &console_task_handle // Task handle
  );

  Serial.println("System initialized. Console task started.");

  // Setup LED pin
  pinMode(LED_PIN, OUTPUT);
}

void loop()
{
  // Toggle LED - for a visual heartbeat
  static bool led_state = false;
  led_state = !led_state;
  digitalWrite(LED_PIN, led_state ? HIGH : LOW);
  delay(1000);
}

// ###########################################################################
// # Task implementations
// ###########################################################################

void console_task(void *parameter)
{
  (void)parameter; // Unused parameter

  // Initialize console
  console_init();

  // Task main loop
  while (1)
  {
    // Run the console processing
    console_run();

    delay(5); // Small delay to yield CPU
  }
}

// ###########################################################################
// # Private function implementations
// ###########################################################################

static void prv_assert_failed(const char *file, uint32_t line, const char *expr)
{
  Serial.printf("[ASSERT FAILED]: %s:%u - %s\n", file, line, expr);
  // In embedded systems, we might want to reset instead of infinite loop
  while (1)
  {
    delay(1000); // Keep watchdog happy if enabled
  }
}