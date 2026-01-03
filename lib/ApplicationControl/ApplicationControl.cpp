#include "ApplicationControl.h"
#include <Arduino.h>
#include "FSM.h"
#include "MessageBroker.h"
#include "custom_assert.h"
#include "custom_types.h"

// ###########################################################################
// # Internal Configuration
// ###########################################################################

typedef struct
{
    bool is_person_present;
    bool is_person_absent;
    bool is_countdown_expired;
} app_ctrl_event_mailbox_t;

/*********************************************
 * State Definitions
 *********************************************/
typedef enum
{
    STATE_IDLE = 0,
    STATE_ENTRY_MOVE_SEQ,
    STATE_EXEC_MOVE_SEQ,
    STATE_INVALID,
    STATE_LAST
} app_ctrl_state_e;

/*********************************************
 * State Transition Events
 *********************************************/
typedef enum
{
    EVENT_PERSON_PRESENT = 0,
    EVENT_PERSON_ABSENT,
    EVENT_COUNTDOWN_FINISHED,
    EVENT_LAST
} app_ctrl_event_e;

/*********************************************
 * State Actions
 *********************************************/
static void prv_action_idle(void);
static void prv_entry_move_seq(void);
static void prv_action_exec_move_seq(void);
static void prv_invalid_state(void);

static const fsm_state_action_cb state_actions[] = {
    [STATE_IDLE] = prv_action_idle,
    [STATE_ENTRY_MOVE_SEQ] = prv_entry_move_seq,
    [STATE_EXEC_MOVE_SEQ] = prv_action_exec_move_seq,
    [STATE_INVALID] = prv_invalid_state,
};

/*********************************************
 * State Transition Matrix
 *********************************************/
static const u16 app_ctrl_transition_matrix[STATE_LAST][EVENT_LAST] = {
    [STATE_IDLE] =
        {
            [EVENT_PERSON_PRESENT] = STATE_ENTRY_MOVE_SEQ,
            [EVENT_PERSON_ABSENT] = STATE_IDLE,
            [EVENT_COUNTDOWN_FINISHED] = STATE_INVALID,
        },
    [STATE_ENTRY_MOVE_SEQ] =
        {
            [EVENT_PERSON_PRESENT] = STATE_EXEC_MOVE_SEQ,
            [EVENT_PERSON_ABSENT] = STATE_EXEC_MOVE_SEQ,
            [EVENT_COUNTDOWN_FINISHED] = STATE_INVALID,
        },
    [STATE_EXEC_MOVE_SEQ] =
        {
            [EVENT_PERSON_PRESENT] = STATE_EXEC_MOVE_SEQ,
            [EVENT_PERSON_ABSENT] = STATE_IDLE,
            [EVENT_COUNTDOWN_FINISHED] = STATE_ENTRY_MOVE_SEQ,
        },
    [STATE_INVALID] =
        {
            [EVENT_PERSON_PRESENT] = STATE_INVALID,
            [EVENT_PERSON_ABSENT] = STATE_INVALID,
            [EVENT_COUNTDOWN_FINISHED] = STATE_INVALID,
        },
};

/*********************************************
 * Generate the FSM Configuration
 *********************************************/
static fsm_config_t app_ctrl_fsm_config = {
    .number_of_states = STATE_LAST,
    .number_of_events = EVENT_LAST,
    .transition_matrix = (const u16*)app_ctrl_transition_matrix,
    .state_actions = state_actions,
    .current_state = STATE_IDLE,
    .current_event = EVENT_PERSON_ABSENT,
};

/*********************************************
 * Generate the Event Mailbox
 *********************************************/
static app_ctrl_event_mailbox_t app_ctrl_event_mailbox = {
    .is_person_present = false,
    .is_person_absent = false,
    .is_countdown_expired = false,
};

// ###########################################################################
// # Private function declarations
// ###########################################################################
static void prv_msg_broker_callback(const msg_t* const message);
static bool prv_get_event_from_mailbox(app_ctrl_event_e in_event);
static void prv_clear_event_mailbox(app_ctrl_event_e in_event);

