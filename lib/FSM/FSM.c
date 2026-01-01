#include "FSM.h"
#include "custom_assert.h"

void fsm_check_config(const fsm_config_t *const config)
{
    // Check the config inputs
    // Null Pointer Checks for all entries
    ASSERT(config != NULL);
    ASSERT(config->transition_matrix != NULL);
    ASSERT(config->state_actions != NULL);

    // out of bounds checks for the Elements
    ASSERT(config->number_of_states > 0);
    ASSERT(config->number_of_events > 0);
    ASSERT(config->current_state < config->number_of_states);
    ASSERT(config->current_event < config->number_of_events);
}

void fsm_set_trigger_event(fsm_config_t *const config, u16 event)
{
    { // Input Checks
        fsm_check_config(config);
        ASSERT(event < config->number_of_events);
    }
    config->current_event = event;
}

void fsm_get_next_state(fsm_config_t *const config)
{
    { // Input Checks
        fsm_check_config(config);
    }

    // Approach taken from https://stackoverflow.com/a/54103595
    const u16 *matrix = config->transition_matrix;
    u16 num_events = config->number_of_events;
    u16 event = config->current_event;
    u16 state = config->current_state;

    config->current_state = *(matrix + (state * num_events) + event);

    ASSERT(config->current_state < config->number_of_states);
}

void fsm_run_state_action(const fsm_config_t *const config)
{
    { // Input Checks
        fsm_check_config(config);
    }
    u16 state = config->current_state;
    fsm_state_action_cb run_state_action = config->state_actions[state];
    ASSERT(run_state_action != NULL);
    run_state_action();
}

void fsm_execute(fsm_config_t *const config)
{
    { // Input Checks
        fsm_check_config(config);
    }
    fsm_get_next_state(config);
    fsm_run_state_action(config);
}
