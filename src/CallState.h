#ifndef PROMISE_DYNTRACER_CALL_STATE_H
#define PROMISE_DYNTRACER_CALL_STATE_H

#include "PromiseUse.h"
#include "State.h"
#include "utilities.h"

class CallState {
  public:
    fn_id_t fn_id;
    call_id_t call_id;

    explicit CallState(call_id_t call_id, fn_id_t fn_id,
                       std::size_t formal_parameter_count)
        : call_id{call_id}, fn_id{fn_id},
          formal_parameter_count_{formal_parameter_count},
          uses_{formal_parameter_count,
                std::vector<std::vector<int>>(
                    2, std::vector<int>(to_underlying_type(PromiseUse::Count),
                                        0))},
          order_{} {

        /* INFO - Reserve size to 15 bytes to prevent repeated string
         * allocations when forced arguments are added. This increases the
         * memory requirement but should speed up the program. */
        order_.reserve(15);
    }

    inline void add_use(std::size_t parameter, bool is_default,
                        PromiseUse use) {

        uses_[parameter][is_default][to_underlying_type(use)] = 1;

        if (use == PromiseUse::Force)
            order_.append("|").append(std::to_string(parameter));
    }

    const std::vector<std::vector<std::vector<int>>> &get_uses() const {
        return uses_;
    }

    const std::string &get_order() const { return order_; }

    const int get_formal_parameter_count() const {
        return formal_parameter_count_;
    }

  private:
    std::size_t formal_parameter_count_;
    std::vector<std::vector<std::vector<int>>> uses_;
    std::string order_;
};

inline std::ostream &operator<<(std::ostream &os, const CallState &call_state) {
    os << "CallState(" << call_state.fn_id << "," << call_state.call_id << ","
       << call_state.get_formal_parameter_count() << ")";
    return os;
}

#endif /* PROMISE_DYTRACER_CALL_STATE_H */
