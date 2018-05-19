#include "FunctionState.h"

FunctionState::FunctionState(int formal_parameter_count)
    : formal_parameter_count(formal_parameter_count),
      parameter_evaluation(std::vector<int>(formal_parameter_count)),
      parameter_metaprogramming(std::vector<int>(formal_parameter_count)),
      call_count(0) {}

void FunctionState::increment_parameter_evaluation(int position) {
    assert(position < formal_parameter_count);
    ++parameter_evaluation[position];
}

void FunctionState::increment_parameter_metaprogramming(int position) {
    ++parameter_metaprogramming[position];
}

void FunctionState::increment_call() { ++call_count; }

void FunctionState::add_formal_parameter_usage_order(std::string order) {
    for (size_t i = 0; i < parameter_usage_order.size(); ++i) {
        if (parameter_usage_order[i] == order) {
            ++parameter_usage_order_count[i];
            return;
        }
    }
    parameter_usage_order.push_back(order);
    parameter_usage_order_count.push_back(1);
}
