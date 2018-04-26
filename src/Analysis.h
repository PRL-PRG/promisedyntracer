#include "State.h"

class Analysis {
  public:
    Analysis(const tracer_state_t &tracer_state);

  private:
    const tracer_state_t &tracer_state_;
};
