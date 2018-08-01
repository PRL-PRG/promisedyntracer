#ifndef __PROMISE_STATE_H__
#define __PROMISE_STATE_H__

#include "State.h"
#include "utilities.h"

class PromiseState {
  public:
    enum class SlotMutation {
        ENVIRONMENT_LOOKUP = 0,
        ENVIRONMENT_ASSIGN,
        EXPRESSION_LOOKUP,
        EXPRESSION_ASSIGN,
        VALUE_LOOKUP,
        VALUE_ASSIGN,
        COUNT
    };

    bool local;
    bool argument;
    prom_id_t id;
    env_id_t env_id;
    fn_id_t fn_id;
    call_id_t call_id;
    int formal_parameter_position;
    bool default_argument;
    bool evaluated;
    std::vector<int> mutations;

    PromiseState(prom_id_t id, env_id_t env_, bool local);

    void make_function_argument(fn_id_t fn_id, call_id_t call_id,
                                int formal_parameter_position,
                                bool default_argument);

    inline void increment_mutation_slot(SlotMutation slot_mutation) {
        ++mutations[to_underlying_type(slot_mutation)];
    }
};

std::string to_string(PromiseState::SlotMutation slot_mutation);

#endif /* __PROMISE_STATE_H__ */
