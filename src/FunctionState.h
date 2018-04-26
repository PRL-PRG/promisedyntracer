#ifndef __FUNCTION_STATE_H__
#define __FUNCTION_STATE_H__
#include "State.h"
class FunctionState {
  public:
    std::vector<int> parameter_evaluation;
    std::vector<int> parameter_metaprogramming;
    std::vector<std::string> parameter_usage_order;
    std::vector<int> parameter_usage_order_count;
    size_t call_count;

    FunctionState(int formal_parameter_count)
        : parameter_evaluation(std::vector<int>(formal_parameter_count)),
          parameter_metaprogramming(std::vector<int>(formal_parameter_count)),
          call_count(0) {}

    inline void increment_parameter_evaluation(int position) {
        ++parameter_evaluation[position];
    }

    inline void increment_parameter_metaprogramming(int position) {
        ++parameter_metaprogramming[position];
    }

    inline void increment_call() { ++call_count; }

    inline void add_formal_parameter_usage_order(std::string order) {
        for (size_t i = 0; i < parameter_usage_order.size(); ++i) {
            if (parameter_usage_order[i] == order) {
                ++parameter_usage_order_count[i];
                return;
            }
        }
        parameter_usage_order.push_back(order);
        parameter_usage_order_count.push_back(1);
    }
};

#endif /* __FUNCTION_STATE_H__ */