// ###########################################################################
// # Private variables
// ###########################################################################

// Logging control
static bool prv_logging_enabled = false;

// TODO: Add application state variables here

// ###########################################################################
// # Public function implementations
// ###########################################################################

void applicationcontrol_init(void)
{
    // Subscribe to 2001, 2002, 3003
    messagebroker_subscribe(MSG_2001, prv_msg_broker_callback); // Presence Detected
    messagebroker_subscribe(MSG_2002, prv_msg_broker_callback); // No Presence Detected
    messagebroker_subscribe(MSG_3003, prv_msg_broker_callback); // Countdown finished
    messagebroker_subscribe(MSG_0003, prv_msg_broker_callback); // Set Logging State
}

void applicationcontrol_run(void)
{
    // TODO: Implement main application control logic
}

// ###########################################################################
// # Private function implementations
// ###########################################################################

static void prv_msg_broker_callback(const msg_t* const message)
{
    ASSERT(message != NULL);

    switch (message->msg_id)
    {
        case MSG_2001: // Presence Detected
            app_ctrl_event_mailbox.is_person_present = true;
            if (prv_logging_enabled)
            {
                Serial.println("[AppCtrl] Event: Presence Detected");
            }
            break;
        case MSG_2002: // No Presence Detected
            app_ctrl_event_mailbox.is_person_absent = true;
            if (prv_logging_enabled)
            {
                Serial.println("[AppCtrl] Event: No Presence Detected");
            }
            break;
        case MSG_3003: // Countdown finished
            app_ctrl_event_mailbox.is_countdown_expired = true;
            if (prv_logging_enabled)
            {
                Serial.println("[AppCtrl] Event: Countdown Finished");
            }
            break;
        case MSG_0003: // Set Logging State
            if (message->data_size == sizeof(bool))
            {
                prv_logging_enabled = *(bool*)(message->data_bytes);
                Serial.print("[AppCtrl] Logging ");
                Serial.println(prv_logging_enabled ? "enabled" : "disabled");
            }
            break;
        default:
            // Unknown message ID
            ASSERT(false);
            break;
    }
}

static bool prv_get_event_from_mailbox(app_ctrl_event_e in_event)
{
    bool did_event_occur = false;
    switch (in_event)
    {
        case EVENT_PERSON_PRESENT: return app_ctrl_event_mailbox.is_person_present;
        case EVENT_PERSON_ABSENT: return app_ctrl_event_mailbox.is_person_absent;
        case EVENT_COUNTDOWN_FINISHED: return app_ctrl_event_mailbox.is_countdown_expired;
        default: ASSERT(false);
    }
    return did_event_occur;
}

static void prv_clear_event_mailbox(app_ctrl_event_e in_event)
{
    switch (in_event)
    {
        case EVENT_PERSON_PRESENT: app_ctrl_event_mailbox.is_person_present = false; break;
        case EVENT_PERSON_ABSENT: app_ctrl_event_mailbox.is_person_absent = false; break;
        case EVENT_COUNTDOWN_FINISHED: app_ctrl_event_mailbox.is_countdown_expired = false; break;
        default: ASSERT(false); break;
    }
}

static void prv_action_idle(void)
{
    // Get the current event from the mailbox - is a person present
    bool is_person_present = prv_get_event_from_mailbox(EVENT_PERSON_PRESENT);
    bool is_countdown_finished = prv_get_event_from_mailbox(EVENT_COUNTDOWN_FINISHED);
    bool is_person_absent = prv_get_event_from_mailbox(EVENT_PERSON_ABSENT);

    if (is_person_present)
    {
        if (prv_logging_enabled)
        {
            Serial.println("[AppCtrl] State: IDLE -> Transition to ENTRY_MOVE_SEQ");
        }
        // Transition to ENTRY_MOVE_SEQ state
        fsm_set_event(&app_ctrl_fsm_config, EVENT_PERSON_PRESENT);
    }
    else if (is_countdown_finished)
    {
        if (prv_logging_enabled)
        {
            Serial.println("[AppCtrl] State: IDLE -> Invalid transition (Countdown finished)");
        }
        // Invalid event -> Moves to the invalid state
        fsm_set_event(&app_ctrl_fsm_config, EVENT_COUNTDOWN_FINISHED);
    }
    else if (is_person_absent)
    {
        // Remain in IDLE state
        fsm_set_event(&app_ctrl_fsm_config, EVENT_PERSON_ABSENT);
    }
    else
    {
        // No event - remain in IDLE state
        delay(1000);
    }
}

