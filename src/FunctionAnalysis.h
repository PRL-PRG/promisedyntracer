#ifndef PROMISE_DYNTRACER_FUNCTION_ANALYSIS_H
#define PROMISE_DYNTRACER_FUNCTION_ANALYSIS_H

#include "State.h"
#include "Timer.h"
#include "utilities.h"

class FunctionAnalysis {
  public:
    FunctionAnalysis(const tracer_state_t &tracer_state,
                     const std::string &output_dir)
        : tracer_state_{tracer_state}, output_dir_{output_dir},
          closure_type_{sexptype_to_string(CLOSXP)},
          builtin_type_{sexptype_to_string(BUILTINSXP)},
          special_type_{sexptype_to_string(SPECIALSXP)} {}

    void closure_exit(const closure_info_t &closure_info) {
        add_function(closure_info.fn_id, closure_type_,
                     closure_info.formal_parameter_count, closure_info.name,
                     sexptype_to_string(closure_info.return_value_type));
        write_function_body(closure_info.fn_id, closure_info.fn_definition);
    }

    void special_exit(const builtin_info_t &special_info) {
        add_function(special_info.fn_id, special_type_,
                     special_info.formal_parameter_count, special_info.name,
                     sexptype_to_string(special_info.return_value_type));
        write_function_body(special_info.fn_id, special_info.fn_definition);
    }

    void builtin_exit(const builtin_info_t &builtin_info) {
        add_function(builtin_info.fn_id, builtin_type_,
                     builtin_info.formal_parameter_count, builtin_info.name,
                     sexptype_to_string(builtin_info.return_value_type));
        write_function_body(builtin_info.fn_id, builtin_info.fn_definition);
    }

    void end(dyntracer_t *dyntracer) { serialize(); }

  private:
    void serialize() {
        std::ofstream function_writer_{output_dir_ + "/functions.csv"};

        function_writer_ << "id , type , arguments , name , return_type , calls"
                         << std::endl;

        for (const auto &key_value : functions_) {
            function_writer_ << key_value.first << " , " << key_value.second
                             << std::endl;
        }
        function_writer_.close();
    }

    void write_function_body(const fn_id_t &fn_id,
                             const std::string &definition) {
        auto result = handled_functions_.insert(fn_id);
        if (!result.second)
            return;
        std::ofstream fout(output_dir_ + "/functions/" + fn_id,
                           std::ios::trunc);
        fout << definition;
        fout.close();
    }

    void add_function(fn_id_t fn_id, const std::string &type,
                      int formal_parameter_count, const std::string &name,
                      const std::string &return_value_type) {
        std::string key(fn_id + " , " + type + " , " +
                        std::to_string(formal_parameter_count) + " , " + name +
                        " , " + return_value_type);
        auto result = functions_.insert({key, 1});
        if (!result.second) {
            ++result.first->second;
        }
    }

    const tracer_state_t &tracer_state_;
    std::string output_dir_;
    const std::string closure_type_;
    const std::string special_type_;
    const std::string builtin_type_;
    std::unordered_map<std::string, int> functions_;
    std::unordered_set<fn_id_t> handled_functions_;
};

#endif /* PROMISE_DYNTRACER_FUNCTION_ANALYSIS_H */
