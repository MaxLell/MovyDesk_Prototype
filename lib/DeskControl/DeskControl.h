#ifndef DESKCONTROL_H
#define DESKCONTROL_H

// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     * @brief Creates and starts the DeskControl task
     * @return Task handle for the created task
     */
    TaskHandle_t deskcontrol_create_task(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // DESKCONTROL_H