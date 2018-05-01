#ifndef __PROMISE_EVALUATION_DISTANCE_ANALYSIS_H__
#define __PROMISE_EVALUATION_DISTANCE_ANALYSIS_H__

#include "State.h"
#include <algorithm>
#include <tuple>
#include <unordered_map>
#include <vector>

class PromiseEvaluationDistanceAnalysis {
  public:
    PromiseEvaluationDistanceAnalysis(const tracer_state_t &tracer_state,
                                      const std::string &output_dir);
    void promise_created(const prom_basic_info_t &prom_basic_info,
                         const SEXP promise);
    void closure_entry(const closure_info_t &closure_info);
    void special_entry(const builtin_info_t &special_info);
    void builtin_entry(const builtin_info_t &builtin_info);
    void promise_force_entry(const prom_info_t &prom_info);
    void gc_promise_unmarked(const prom_id_t &prom_id, const SEXP promise);
    void serialize();
    inline int get_closure_counter();
    inline int get_special_counter();
    inline int get_builtin_counter();
    inline void increment_closure_counter();
    inline void increment_special_counter();
    inline void increment_builtin_counter();

  private:
    int closure_counter_;
    int special_counter_;
    int builtin_counter_;
    std::unordered_map<prom_id_t, std::tuple<SEXP, int, int, int>> promises_;
    std::unordered_map<std::string, int> evaluation_distances_;
    std::string output_dir_;
    const tracer_state_t &tracer_state_;
};

#endif /* __PROMISE_EVALUATION_DISTANCE_ANALYSIS_H__ */
