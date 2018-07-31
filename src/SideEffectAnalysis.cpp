#include "SideEffectAnalysis.h"

const int SideEffectAnalysis::PROMISE = 0;
const int SideEffectAnalysis::FUNCTION = 0;
const int SideEffectAnalysis::GLOBAL = 0;

const std::vector<std::string> SideEffectAnalysis::scopes{"promise", "function",
                                                          "global"};

SideEffectAnalysis::SideEffectAnalysis(tracer_state_t &tracer_state,
                                       const std::string &output_dir)
    : tracer_state_(tracer_state), output_dir_(output_dir),
      defines_{std::vector<long long int>(3)},
      assigns_{std::vector<long long int>(3)},
      removals_{std::vector<long long int>(3)},
      lookups_{std::vector<long long int>(3)}, timestamp_{0},
      undefined_timestamp{std::numeric_limits<std::size_t>::max()},
      observed_side_effects_table_writer_{
          output_dir + "/" + "observed-side-effects.csv", {"scope", "count"}},
      caused_side_effects_table_writer_{output_dir + "/" +
                                            "caused-side-effects.csv",
                                        {"scope", "action", "count"}} {}

void SideEffectAnalysis::promise_created(
    const prom_basic_info_t &prom_basic_info, const SEXP promise) {
    update_promise_timestamp_(prom_basic_info.prom_id);
}

void SideEffectAnalysis::environment_define_var(const SEXP symbol,
                                                const SEXP value,
                                                const SEXP rho) {
    bool exists = false;
    update_variable_timestamp_(
        tracer_state_.to_variable_id(symbol, rho, exists));
    environment_action(rho, defines_);
}

void SideEffectAnalysis::environment_assign_var(const SEXP symbol,
                                                const SEXP value,
                                                const SEXP rho) {
    bool exists = false;
    update_variable_timestamp_(
        tracer_state_.to_variable_id(symbol, rho, exists));
    environment_action(rho, assigns_);
}

void SideEffectAnalysis::environment_remove_var(const SEXP symbol,
                                                const SEXP rho) {
    environment_action(rho, removals_);
}

void SideEffectAnalysis::environment_lookup_var(const SEXP symbol,
                                                const SEXP value,
                                                const SEXP rho) {
    // we process the action first.
    environment_action(rho, lookups_);

    // if timestamp is absent, then we don't have to go further and
    // compare with promise timestamp.
    bool exists = false;
    timestamp_t variable_timestamp{get_variable_timestamp_(
        tracer_state_.to_variable_id(symbol, rho, exists))};

    if (variable_timestamp == undefined_timestamp)
        return;

    size_t stack_size = tracer_state_.full_stack.size();

    for (int i = stack_size - 1; i >= 0; --i) {
        stack_event_t exec_context = tracer_state_.full_stack[i];

        if (exec_context.type == stack_type::PROMISE) {
            timestamp_t promise_timestamp{
                get_promise_timestamp_(exec_context.promise_id)};

            if (promise_timestamp != undefined_timestamp) {
                if (promise_timestamp < variable_timestamp) {
                    side_effect_observers_.insert(exec_context.promise_id);
                }
            }
        }
    }
}

void SideEffectAnalysis::environment_action(
    const SEXP rho, std::vector<long long int> &counter) {
    size_t stack_size = tracer_state_.full_stack.size();
    for (int i = stack_size - 1; i >= 0; --i) {
        stack_event_t exec_context = tracer_state_.full_stack[i];
        if (exec_context.type == stack_type::CALL) {
            SEXP enclosing_address =
                reinterpret_cast<SEXP>(exec_context.enclosing_environment);
            // function side effects matter iff they are done in external
            // environments.
            if (rho != enclosing_address)
                ++counter[SideEffectAnalysis::FUNCTION];
            return;
        } else if (exec_context.type == stack_type::PROMISE) {

            ++counter[SideEffectAnalysis::PROMISE];
            return;
        }
    }
    /* empty stack implies that the action happens at top level */
    ++counter[SideEffectAnalysis::GLOBAL];
}

void SideEffectAnalysis::end(dyntracer_t *dyntracer) { serialize(); }

void SideEffectAnalysis::serialize() {

    for (int i = 0; i < scopes.size(); ++i) {
        caused_side_effects_table_writer_
            .write_row(scopes[i], "defines", defines_[i])
            .write_row(scopes[i], "assigns", assigns_[i])
            .write_row(scopes[i], "removals", removals_[i])
            .write_row(scopes[i], "lookups", lookups_[i]);
    }

    observed_side_effects_table_writer_.write_row(
        "promise", side_effect_observers_.size());
}

timestamp_t SideEffectAnalysis::get_timestamp_() const { return timestamp_; }

timestamp_t SideEffectAnalysis::update_timestamp_() { return timestamp_++; }

void SideEffectAnalysis::update_promise_timestamp_(prom_id_t promise_id) {
    promise_timestamps_[promise_id] = update_timestamp_();
}

void SideEffectAnalysis::update_variable_timestamp_(var_id_t variable_id) {
    variable_timestamps_[variable_id] = update_timestamp_();
}

timestamp_t SideEffectAnalysis::get_promise_timestamp_(prom_id_t promise_id) {
    auto iter = promise_timestamps_.find(promise_id);
    if (iter == promise_timestamps_.end()) {
        return undefined_timestamp;
    }
    return iter->second;
}

timestamp_t SideEffectAnalysis::get_variable_timestamp_(var_id_t variable_id) {
    auto iter = variable_timestamps_.find(variable_id);
    if (iter == variable_timestamps_.end()) {
        return undefined_timestamp;
    }
    return iter->second;
}
