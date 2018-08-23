#include "PromiseSlotMutationAnalysis.h"

PromiseSlotMutationAnalysis::PromiseSlotMutationAnalysis(
    const tracer_state_t &tracer_state, const std::string &output_dir,
    PromiseMapper *const promise_mapper)
    : tracer_state_(tracer_state), output_dir_(output_dir),
      promise_mapper_(promise_mapper) {}

void PromiseSlotMutationAnalysis::gc_promise_unmarked(const prom_id_t prom_id,
                                                      const SEXP promise) {
    PromiseState &promise_state = promise_mapper_->find(prom_id);
    update_promise_slot_access_count(promise_state);
}

void PromiseSlotMutationAnalysis::promise_environment_lookup(
    const prom_info_t &info, const SEXP promise) {
    update_promise_argument_slot(
        info.prom_id, PromiseState::SlotMutation::ENVIRONMENT_LOOKUP);
}

void PromiseSlotMutationAnalysis::promise_expression_lookup(
    const prom_info_t &info, const SEXP promise) {
    update_promise_argument_slot(info.prom_id,
                                 PromiseState::SlotMutation::EXPRESSION_LOOKUP);
}

void PromiseSlotMutationAnalysis::promise_value_lookup(const prom_info_t &info,
                                                       const SEXP promise) {

    update_promise_argument_slot(info.prom_id,
                                 PromiseState::SlotMutation::VALUE_LOOKUP);
}

void PromiseSlotMutationAnalysis::promise_environment_set(
    const prom_info_t &info, const SEXP promise) {
    update_promise_argument_slot(
        info.prom_id, PromiseState::SlotMutation::ENVIRONMENT_ASSIGN);
}

void PromiseSlotMutationAnalysis::promise_expression_set(
    const prom_info_t &info, const SEXP promise) {
    update_promise_argument_slot(info.prom_id,
                                 PromiseState::SlotMutation::EXPRESSION_ASSIGN);
}

void PromiseSlotMutationAnalysis::promise_value_set(const prom_info_t &info,
                                                    const SEXP promise) {

    update_promise_argument_slot(info.prom_id,
                                 PromiseState::SlotMutation::VALUE_ASSIGN);
}

void PromiseSlotMutationAnalysis::update_promise_argument_slot(
    const prom_id_t prom_id, PromiseState::SlotMutation slot_mutation) {
    // if (in_force)
    //     return;
    if (promise_is_being_forced_(prom_id))
        return;
    promise_mapper_->find(prom_id).increment_mutation_slot(slot_mutation);
}

bool PromiseSlotMutationAnalysis::promise_is_being_forced_(
    const prom_id_t prom_id) {
    auto stack = tracer_state_.full_stack;
    for (auto iter = stack.crbegin(); iter != stack.crend(); ++iter) {
        auto element = *iter;
        if (element.type == stack_type::PROMISE) {
            if (element.promise_id == prom_id)
                return true;
        }
    }
    return false;
}

void PromiseSlotMutationAnalysis::serialize() {
    for (const auto &key_value : *promise_mapper_) {
        update_promise_slot_access_count(key_value.second);
    }

    serialize_promise_slot_accesses();
}

void PromiseSlotMutationAnalysis::serialize_promise_slot_accesses() {
    std::ofstream fout(output_dir_ + "/promise-slot-accesses.csv",
                       std::ios::trunc);
    for (int i = 0; i < to_underlying_type(PromiseState::SlotMutation::COUNT);
         ++i) {
        fout << to_string(static_cast<PromiseState::SlotMutation>(i)) << " , ";
    }

    fout << "argument_type"
         << " , "
         << "evaluated"
         << " , "
         << "promise_count" << std::endl;

    for (const auto &key_value : promise_slot_accesses_) {
        fout << key_value.first << " , " << key_value.second << std::endl;
    }

    fout.close();
}

void PromiseSlotMutationAnalysis::update_promise_slot_access_count(
    const PromiseState &promise_state) {
    std::string key("");
    for (int i = 0; i < to_underlying_type(PromiseState::SlotMutation::COUNT);
         ++i) {
        key += std::to_string(promise_state.mutations[i]) + " , ";
    }
    key = key + parameter_mode_to_string(promise_state.parameter_mode) + " , ";
    key += promise_state.evaluated ? "Y" : "N";
    auto result = promise_slot_accesses_.insert(make_pair(key, 1));
    if (!result.second)
        ++result.first->second;
}
