#include "FunctionState.h"

FunctionState::FunctionState(int formal_parameter_count)
    : formal_parameter_count(formal_parameter_count),
      parameter_evaluation(std::vector<int>(formal_parameter_count)),
      parameter_metaprogramming(std::vector<int>(formal_parameter_count)),
      call_count(0) {}

void FunctionState::increment_parameter_metaprogramming(int position) {
    ++parameter_metaprogramming[position];
}

void FunctionState::increment_call() { ++call_count; }

void FunctionState::add_formal_parameter_usage_order(
    const std::vector<int> &usage_order) {

    std::stringstream order;

    for (auto const &position : usage_order) {
        order << "|" << position;
        ++parameter_evaluation[position];
    }

    for (size_t i = 0; i < parameter_usage_order.size(); ++i) {
        if (parameter_usage_order[i] == order.str()) {
            ++parameter_usage_order_count[i];
            return;
        }
    }
    parameter_usage_order.push_back(order.str());
    parameter_usage_order_count.push_back(1);
}
