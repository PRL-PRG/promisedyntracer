#ifndef __SIDE_EFFECT_ANALYSIS_H__
#define __SIDE_EFFECT_ANALYSIS_H__

#include "ArgumentPromiseState.h"
#include "CallState.h"
#include "FunctionState.h"
#include "State.h"
#include <algorithm>
#include <tuple>
#include <unordered_map>
#include <vector>

class SideEffectAnalysis {
  public:
    const static int PROMISE;
    const static int FUNCTION;
    const static int GLOBAL;
    const static std::vector<std::string> scopes;

    SideEffectAnalysis(const tracer_state_t &tracer_state,
                       const std::string &output_dir);
    void environment_define_var(const SEXP symbol, const SEXP value,
                                const SEXP rho);
    void environment_assign_var(const SEXP symbol, const SEXP value,
                                const SEXP rho);
    void environment_remove_var(const SEXP symbol, const SEXP rho);
    void environment_lookup_var(const SEXP symbol, const SEXP value,
                                const SEXP rho);
    void environment_action(const SEXP rho,
                            std::vector<long long int> &counter);
    void serialize();

  private:
    std::vector<long long int> defines_;
    std::vector<long long int> assigns_;
    std::vector<long long int> removals_;
    std::vector<long long int> lookups_;
    std::string output_dir_;
    const tracer_state_t &tracer_state_;
};

#endif /* __SIDE_EFFECT_ANALYSIS_H__ */
