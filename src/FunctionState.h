#ifndef __FUNCTION_STATE_H__
#define __FUNCTION_STATE_H__

#include "PromiseUse.h"
#include "State.h"
#include "utilities.h"

class FunctionState {
  public:
    FunctionState(std::size_t formal_parameter_count)
        : formal_parameter_count_{formal_parameter_count},
          uses_{formal_parameter_count,
                std::vector<std::vector<int>>(
                    2, std::vector<int>(to_underlying_type(PromiseUse::Count),
                                        0))},
          call_count_{0} {}

    void increment_call() { ++call_count_; }

    void add_uses(const std::vector<std::vector<std::vector<int>>> &uses) {

        for (std::size_t parameter = 0; parameter < formal_parameter_count_;
             ++parameter) {
            for (std::size_t type = 0; type < 2; ++type) {
                for (std::size_t use = 0;
                     use < to_underlying_type(PromiseUse::Count); ++use) {
                    uses_[parameter][type][use] += uses[parameter][type][use];
                }
            }
        }
    }

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

    const std::vector<std::vector<std::vector<int>>> &get_uses() const {
        return uses_;
    }

    const std::vector<std::string> &get_orders() const { return orders_; }

    const std::vector<std::size_t> &get_order_counts() const {
        return order_counts_;
    }

  private:
    std::size_t formal_parameter_count_;
    std::size_t call_count_;
    std::vector<std::vector<std::vector<int>>> uses_;
    std::vector<std::string> orders_;
    std::vector<std::size_t> order_counts_;
};

#endif /* __FUNCTION_STATE_H__ */
