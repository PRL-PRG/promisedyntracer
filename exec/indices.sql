-- Indexes for tracer's SQL schema

-- no indexes for metadata
create index idx_functions_pk on functions(id);
create index idx_arguments_pk on arguments(id);
create index idx_arguments_fk on arguments(call_id);
create index idx_calls_pk on calls(id);
create index idx_calls_fk_function_id on calls(function_id);
create index idx_calls_fk_parent_id on calls(parent_id);
create index idx_call_returns_fk on call_returns(call_id);
create index idx_promises_pk on promises(id);
create index idx_promise_associations_fk_promise_id on promise_associations(promise_id);
create index idx_promise_associations_fk_call_id on promise_associations(call_id);
create index idx_promise_associations_fk_argument_id on promise_associations(argument_id);
create index idx_promise_evaluations_fk_promise_id on promise_evaluations(promise_id);
create index idx_promise_evaluations_fk_from_call_id on promise_evaluations(from_call_id);
create index idx_promise_evaluations_fk_in_call_id on promise_evaluations(in_call_id);
create index idx_promise_returns_fk_promise_id on promise_returns(promise_id);
-- also possible but unnecessary: create index idx_promise_returns_fk_clock on promise_returns(clock);
create index idx_promise_lifecycle_fk_promise_id on promise_lifecycle(promise_id);
create index idx_promise_lifecycle_fk_gc_trigger_counter on promise_lifecycle(gc_trigger_counter);
create index idx_promise_argument_types_fk on promise_argument_types(promise_id);
create index idx_gc_trigger_pk on gc_trigger(counter);
create index idx_type_distribution_fk on type_distribution(gc_trigger_counter);
create index idx_environments_pk on environments(id);
create index idx_environments_fk on environments(function_id);
create index idx_variables_pk on variables(id);
create index idx_variables_fk_environment_id on variables(environment_id);
create index idx_variable_actions_fk_promise_id on variable_actions(promise_id);
create index idx_variable_actions_fk_variable_id on variable_actions(variable_id);
-- no indexes for jumps
create index idx_function_environment_fk_actions on function_environment_actions(function_id);
create index idx_promise_environment_fk_actions on promise_environment_actions(promise_id);
-- no indexes for aggregated environment actions

create index idx_functions_aux_type on functions(type);
create index idx_call_returns_aux_return_value_type on call_returns(return_value_type);
create index idx_promises_aux_type on promises(type);
create index idx_promise_evaluations_aux_event_type on promise_evaluations(event_type);
create index idx_promise_lifecycle_aux_event_type on promise_lifecycle(event_type);
create index idx_type_distribution_aux_type on type_distribution(type);
create index idx_jumps_aux_restart on type_distribution(restart);

create index idx_function_environment_actions_aux_direct_end on function_environment_actions(direct_end);
create index idx_function_environment_actions_aux_direct_ena on function_environment_actions(direct_ena);
create index idx_function_environment_actions_aux_direct_enr on function_environment_actions(direct_enr);
create index idx_function_environment_actions_aux_direct_enl on function_environment_actions(direct_enl);
create index idx_function_environment_actions_aux_transitive_end on function_environment_actions(transitive_end);
create index idx_function_environment_actions_aux_transitive_ena on function_environment_actions(transitive_ena);
create index idx_function_environment_actions_aux_transitive_enr on function_environment_actions(transitive_enr);
create index idx_function_environment_actions_aux_transitive_enl on function_environment_actions(transitive_enl);
create index idx_promise_environment_actions_aux_direct_end on promise_environment_actions(direct_end);
create index idx_promise_environment_actions_aux_direct_ena on promise_environment_actions(direct_ena);
create index idx_promise_environment_actions_aux_direct_enr on promise_environment_actions(direct_enr);
create index idx_promise_environment_actions_aux_direct_enl on promise_environment_actions(direct_enl);
create index idx_promise_environment_actions_aux_transitive_end on promise_environment_actions(transitive_end);
create index idx_promise_environment_actions_aux_transitive_ena on promise_environment_actions(transitive_ena);
create index idx_promise_environment_actions_aux_transitive_enr on promise_environment_actions(transitive_enr);
create index idx_promise_environment_actions_aux_transitive_enl on promise_environment_actions(transitive_enl);
