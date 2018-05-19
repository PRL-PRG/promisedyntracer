#ifndef __PROMISE_MAPPER_H__
#define __PROMISE_MAPPER_H__

#include "PromiseState.h"
#include "State.h"
#include <algorithm>
#include <tuple>
#include <unordered_map>
#include <vector>

class PromiseMapper {
    using promises_t = std::unordered_map<prom_id_t, PromiseState>;

  public:
    using iterator = promises_t::iterator;
    using const_iterator = promises_t::const_iterator;

    PromiseMapper(tracer_state_t &tracer_state, const std::string &output_dir);
    void promise_created(const prom_basic_info_t &prom_basic_info,
                         const SEXP promise);
    void closure_entry(const closure_info_t &closure_info);
    void promise_force_entry(const prom_info_t &prom_info, const SEXP promise);
    void promise_environment_lookup(const prom_info_t &info, const SEXP promise,
                                    int in_force);
    void promise_expression_lookup(const prom_info_t &info, const SEXP promise,
                                   int in_force);
    void promise_value_lookup(const prom_info_t &info, const SEXP promise,
                              int in_force);
    void promise_environment_set(const prom_info_t &info, const SEXP promise,
                                 int in_force);
    void promise_expression_set(const prom_info_t &info, const SEXP promise,
                                int in_force);
    void promise_value_set(const prom_info_t &info, const SEXP promise,
                           int in_force);
    void gc_promise_unmarked(const prom_id_t prom_id, const SEXP promise);
    void end(dyntrace_context_t *context);
    PromiseState &find(const prom_id_t prom_id);

    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    const_iterator cbegin() const;
    const_iterator cend() const;

  private:
    void insert_if_non_local(const prom_id_t prom_id, const SEXP promise);
    promises_t promises_;
    std::string output_dir_;
    tracer_state_t &tracer_state_;
    static const size_t PROMISE_MAPPING_BUCKET_COUNT;
};

#endif /* __PROMISE_ACCESS_ANALYSIS_H__ */
