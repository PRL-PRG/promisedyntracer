#ifndef __CALL_STATE_H__
#define __CALL_STATE_H__

#include "State.h"

class CallState {
  public:
    fn_id_t fn_id;
    call_id_t call_id;
    std::string formal_parameter_usage_order;

    CallState(call_id_t call_id, fn_id_t fn_id)
        : call_id(call_id), fn_id(fn_id) {}

    inline void
    update_formal_parameter_usage_order(int formal_parameter_position) {

        formal_parameter_usage_order.append("|");
        formal_parameter_usage_order.append(
            std::to_string(formal_parameter_position));
    }
};

#endif /* __CALL_STATE_H__ */
