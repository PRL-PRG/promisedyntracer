#ifndef __SIDE_EFFECT_ANALYSIS_H__
#define __SIDE_EFFECT_ANALYSIS_H__

#include "CallState.h"
#include "FunctionState.h"
#include "PromiseState.h"
#include "State.h"
#include "TextTableWriter.h"
#include <algorithm>
#include <tuple>
#include <unordered_map>
#include <vector>

typedef std::size_t timestamp_t;

class SideEffectAnalysis {
  public:
    const static int PROMISE;
    const static int FUNCTION;
    const static int GLOBAL;
    const static std::vector<std::string> scopes;

    SideEffectAnalysis(tracer_state_t &tracer_state,
                       const std::string &output_dir);
    void promise_created(const prom_basic_info_t &prom_basic_info,
                         const SEXP promise);
    void environment_define_var(const SEXP symbol, const SEXP value,
                                const SEXP rho);
    void environment_assign_var(const SEXP symbol, const SEXP value,
                                const SEXP rho);
    void environment_remove_var(const SEXP symbol, const SEXP rho);
    void environment_lookup_var(const SEXP symbol, const SEXP value,
                                const SEXP rho);
    void environment_action(const SEXP rho,
                            std::vector<long long int> &counter);
    void end(dyntracer_t *dyntracer);

  private:
    void serialize();
    timestamp_t get_timestamp_() const;
    timestamp_t update_timestamp_();
    void update_promise_timestamp_(prom_id_t promise_id);
    void update_variable_timestamp_(var_id_t variable_id);
    timestamp_t get_promise_timestamp_(prom_id_t promise_id);
    timestamp_t get_variable_timestamp_(var_id_t variable_id);
    std::unordered_map<prom_id_t, timestamp_t> promise_timestamps_;
    std::unordered_map<var_id_t, timestamp_t> variable_timestamps_;
    std::vector<long long int> defines_;
    std::vector<long long int> assigns_;
    std::vector<long long int> removals_;
    std::vector<long long int> lookups_;
    std::string output_dir_;
    timestamp_t timestamp_;
    tracer_state_t &tracer_state_;
    const timestamp_t undefined_timestamp;
    std::unordered_set<prom_id_t> side_effect_observers_;
    TextTableWriter caused_side_effects_table_writer_;
    TextTableWriter observed_side_effects_table_writer_;
};

#endif /* __SIDE_EFFECT_ANALYSIS_H__ */
