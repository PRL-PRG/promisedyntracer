#ifndef __CALL_STATE_H__
#define __CALL_STATE_H__

#include "State.h"

class CallState {
  public:
    fn_id_t fn_id;
    call_id_t call_id;

    CallState(call_id_t call_id, fn_id_t fn_id)
        : call_id{call_id}, fn_id{fn_id} {}

    inline void
    update_formal_parameter_usage_order(int formal_parameter_position) {
        /* if the argument has already been added, then
           don't add it again. This is useful when handling
           ... args. All ... args will get same formal parameter position.
           If any ... arg is forced, assume that the particular formal
           parameter position is forced, but don't repeat the position.
         */
        for (auto const &position : formal_parameter_usage_order_) {
            if (position == formal_parameter_position)
                return;
        }
        formal_parameter_usage_order_.push_back(formal_parameter_position);
    }

    const std::vector<int> &get_formal_parameter_usage_order() const {
        return formal_parameter_usage_order_;
    }

  private:
    std::vector<int> formal_parameter_usage_order_;
};

#endif /* __CALL_STATE_H__ */
