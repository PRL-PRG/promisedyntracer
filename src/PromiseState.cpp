#include "PromiseState.h"

PromiseState::PromiseState(prom_id_t id, env_id_t env_id, bool local)
    : local(local), argument(false), id(id), env_id(env_id), fn_id(""),
      call_id(0), formal_parameter_position(-1), default_argument(false),
      evaluated(false),
      mutations(std::vector<int>(to_underlying_type(SlotMutation::COUNT))) {}

void PromiseState::make_function_argument(fn_id_t fn_id, call_id_t call_id,
                                          int formal_parameter_position,
                                          bool default_argument) {
    this->argument = true;
    this->local = true;
    this->fn_id = fn_id;
    this->call_id = call_id;
    this->formal_parameter_position = formal_parameter_position;
    this->default_argument = default_argument;
}

std::string to_string(PromiseState::SlotMutation slot_mutation) {
    switch (slot_mutation) {
        case PromiseState::SlotMutation::ENVIRONMENT_LOOKUP:
            return "ENVIRONMENT_LOOKUP";
        case PromiseState::SlotMutation::ENVIRONMENT_ASSIGN:
            return "ENVIRONMENT_ASSIGN";
        case PromiseState::SlotMutation::EXPRESSION_LOOKUP:
            return "EXPRESSION_LOOKUP";
        case PromiseState::SlotMutation::EXPRESSION_ASSIGN:
            return "EXPRESSION_ASSIGN";
        case PromiseState::SlotMutation::VALUE_LOOKUP:
            return "VALUE_LOOKUP";
        case PromiseState::SlotMutation::VALUE_ASSIGN:
            return "VALUE_ASSIGN";
        case PromiseState::SlotMutation::COUNT:
            return "COUNT";
        default:
            return "UNKNOWN_SLOT_MUTATION";
    }
}
