#ifndef __FUNCTION_RETURN_TYPE_ANALYSIS_H__
#define __FUNCTION_RETURN_TYPE_ANALYSIS_H__

#include "State.h"
#include <algorithm>
#include <tuple>
#include <unordered_map>
#include <vector>

class FunctionReturnTypeAnalysis {
  public:
    FunctionReturnTypeAnalysis(const tracer_state_t &tracer_state,
                               const std::string &output_dir);
    void closure_exit(const closure_info_t &closure_info);
    void special_exit(const builtin_info_t &special_info);
    void builtin_exit(const builtin_info_t &builtin_info);
    void end(dyntrace_context_t *context);

  private:
    void serialize();
    void add_return_type(fn_id_t fn_id, sexp_type type);
    std::unordered_map<fn_id_t, std::vector<int>> function_return_types_;
    std::string output_dir_;
    const tracer_state_t &tracer_state_;
};

#endif /* __FUNCTION_RETURN_TYPE_ANALYSIS_H__ */
