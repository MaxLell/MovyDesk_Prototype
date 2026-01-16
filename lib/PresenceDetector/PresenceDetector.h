#ifndef PRESENCEDETECTOR_H
#define PRESENCEDETECTOR_H

// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     * @brief Creates and starts the PresenceDetector task
     * @return Task handle for the created task
     */
    TaskHandle_t presencedetector_create_task(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // PRESENCEDETECTOR_H