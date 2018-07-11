#include "FunctionAnalysis.h"

FunctionAnalysis::FunctionAnalysis(const tracer_state_t &tracer_state,
                                   const std::string &output_dir)
    : tracer_state_(tracer_state), output_dir_(output_dir) {}

void FunctionAnalysis::closure_entry(const closure_info_t &closure_info) {
    add_function_name(closure_info.fn_id, closure_info.name);
    add_function_type_and_body(closure_info.fn_id, "cl",
                               closure_info.fn_definition);
}

void FunctionAnalysis::closure_exit(const closure_info_t &closure_info) {
    add_return_type(closure_info.fn_id, closure_info.return_value_type);
}

void FunctionAnalysis::special_entry(const builtin_info_t &special_info) {
    add_function_name(special_info.fn_id, special_info.name);
    add_function_type_and_body(special_info.fn_id, "sp",
                               special_info.fn_definition);
}

void FunctionAnalysis::special_exit(const builtin_info_t &special_info) {
    add_return_type(special_info.fn_id, special_info.return_value_type);
}

void FunctionAnalysis::builtin_entry(const builtin_info_t &builtin_info) {
    add_function_name(builtin_info.fn_id, builtin_info.name);
    add_function_type_and_body(builtin_info.fn_id, "bu",
                               builtin_info.fn_definition);
}

void FunctionAnalysis::builtin_exit(const builtin_info_t &builtin_info) {
    add_return_type(builtin_info.fn_id, builtin_info.return_value_type);
}

void FunctionAnalysis::end(dyntracer_t *dyntracer) { serialize(); }

void FunctionAnalysis::serialize() {
    serialize_function_name();
    serialize_function_type();
    serialize_function_return_type();
}

void FunctionAnalysis::add_function_name(fn_id_t fn_id,
                                         const std::string name) {
    std::string key(fn_id + " , " + name);
    auto result = function_names_.insert(std::make_pair(key, 1));
    if (!result.second) {
        ++result.first->second;
    }
}

void FunctionAnalysis::add_return_type(fn_id_t fn_id, sexptype_t type) {
    auto iter = function_return_types_.find(fn_id);
    if (iter == function_return_types_.end()) {
        std::vector<int> return_type(MAX_NUM_SEXPTYPE);
        ++return_type[(int)type];
        function_return_types_.insert(make_pair(fn_id, return_type));
    } else {
        ++iter->second[(int)type];
    }
}

void FunctionAnalysis::add_function_type_and_body(
    fn_id_t fn_id, const std::string fn_type, const std::string definition) {
    auto result = function_types_.insert(std::make_pair(fn_id, fn_type));
    if (!result.second)
        return;
    std::ofstream fout(output_dir_ + "/functions/" + fn_id, std::ios::trunc);
    fout << definition;
    fout.close();
}

void FunctionAnalysis::serialize_function_name() {
    std::ofstream fout(output_dir_ + "/function-name.csv", std::ios::trunc);

    fout << "function_id"
         << " , "
         << "function_name"
         << " , "
         << "count" << std::endl;

    for (const auto &key_value : function_names_) {
        fout << key_value.first << " , " << key_value.second << std::endl;
    }

    fout.close();
}

void FunctionAnalysis::serialize_function_type() {
    std::ofstream fout(output_dir_ + "/function-type.csv", std::ios::trunc);

    fout << "function_id"
         << " , "
         << "function_type" << std::endl;

    for (const auto &key_value : function_types_) {
        fout << key_value.first << " , " << key_value.second << std::endl;
    }

    fout.close();
}

void FunctionAnalysis::serialize_function_return_type() {
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
            fout << key_value.first << " , " << sexptype_to_string(i) << " , "
                 << key_value.second[i] << std::endl;
        }
    }

    fout.close();
}
