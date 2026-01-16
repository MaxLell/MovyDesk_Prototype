#ifndef CONSOLE_H
#define CONSOLE_H

// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     * @brief Creates and starts the Console task
     * @return Task handle for the created task
     */
    TaskHandle_t console_create_task(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // CONSOLE_H