#include "SideEffectAnalysis.h"

const int SideEffectAnalysis::PROMISE = 0;
const int SideEffectAnalysis::FUNCTION = 0;
const int SideEffectAnalysis::GLOBAL = 0;

const std::vector<std::string> SideEffectAnalysis::scopes{"promise", "function",
                                                          "global"};

SideEffectAnalysis::SideEffectAnalysis(const tracer_state_t &tracer_state,
                                       const std::string &output_dir)
    : tracer_state_(tracer_state), output_dir_(output_dir),
      defines_(std::vector<long long int>(3)),
      assigns_(std::vector<long long int>(3)),
      removals_(std::vector<long long int>(3)),
      lookups_(std::vector<long long int>(3)) {}

void SideEffectAnalysis::environment_define_var(const SEXP symbol,
                                                const SEXP value,
                                                const SEXP rho) {
    environment_action(rho, defines_);
}

void SideEffectAnalysis::environment_assign_var(const SEXP symbol,
                                                const SEXP value,
                                                const SEXP rho) {
    environment_action(rho, assigns_);
}

void SideEffectAnalysis::environment_remove_var(const SEXP symbol,
                                                const SEXP rho) {
    environment_action(rho, removals_);
}

void SideEffectAnalysis::environment_lookup_var(const SEXP symbol,
                                                const SEXP value,
                                                const SEXP rho) {
    environment_action(rho, lookups_);
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

    std::ofstream fout(output_dir_ + "/scope-side-effect.csv", std::ios::trunc);

    fout << "scope"
         << " , "
         << "action"
         << " , "
         << "count" << std::endl;
    for (int i = 0; i < scopes.size(); ++i) {
        fout << scopes[i] << " , "
             << "defines"
             << " , " << defines_[i] << std::endl;
        fout << scopes[i] << " , "
             << "assigns"
             << " , " << assigns_[i] << std::endl;
        fout << scopes[i] << " , "
             << "removals"
             << " , " << removals_[i] << std::endl;
        fout << scopes[i] << " , "
             << "lookups"
             << " , " << lookups_[i] << std::endl;
    }
    fout.close();
}
