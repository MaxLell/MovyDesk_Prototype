#include "MessageBroker.h"
#include "custom_assert.h"

static msg_topic_t *topic_library[E_TOPIC_LAST_TOPIC] = {NULL};
static msg_topic_t topics[E_TOPIC_LAST_TOPIC] = {0};

static bool is_initialized = false;

void messagebroker_init(void)
{
    ASSERT(!is_initialized);

    for (u16 msg_id = (E_TOPIC_FIRST_TOPIC + 1); msg_id < E_TOPIC_LAST_TOPIC; msg_id++)
    {
        topics[msg_id].msg_id = msg_id;

        for (u16 i = 0; i < MESSAGE_BROKER_CALLBACK_ARRAY_SIZE; i++)
        {
            topics[msg_id].callback_array[i] = NULL;
        }

        topic_library[msg_id] = &topics[msg_id];
    }
    is_initialized = true;
}

void messagebroker_subscribe(msg_id_e topic, msg_callback_t in_function_ptr)
{
    { // Input Checks
        ASSERT(topic > E_TOPIC_FIRST_TOPIC);
        ASSERT(topic < E_TOPIC_LAST_TOPIC);
        ASSERT(in_function_ptr != NULL);
        ASSERT(is_initialized);
    }

    bool is_subscribed = false;
    bool is_already_subscribed = false;

    for (u16 i = 0; i < MESSAGE_BROKER_CALLBACK_ARRAY_SIZE; i++)
    {
        if (topic_library[topic]->callback_array[i] == NULL)
        {
            topic_library[topic]->callback_array[i] = in_function_ptr;
            is_subscribed = true;
            break;
        }
        else if (topic_library[topic]->callback_array[i] == in_function_ptr)
        {
            is_already_subscribed = true;
            break;
        }
    }

    ASSERT(is_subscribed);
}

void messagebroker_publish(const msg_t *const message)
{
    { // Input Checks
        ASSERT(is_initialized);
        ASSERT(message != NULL);
        ASSERT(message->msg_id > E_TOPIC_FIRST_TOPIC);
        ASSERT(message->msg_id < E_TOPIC_LAST_TOPIC);
    }

    bool message_is_published = false;

    msg_id_e topic = message->msg_id;

    for (u8 i = 0; i < MESSAGE_BROKER_CALLBACK_ARRAY_SIZE; i++)
    {
        msg_callback_t callback = topic_library[topic]->callback_array[i];
        if (callback != NULL)
        {
            callback(message);
            message_is_published = true;
        }
    }
    ASSERT(message_is_published);
}
