#include <Arduino.h>
#include "custom_assert.h"
#include "Console.h"

// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ###########################################################################
// # Private function declarations
// ###########################################################################
void console_task(void* parameter);
static void prv_assert_failed(const char* file, uint32_t line, const char* expr);

// ###########################################################################
// # Task handles
// ###########################################################################
TaskHandle_t console_task_handle = NULL;

// ###########################################################################
// # Setup and Loop
// ###########################################################################

void setup() {
  // Initialize custom assert
  custom_assert_init(prv_assert_failed);
  
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
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000)); // Delay 1 second to prevent watchdog reset
}

// ###########################################################################
// # Task implementations
// ###########################################################################

void console_task(void* parameter) {
  (void)parameter; // Unused parameter

    // Initialize console
  console_init();
  
  // Task main loop
  while (1) {
    // Run the console processing
    console_run();
    
    delay(5); // Small delay to yield CPU
  }
}

// ###########################################################################
// # Private function implementations
// ###########################################################################

static void prv_assert_failed(const char* file, uint32_t line, const char* expr)
{
  Serial.printf("ASSERT FAILED: %s:%u - %s\n", file, line, expr);
  // In embedded systems, we might want to reset instead of infinite loop
  while (1)
  {
    delay(1000); // Keep watchdog happy if enabled
  }
}