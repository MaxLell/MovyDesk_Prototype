#ifndef FSM_H
#define FSM_H

#include "custom_types.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    typedef void (*fsm_state_action_cb)(void);

    typedef struct
    {
        const u16 number_of_states;
        const u16 number_of_events;
        const u16* transition_matrix;
        const fsm_state_action_cb* state_actions;
        u16 current_state;
        u16 current_event;
    } fsm_config_t;

    void fsm_set_event(fsm_config_t* const config, u16 event);

    void fsm_get_next_state(fsm_config_t* const config);

    void fsm_run_state_action(const fsm_config_t* const config);

    void fsm_execute(fsm_config_t* const config);

    void fsm_check_config(const fsm_config_t* const config);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // FSM_H
