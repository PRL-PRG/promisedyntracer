#include "PromiseEvaluationDistanceAnalysis.h"

PromiseEvaluationDistanceAnalysis::PromiseEvaluationDistanceAnalysis(
    const tracer_state_t &tracer_state, const std::string &output_dir)
    : tracer_state_(tracer_state), output_dir_(output_dir), closure_counter_(0),
      special_counter_(0), builtin_counter_(0) {}

inline int PromiseEvaluationDistanceAnalysis::get_closure_counter() {
    return closure_counter_;
}

inline int PromiseEvaluationDistanceAnalysis::get_special_counter() {
    return special_counter_;
}

inline int PromiseEvaluationDistanceAnalysis::get_builtin_counter() {
    return builtin_counter_;
}

inline void PromiseEvaluationDistanceAnalysis::increment_closure_counter() {
    ++closure_counter_;
}

inline void PromiseEvaluationDistanceAnalysis::increment_special_counter() {
    ++special_counter_;
}

inline void PromiseEvaluationDistanceAnalysis::increment_builtin_counter() {
    ++builtin_counter_;
}

void PromiseEvaluationDistanceAnalysis::promise_created(
    const prom_basic_info_t &prom_basic_info, const SEXP promise) {
    promises_.emplace(std::make_pair(
        prom_basic_info.prom_id,
        std::make_tuple(promise, get_closure_counter(), get_special_counter(),
                        get_builtin_counter())));
}

void PromiseEvaluationDistanceAnalysis::closure_entry(
    const closure_info_t &closure_info) {
    increment_closure_counter();
}

void PromiseEvaluationDistanceAnalysis::special_entry(
    const builtin_info_t &special_info) {
    increment_special_counter();
}

void PromiseEvaluationDistanceAnalysis::builtin_entry(
    const builtin_info_t &builtin_info) {
    increment_builtin_counter();
}

void PromiseEvaluationDistanceAnalysis::promise_force_entry(
    const prom_info_t &prom_info) {
    auto iter = promises_.find(prom_info.prom_id);
    if (iter == promises_.end())
        return;
    int closure_counter = std::get<1>(iter->second);
    int special_counter = std::get<2>(iter->second);
    int builtin_counter = std::get<3>(iter->second);
    promises_.erase(iter);
    std::string key =
        to_string(get_closure_counter() - closure_counter) + " , " +
        to_string(get_special_counter() - special_counter) + " , " +
        to_string(get_builtin_counter() - builtin_counter);
    auto result = evaluation_distances_.emplace(make_pair(key, 1));
    if (!result.second)
        ++result.first->second;
}

void PromiseEvaluationDistanceAnalysis::gc_promise_unmarked(
    const prom_id_t &prom_id, const SEXP promise) {
    promises_.erase(prom_id);
}

void PromiseEvaluationDistanceAnalysis::serialize() {
    std::ofstream fout(output_dir_ + "/promise-evaluation-distance.csv",
                       std::ios::trunc);

    fout << "closure_count"
         << " , "
         << "special_count"
         << " , "
         << "builtin_count"
         << " , "
         << "promise_count" << std::endl;

    for (const auto &key_value : evaluation_distances_) {
        fout << key_value.first << " , " << key_value.second << std::endl;
    }

    fout.close();
}
