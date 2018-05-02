#ifndef __ARGUMENT_PROMISE_STATE_H__
#define __ARGUMENT_PROMISE_STATE_H__

#include "State.h"

class ArgumentPromiseState {
  public:
    fn_id_t fn_id;
    call_id_t call_id;
    int formal_parameter_position;
    int actual_argument_position;
    bool default_argument;
    int evaluation_context;
    bool evaluated;

    ArgumentPromiseState(fn_id_t fn_id, call_id_t call_id,
                         int formal_parameter_position,
                         int actual_argument_position, bool default_argument,
                         int evaluation_context, bool evaluated = false)
        : fn_id(fn_id), call_id(call_id),
          formal_parameter_position(formal_parameter_position),
          actual_argument_position(actual_argument_position),
          default_argument(default_argument),
          evaluation_context(evaluation_context), evaluated(evaluated) {}
};

#endif /* __ARGUMENT_PROMISE_STATE_H__ */
