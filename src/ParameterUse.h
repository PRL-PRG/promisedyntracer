#ifndef PROMISE_DYNTRACER_PARAMETER_USE_H
#define PROMISE_DYNTRACER_PARAMETER_USE_H

#include "State.h"
#include "sexptypes.h"
#include <cstdint>
#include <string>

class ParameterUse {
  public:
    explicit ParameterUse()
        : type_(0), force_(0), lookup_(0), metaprogram_(0),
          mode_(parameter_mode_t::UNKNOWN) {}

    std::uint8_t get_metaprogram() const { return metaprogram_; }

    void metaprogram() { ++metaprogram_; }

    std::uint8_t get_lookup() const { return lookup_; }

    void lookup() { ++lookup_; }

    std::uint8_t get_force() const { return force_; }

    void force() { ++force_; }

    sexptype_t get_type() const { return type_; }

    void set_type(const sexptype_t type) { type_ = type; }

    parameter_mode_t get_parameter_mode() const { return mode_; }

    void set_parameter_mode(parameter_mode_t mode) { mode_ = mode; }

  private:
    sexptype_t type_;
    std::uint8_t force_;
    std::uint8_t lookup_;
    std::uint8_t metaprogram_;
    parameter_mode_t mode_;
};

#endif /* PROMISE_DYNTRACER_PARAMETER_USE_H */
