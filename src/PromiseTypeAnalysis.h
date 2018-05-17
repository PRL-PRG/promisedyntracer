#ifndef __PROMISE_TYPE_ANALYSIS_H__
#define __PROMISE_TYPE_ANALYSIS_H__

#include "State.h"
#include "utilities.h"

class PromiseTypeAnalysis {
  public:
    PromiseTypeAnalysis(const tracer_state_t &tracer_state,
                        const std::string &output_dir);
    void promise_created(const prom_basic_info_t &prom_basic_info,
                         const SEXP promise);
    void closure_entry(const closure_info_t &closure_info);
    void promise_force_exit(const prom_info_t &prom_info, const SEXP promise);
    void gc_promise_unmarked(prom_id_t prom_id, const SEXP promise);
    inline bool add_default_argument_promise(prom_id_t prom_id);
    inline bool add_custom_argument_promise(prom_id_t prom_id);
    inline bool add_non_argument_promise(prom_id_t prom_id);
    inline size_t remove_default_argument_promise(prom_id_t prom_id);
    inline size_t remove_custom_argument_promise(prom_id_t prom_id);
    inline size_t remove_non_argument_promise(prom_id_t prom_id);
    inline bool is_default_argument_promise(prom_id_t prom_id);
    inline bool is_custom_argument_promise(prom_id_t prom_id);
    inline bool is_non_argument_promise(prom_id_t prom_id);
    void end(dyntrace_context_t *context);

  private:
    void serialize();
    void serialize_evaluated_promises();
    void serialize_unevaluated_promises();
    void add_unevaluated_promise(const std::string promise_type, SEXP promise);

    std::string output_dir_;
    const tracer_state_t &tracer_state_;
    std::set<prom_id_t> default_argument_promises_;
    std::set<prom_id_t> custom_argument_promises_;
    std::set<prom_id_t> non_argument_promises_;
    int default_argument_promise_types_[MAX_NUM_SEXPTYPE][MAX_NUM_SEXPTYPE];
    int custom_argument_promise_types_[MAX_NUM_SEXPTYPE][MAX_NUM_SEXPTYPE];
    int non_argument_promise_types_[MAX_NUM_SEXPTYPE][MAX_NUM_SEXPTYPE];
    std::unordered_map<std::string, int> unevaluated_promises_;
};

#endif /* __PROMISE_TYPE_ANALYSIS_H__ */
