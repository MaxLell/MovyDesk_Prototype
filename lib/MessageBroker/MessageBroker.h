#ifndef MESSAGEBROKER_H
#define MESSAGEBROKER_H

#include "MessageIDs.h"
#include "custom_types.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    typedef struct
    {
        msg_id_e msg_id;
        u16 data_size;
        u8* data_bytes;
    } msg_t;

    typedef void (*msg_callback_t)(const msg_t* const message);

    void messagebroker_init(void);

    void messagebroker_subscribe(msg_id_e topic, msg_callback_t callback);

    void messagebroker_publish(const msg_t* const message);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // MESSAGEBROKER_H