static void prv_entry_move_seq(void)
{
    if (prv_logging_enabled)
    {
        Serial.println("[AppCtrl] State: ENTRY_MOVE_SEQ");
    }

    // Get a random number between 45 and 55 Minutes
    uint32_t countdown_time_ms = (random(45, 56)) * 60 * 1000; // Convert minutes to milliseconds

    if (prv_logging_enabled)
    {
        Serial.print("[AppCtrl] Starting countdown: ");
        Serial.print(countdown_time_ms / 60000);
        Serial.println(" minutes");
    }

    // Publish a message to start the countdown timer
    msg_t timer_msg;
    timer_msg.msg_id = MSG_3001; // Start countdown
    timer_msg.data_size = sizeof(uint32_t);
    timer_msg.data_bytes = (u8*)&countdown_time_ms;
    messagebroker_publish(&timer_msg);

    if (prv_logging_enabled)
    {
        Serial.println("[AppCtrl] State: ENTRY_MOVE_SEQ -> Transition to EXEC_MOVE_SEQ");
    }
    // Transition to EXEC_MOVE_SEQ state
    fsm_set_event(&app_ctrl_fsm_config, EVENT_PERSON_PRESENT);
}

static void prv_action_exec_move_seq(void)
{
    // Check for events in the mailbox
    bool is_person_absent = prv_get_event_from_mailbox(EVENT_PERSON_ABSENT);
    bool is_countdown_finished = prv_get_event_from_mailbox(EVENT_COUNTDOWN_FINISHED);

    if (is_person_absent)
    {
        if (prv_logging_enabled)
        {
            Serial.println("[AppCtrl] State: EXEC_MOVE_SEQ -> Stopping countdown, transitioning to IDLE");
        }
        // Stop the countdown timer
        msg_t timer_msg;
        timer_msg.msg_id = MSG_3002; // Stop countdown
        timer_msg.data_size = 0;
        timer_msg.data_bytes = NULL;
        messagebroker_publish(&timer_msg);

        // Transition to IDLE state
        fsm_set_event(&app_ctrl_fsm_config, EVENT_PERSON_ABSENT);
    }
    else if (is_countdown_finished)
    {
        if (prv_logging_enabled)
        {
            Serial.println("[AppCtrl] State: EXEC_MOVE_SEQ -> Toggling desk position");
        }
        // Send a message to toggle the desk position
        msg_t desk_msg;
        desk_msg.msg_id = MSG_1009; // Toggle Desk Position
        desk_msg.data_size = 0;
        desk_msg.data_bytes = NULL;
        messagebroker_publish(&desk_msg);

        if (prv_logging_enabled)
        {
            Serial.println("[AppCtrl] State: EXEC_MOVE_SEQ -> Transition to ENTRY_MOVE_SEQ");
        }
        // Transition to ENTRY_MOVE_SEQ state
        fsm_set_event(&app_ctrl_fsm_config, EVENT_COUNTDOWN_FINISHED);
    }
    else
    {
        // No event - remain in EXEC_MOVE_SEQ state
        delay(1000);
    }
}

static void prv_invalid_state(void)
{
    if (prv_logging_enabled)
    {
        Serial.println("[AppCtrl] ERROR: Invalid state reached!");
    }
    ASSERT(false);
    // TODO: Implement invalid state handling
}
