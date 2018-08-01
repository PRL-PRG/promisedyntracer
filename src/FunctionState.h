#ifndef __FUNCTION_STATE_H__
#define __FUNCTION_STATE_H__
#include "State.h"

class FunctionState {
  public:
    std::vector<int> parameter_evaluation;
    std::vector<int> parameter_metaprogramming;
    std::vector<std::string> parameter_usage_order;
    std::vector<int> parameter_usage_order_count;
    int formal_parameter_count;
    size_t call_count;

    FunctionState(int formal_parameter_count);
    void increment_parameter_metaprogramming(int position);
    void increment_call();
    void add_formal_parameter_usage_order(const std::vector<int> &usage_order);
};

#endif /* __FUNCTION_STATE_H__ */
