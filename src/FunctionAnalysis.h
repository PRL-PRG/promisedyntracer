#ifndef __FUNCTION_ANALYSIS_H__
#define __FUNCTION_ANALYSIS_H__

#include "State.h"
#include "Timer.h"
#include "utilities.h"

class FunctionAnalysis {
  public:
    FunctionAnalysis(const tracer_state_t &tracer_state,
                     const std::string &output_dir);

    void closure_entry(const closure_info_t &closure_info);
    void closure_exit(const closure_info_t &closure_info);
    void special_entry(const builtin_info_t &special_info);
    void special_exit(const builtin_info_t &special_info);
    void builtin_entry(const builtin_info_t &builtin_info);
    void builtin_exit(const builtin_info_t &builtin_info);
    void end(dyntracer_t *dyntracer);

  private:
    void add_function_name(fn_id_t fn_id, const std::string name);
    void add_return_type(fn_id_t fn_id, sexptype_t type);
    void add_function_type_and_body(fn_id_t fn_id, const std::string fn_type,
                                    const std::string definition);
    void serialize_function_return_type();
    void serialize_function_name();
    void serialize_function_type();
    void serialize();
    std::unordered_map<fn_id_t, std::vector<int>> function_return_types_;
    std::unordered_map<std::string, int> function_names_;
    std::unordered_map<fn_id_t, std::string> function_types_;
    std::string output_dir_;
    const tracer_state_t &tracer_state_;
};

#endif /* __FUNCTION_ANALYSIS_H__ */
