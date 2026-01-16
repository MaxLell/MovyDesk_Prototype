#ifndef APPLICATIONCONTROL_H
#define APPLICATIONCONTROL_H

// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     * @brief Creates and starts the ApplicationControl task
     * @return Task handle for the created task
     */
    TaskHandle_t applicationcontrol_create_task(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // APPLICATIONCONTROL_H
