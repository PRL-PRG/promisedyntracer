#include "FunctionReturnTypeAnalysis.h"

FunctionReturnTypeAnalysis::FunctionReturnTypeAnalysis(
    const tracer_state_t &tracer_state, const std::string &output_dir)
    : tracer_state_(tracer_state), output_dir_(output_dir) {}

void FunctionReturnTypeAnalysis::closure_exit(
    const closure_info_t &closure_info) {
    add_return_type(closure_info.fn_id, closure_info.return_value_type);
}

void FunctionReturnTypeAnalysis::special_exit(
    const builtin_info_t &special_info) {
    add_return_type(special_info.fn_id, special_info.return_value_type);
}

void FunctionReturnTypeAnalysis::builtin_exit(
    const builtin_info_t &builtin_info) {
    add_return_type(builtin_info.fn_id, builtin_info.return_value_type);
}

void FunctionReturnTypeAnalysis::add_return_type(fn_id_t fn_id,
                                                 sexp_type type) {
    auto iter = function_return_types_.find(fn_id);
    if (iter == function_return_types_.end()) {
        std::vector<int> return_type(MAX_NUM_SEXPTYPE);
        ++return_type[(int)type];
        function_return_types_.insert(make_pair(fn_id, return_type));
    } else {
        ++iter->second[(int)type];
    }
}

void FunctionReturnTypeAnalysis::end(dyntrace_context_t *context) {
    serialize();
}

void FunctionReturnTypeAnalysis::serialize() {
    std::ofstream fout(output_dir_ + "/function-return-type.csv",
                       std::ios::trunc);

    fout << "function_id"
         << " , "
         << "return_type"
         << " , "
         << "count" << std::endl;

    for (const auto &key_value : function_return_types_) {
        for (int i = 0; i < MAX_NUM_SEXPTYPE; ++i) {
            if (key_value.second[i] == 0)
                continue;
            fout << key_value.first << " , "
                 << sexp_type_to_string((sexp_type)i) << " , "
                 << key_value.second[i] << std::endl;
        }
    }

    fout.close();
}
