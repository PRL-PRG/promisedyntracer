#ifndef __FUNCTION_STATE_H__
#define __FUNCTION_STATE_H__

#include "ParameterUse.h"
#include "State.h"
#include "utilities.h"

class FunctionState {
  public:
    FunctionState(std::size_t formal_parameter_count)
        : formal_parameter_count_{formal_parameter_count},
          default_parameter_uses_{formal_parameter_count},
          custom_parameter_uses_{formal_parameter_count}, call_count_{0} {}

    const std::size_t get_formal_parameter_count() const {
        return formal_parameter_count_;
    }

    const std::vector<ParameterUse> &get_custom_parameter_uses() const {
        return custom_parameter_uses_;
    }

    const std::vector<ParameterUse> &get_default_parameter_uses() const {
        return default_parameter_uses_;
    }

    void increment_call() { ++call_count_; }

    void add_order(const std::string &order) {
        for (size_t i = 0; i < orders_.size(); ++i) {
            if (orders_[i] == order) {
                ++order_counts_[i];
                return;
            }
        }
        orders_.push_back(order);
        order_counts_.push_back(1);
    }

    const std::vector<std::string> &get_orders() const { return orders_; }

    const std::vector<std::size_t> &get_order_counts() const {
        return order_counts_;
    }

  private:
    std::size_t formal_parameter_count_;
    std::size_t call_count_;
    std::vector<ParameterUse> default_parameter_uses_;
    std::vector<ParameterUse> custom_parameter_uses_;
    std::vector<std::string> orders_;
    std::vector<std::size_t> order_counts_;
};

#endif /* __FUNCTION_STATE_H__ */
