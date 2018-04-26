#ifndef __ARGUMENT_PROMISE_STATE_H__
#define __ARGUMENT_PROMISE_STATE_H__

#include "State.h"

class ArgumentPromiseState {
  public:
    fn_id_t fn_id;
    call_id_t call_id;
    int formal_parameter_position;
    int actual_argument_position;

    ArgumentPromiseState(fn_id_t fn_id, call_id_t call_id,
                         int formal_parameter_position,
                         int actual_argument_position)
        : fn_id(fn_id), call_id(call_id),
          formal_parameter_position(formal_parameter_position),
          actual_argument_position(actual_argument_position) {}

};

#endif /* __ARGUMENT_PROMISE_STATE_H__ */
