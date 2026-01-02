#ifndef MESSAGEBROKER_H
#define MESSAGEBROKER_H

#include "custom_types.h"
#include "MessageIDs.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#define MESSAGE_BROKER_CALLBACK_ARRAY_SIZE 10U

  typedef void (*msg_callback_t)(const msg_t *const message);

  typedef struct
  {
    msg_id_e msg_id;
    u16 data_size;
    u8 *data_bytes;
  } msg_t;

  typedef struct
  {
    msg_id_e msg_id;
    msg_callback_t callback_array[MESSAGE_BROKER_CALLBACK_ARRAY_SIZE];
  } msg_topic_t;

  void messagebroker_init(void);

  void messagebroker_subscribe(msg_id_e topic,
                              msg_callback_t callback);

  void messagebroker_publish(const msg_t *const message);

  void messagebroker_check_config(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // MESSAGEBROKER_H
