#include "PromiseMapper.h"

const size_t PromiseMapper::PROMISE_MAPPING_BUCKET_COUNT = 1000000;

PromiseMapper::PromiseMapper(tracer_state_t &tracer_state,
                             const std::string &output_dir)
    : tracer_state_(tracer_state), output_dir_(output_dir),
      promises_(std::unordered_map<prom_id_t, PromiseState>(
          PROMISE_MAPPING_BUCKET_COUNT)) {}

void PromiseMapper::promise_created(const prom_basic_info_t &prom_basic_info,
                                    const SEXP promise) {
    auto result = promises_.insert(
        {prom_basic_info.prom_id,
         PromiseState(prom_basic_info.prom_id,
                      tracer_state_.to_environment_id(PRENV(promise)), true)});
    // if result.second is false, this means that a promise with this id
    // already exists in map. This means that the promise with the same id
    // has either not been removed in the gc_promise_unmarked stage or the
    // the promise id assignment scheme assigns same id to two promises.
    assert(result.second == true);
}

void PromiseMapper::closure_entry(const closure_info_t &closure_info) {
    int max_position = 0;
    int actual_argument_position = 0;
    for (const auto &argument : closure_info.arguments) {
        if (argument.value_type != PROMSXP)
            continue;
        prom_id_t promise_id = argument.promise_id;
        int formal_parameter_position = argument.formal_parameter_position;
        bool is_default_argument = argument.default_argument;
        if (argument.promise_id < 0) {
            promises_.insert({argument.promise_id,
                              PromiseState(argument.promise_id,
                                           tracer_state_.to_environment_id(
                                               argument.promise_environment),
                                           false)});
        }
        PromiseState &promise_state = promises_.at(promise_id);
        // if this assert fails, it means that the promise with this id
        // has not been seen in promise_created stage. This implies either
        // incorrect insertion to promises_ mapping or missing probes at
        // promise creation point in the interpreter.
        // assert(it != promises_.end());
        promise_state.make_function_argument(
            closure_info.fn_id, closure_info.call_id, formal_parameter_position,
            actual_argument_position, is_default_argument);
    }
}

void PromiseMapper::promise_force_entry(const prom_info_t &prom_info,
                                        const SEXP promise) {
    insert_if_non_local(prom_info.prom_id, promise);
}

void PromiseMapper::promise_environment_lookup(const prom_info_t &prom_info,
                                               const SEXP promise) {
    insert_if_non_local(prom_info.prom_id, promise);
}

void PromiseMapper::promise_expression_lookup(const prom_info_t &prom_info,
                                              const SEXP promise) {
    insert_if_non_local(prom_info.prom_id, promise);
}

void PromiseMapper::promise_value_lookup(const prom_info_t &prom_info,
                                         const SEXP promise) {
    insert_if_non_local(prom_info.prom_id, promise);
}

void PromiseMapper::promise_environment_set(const prom_info_t &prom_info,
                                            const SEXP promise) {
    insert_if_non_local(prom_info.prom_id, promise);
}

void PromiseMapper::promise_expression_set(const prom_info_t &prom_info,
                                           const SEXP promise) {
    insert_if_non_local(prom_info.prom_id, promise);
}

void PromiseMapper::promise_value_set(const prom_info_t &prom_info,
                                      const SEXP promise) {
    insert_if_non_local(prom_info.prom_id, promise);
}

void PromiseMapper::gc_promise_unmarked(const prom_id_t prom_id,
                                        const SEXP promise) {
    // it is possible that the promise does not exist in mapping.
    // this happens if the promise was created outside of tracing
    // but is being garbage collected in the middle of tracing
    promises_.erase(prom_id);
}

void PromiseMapper::end(dyntracer_t *dyntracer) { promises_.clear(); }

PromiseState &PromiseMapper::find(const prom_id_t prom_id) {
    auto iter = promises_.find(prom_id);
    // all promises encountered are added to the map. Its not possible for
    // a promise id to be encountered which is not already mapped.
    // If this happens, possibly, the mapper methods are not the first to
    // be called in the analysis driver methods. Hence, they are not able
    // to update the mapping.
    assert(iter != promises_.end());
    return iter->second;
}

void PromiseMapper::insert_if_non_local(const prom_id_t prom_id,
                                        const SEXP promise) {

    // the insertion only happens if the promise with this id does not already
    // exist. If the promise does not already exist, it means that we have not
    // seen its creation which means it is non local.
    promises_.insert(
        {prom_id,
         PromiseState(prom_id, tracer_state_.to_environment_id(PRENV(promise)),
                      false)});
}

PromiseMapper::iterator PromiseMapper::begin() { return promises_.begin(); }

PromiseMapper::iterator PromiseMapper::end() { return promises_.end(); }

PromiseMapper::const_iterator PromiseMapper::begin() const {
    return promises_.begin();
}

PromiseMapper::const_iterator PromiseMapper::end() const {
    return promises_.end();
}

PromiseMapper::const_iterator PromiseMapper::cbegin() const {
    return promises_.cbegin();
}

PromiseMapper::const_iterator PromiseMapper::cend() const {
    return promises_.cend();
}
