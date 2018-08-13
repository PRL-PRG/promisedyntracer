#ifndef PROMISE_DYNTRACER_PARAMETER_USE_H
#define PROMISE_DYNTRACER_PARAMETER_USE_H

#include <cstdint>
#include <string>

class ParameterUse {
  public:
    explicit ParameterUse()
        : unpromised_(0), forced_(0), looked_up_(0), metaprogrammed_(0),
          metaprogrammed_and_looked_up_(0), used_(0) {}

    std::uint8_t get_unpromised() const { return unpromised_; }

    void unpromise() { unpromised_ = 1; }

    std::uint8_t get_forced() const { return forced_; }

    void force() {
        forced_ = 1;
        used_ = 1;
    }

    std::uint8_t get_looked_up() const { return looked_up_; }

    void lookup() {
        used_ = 1;
        if (get_metaprogrammed_and_looked_up()) {
            return;
        } else if (get_metaprogrammed()) {
            metaprogram_and_look_up();
        } else {
            looked_up_ = 1;
        }
    }

    std::uint8_t get_metaprogrammed() const { return metaprogrammed_; }

    void metaprogram() {
        used_ = 1;
        if (get_metaprogrammed_and_looked_up()) {
            return;
        } else if (get_looked_up()) {
            metaprogram_and_look_up();
        } else {
            metaprogrammed_ = 1;
        }
    }

    std::uint8_t get_metaprogrammed_and_looked_up() const {
        return metaprogrammed_and_looked_up_;
    }

    void metaprogram_and_look_up() {
        looked_up_ = 0;
        metaprogrammed_ = 0;
        metaprogrammed_and_looked_up_ = 1;
    }

    std::uint8_t get_used() const { return used_; }

    void operator+=(const ParameterUse &rhs) {
        unpromised_ += rhs.get_unpromised();
        forced_ += rhs.get_forced();
        looked_up_ += rhs.get_looked_up();
        metaprogrammed_ += rhs.get_metaprogrammed();
        metaprogrammed_and_looked_up_ += rhs.get_metaprogrammed_and_looked_up();
        used_ += rhs.get_used();
    }

  private:
    std::uint8_t unpromised_;
    std::uint8_t forced_;
    std::uint8_t looked_up_;
    std::uint8_t metaprogrammed_;
    std::uint8_t metaprogrammed_and_looked_up_;
    std::uint8_t used_;
};

#endif /* PROMISE_DYNTRACER_PARAMETER_USE_H */
