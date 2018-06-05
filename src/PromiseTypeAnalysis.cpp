#include "PromiseTypeAnalysis.h"

PromiseTypeAnalysis::PromiseTypeAnalysis(const tracer_state_t &tracer_state,
                                         const std::string &output_dir)
    : tracer_state_(tracer_state), output_dir_(output_dir) {

    for (int i = 0; i < MAX_NUM_SEXPTYPE; ++i) {
        for (int j = 0; j < MAX_NUM_SEXPTYPE; ++j) {
            default_argument_promise_types_[i][j] = 0;
            custom_argument_promise_types_[i][j] = 0;
            non_argument_promise_types_[i][j] = 0;
        }
    }
}

inline bool
PromiseTypeAnalysis::add_default_argument_promise(prom_id_t prom_id) {
    return default_argument_promises_.insert(prom_id).second;
}

inline bool
PromiseTypeAnalysis::add_custom_argument_promise(prom_id_t prom_id) {
    return custom_argument_promises_.insert(prom_id).second;
}

inline bool PromiseTypeAnalysis::add_non_argument_promise(prom_id_t prom_id) {
    return non_argument_promises_.insert(prom_id).second;
}

inline size_t
PromiseTypeAnalysis::remove_default_argument_promise(prom_id_t prom_id) {
    return default_argument_promises_.erase(prom_id);
}

inline size_t
PromiseTypeAnalysis::remove_custom_argument_promise(prom_id_t prom_id) {
    return custom_argument_promises_.erase(prom_id);
}

inline size_t
PromiseTypeAnalysis::remove_non_argument_promise(prom_id_t prom_id) {
    return non_argument_promises_.erase(prom_id);
}

inline bool
PromiseTypeAnalysis::is_default_argument_promise(prom_id_t prom_id) {
    return (default_argument_promises_.find(prom_id) !=
            default_argument_promises_.end());
}

inline bool PromiseTypeAnalysis::is_custom_argument_promise(prom_id_t prom_id) {
    return (custom_argument_promises_.find(prom_id) !=
            custom_argument_promises_.end());
}

inline bool PromiseTypeAnalysis::is_non_argument_promise(prom_id_t prom_id) {
    return (non_argument_promises_.find(prom_id) !=
            non_argument_promises_.end());
}

void PromiseTypeAnalysis::promise_created(
    const prom_basic_info_t &prom_basic_info, const SEXP promise) {
    add_non_argument_promise(prom_basic_info.prom_id);
}

void PromiseTypeAnalysis::closure_entry(const closure_info_t &closure_info) {
    for (const auto &argument : closure_info.arguments) {
        prom_id_t promise_id = argument.promise_id;
        int formal_parameter_position = argument.formal_parameter_position;
        bool is_default_argument = argument.default_argument;
        if (is_default_argument)
            add_default_argument_promise(promise_id);
        else
            add_custom_argument_promise(promise_id);
        remove_non_argument_promise(promise_id);
    }
}

void PromiseTypeAnalysis::promise_force_exit(const prom_info_t &prom_info,
                                             const SEXP promise) {

    if (is_default_argument_promise(prom_info.prom_id)) {
        ++default_argument_promise_types_[TYPEOF(PRCODE(promise))]
                                         [TYPEOF(PRVALUE(promise))];
        remove_default_argument_promise(prom_info.prom_id);
    } else if (is_custom_argument_promise(prom_info.prom_id)) {
        ++custom_argument_promise_types_[TYPEOF(PRCODE(promise))]
                                        [TYPEOF(PRVALUE(promise))];
        remove_custom_argument_promise(prom_info.prom_id);
    } else if (is_non_argument_promise(prom_info.prom_id)) {
        ++non_argument_promise_types_[TYPEOF(PRCODE(promise))]
                                     [TYPEOF(PRVALUE(promise))];
        remove_non_argument_promise(prom_info.prom_id);
    }
}

void PromiseTypeAnalysis::gc_promise_unmarked(prom_id_t promise_id,
                                              const SEXP promise) {
    if (is_default_argument_promise(promise_id)) {
        add_unevaluated_promise("da", promise);
        remove_default_argument_promise(promise_id);
    } else if (is_custom_argument_promise(promise_id)) {
        add_unevaluated_promise("ca", promise);
        remove_custom_argument_promise(promise_id);
    } else if (is_non_argument_promise(promise_id)) {
        add_unevaluated_promise("na", promise);
        remove_non_argument_promise(promise_id);
    }
}

void PromiseTypeAnalysis::end(dyntrace_context_t *context) { serialize(); }

void PromiseTypeAnalysis::add_unevaluated_promise(
    const std::string promise_type, SEXP promise) {
    std::string key = promise_type + " , " +
                      sexptype_to_string(TYPEOF(PRCODE(promise))) + " , " +
                      infer_sexptype(promise);
    auto result = unevaluated_promises_.insert(std::make_pair(key, 1));
    if (!result.second)
        ++result.first->second;
}

void PromiseTypeAnalysis::serialize() {
    serialize_unevaluated_promises();
    serialize_evaluated_promises();
}

void PromiseTypeAnalysis::serialize_evaluated_promises() {
    std::ofstream fout(output_dir_ + "/evaluated-promise-type.csv",
                       std::ios::trunc);

    fout << "promise_type"
         << " , "
         << "promise_expression_type"
         << " , "
         << "promise_value_type"
         << " , "
         << "count" << std::endl;

    auto serializer = [&](const auto &matrix, std::string type) {
        for (int i = 0; i < MAX_NUM_SEXPTYPE; ++i) {
            for (int j = 0; j < MAX_NUM_SEXPTYPE; ++j) {
                if (matrix[i][j] != 0) {
                    fout << type << " , " << sexptype_to_string(i) << " , "
                         << sexptype_to_string(j) << " , " << matrix[i][j]
                         << std::endl;
                }
            }
        }
    };

    serializer(default_argument_promise_types_, "da");
    serializer(custom_argument_promise_types_, "ca");
    serializer(non_argument_promise_types_, "na");

    fout.close();
}

void PromiseTypeAnalysis::serialize_unevaluated_promises() {
    std::ofstream fout(output_dir_ + "/unevaluated-promise-type.csv",
                       std::ios::trunc);

    fout << "promise_type"
         << " , "
         << "promise_expression_type"
         << " , "
         << "inferred_promise_value_type"
         << " , "
         << "count" << std::endl;

    for (auto &key_value : unevaluated_promises_) {
        fout << key_value.first << " , " << key_value.second << std::endl;
    }

    fout.close();
}
