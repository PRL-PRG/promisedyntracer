#include "AnalysisDriver.h"

AnalysisDriver::AnalysisDriver(const tracer_state_t &tracer_state,
                               const std::string &output_dir)
    : strictness_analysis_(StrictnessAnalysis(tracer_state, output_dir)),
      output_dir_(output_dir) {}

void AnalysisDriver::closure_entry(const closure_info_t &closure_info) {
    strictness_analysis_.closure_entry(closure_info);
}

void AnalysisDriver::closure_exit(const closure_info_t &closure_info) {
    strictness_analysis_.closure_exit(closure_info);
}

void AnalysisDriver::promise_entry(const prom_info_t &prom_info) {
    strictness_analysis_.promise_entry(prom_info);
}

void AnalysisDriver::promise_exit(const prom_info_t &prom_info) {
    strictness_analysis_.promise_exit(prom_info);
}

void AnalysisDriver::gc_promise_unmarked(const prom_id_t prom_id) {
    strictness_analysis_.gc_promise_unmarked(prom_id);
}

void AnalysisDriver::serialize() { strictness_analysis_.serialize(); }
