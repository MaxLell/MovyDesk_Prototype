#ifndef TIMERMANAGER_H
#define TIMERMANAGER_H

// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     * @brief Creates and starts the TimerManager task
     * @return Task handle for the created task
     */
    TaskHandle_t timermanager_create_task(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // TIMERMANAGER_H
