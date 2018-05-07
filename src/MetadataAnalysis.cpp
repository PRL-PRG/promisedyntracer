#include "MetadataAnalysis.h"

MetadataAnalysis::MetadataAnalysis(const tracer_state_t &tracer_state,
                                   const std::string &output_dir)
    : tracer_state_(tracer_state), output_dir_(output_dir) {}

void MetadataAnalysis::begin(dyntrace_context_t *context) {
    std::ofstream fout(output_dir_ + "/metadata.csv", std::ios::trunc);
    environment_variables_t environment_variables =
        context->dyntracing_context->environment_variables;
    serialize_row(fout, "GIT_COMMIT_INFO", GIT_COMMIT_INFO);
    serialize_row(fout, "DYNTRACE_BEGIN_DATETIME",
                  context->dyntracing_context->begin_datetime);
    serialize_row(fout, "R_COMPILE_PKGS", environment_variables.r_compile_pkgs);
    serialize_row(fout, "R_DISABLE_BYTECODE",
                  environment_variables.r_disable_bytecode);
    serialize_row(fout, "R_ENABLE_JIT", environment_variables.r_enable_jit);
    serialize_row(fout, "R_KEEP_PKG_SOURCE",
                  environment_variables.r_keep_pkg_source);
    serialize_row(fout, "RDT_COMPILE_VIGNETTE", getenv("RDT_COMPILE_VIGNETTE"));
    fout.close();
}

void MetadataAnalysis::end(dyntrace_context_t *context) {
    std::ofstream fout(output_dir_ + "/metadata.csv", std::ios::app);
    serialize_row(fout, "DYNTRACE_END_DATETIME",
                  context->dyntracing_context->end_datetime);
    execution_time_t execution_time =
        context->dyntracing_context->execution_time;
    serialize_row(fout, "PROBE_FUNCTION_ENTRY",
                  clock_ticks_to_string(execution_time.probe_function_entry));
    serialize_row(fout, "PROBE_FUNCTION_EXIT",
                  clock_ticks_to_string(execution_time.probe_function_exit));
    serialize_row(fout, "PROBE_BUILTIN_ENTRY",
                  clock_ticks_to_string(execution_time.probe_builtin_entry));
    serialize_row(fout, "PROBE_BUILTIN_EXIT",
                  clock_ticks_to_string(execution_time.probe_builtin_exit));
    serialize_row(fout, "PROBE_SPECIALSXP_ENTRY",
                  clock_ticks_to_string(execution_time.probe_specialsxp_entry));
    serialize_row(fout, "PROBE_SPECIALSXP_EXIT",
                  clock_ticks_to_string(execution_time.probe_specialsxp_exit));
    serialize_row(fout, "PROBE_PROMISE_CREATED",
                  clock_ticks_to_string(execution_time.probe_promise_created));
    serialize_row(
        fout, "PROBE_PROMISE_FORCE_ENTRY",
        clock_ticks_to_string(execution_time.probe_promise_force_entry));
    serialize_row(
        fout, "PROBE_PROMISE_FORCE_EXIT",
        clock_ticks_to_string(execution_time.probe_promise_force_exit));
    serialize_row(
        fout, "PROBE_PROMISE_VALUE_LOOKUP",
        clock_ticks_to_string(execution_time.probe_promise_value_lookup));
    serialize_row(
        fout, "PROBE_PROMISE_EXPRESSION_LOOKUP",
        clock_ticks_to_string(execution_time.probe_promise_expression_lookup));
    serialize_row(fout, "PROBE_ERROR",
                  clock_ticks_to_string(execution_time.probe_error));
    serialize_row(fout, "PROBE_VECTOR_ALLOC",
                  clock_ticks_to_string(execution_time.probe_vector_alloc));
    serialize_row(fout, "PROBE_EVAL_ENTRY",
                  clock_ticks_to_string(execution_time.probe_eval_entry));
    serialize_row(fout, "PROBE_EVAL_EXIT",
                  clock_ticks_to_string(execution_time.probe_eval_exit));
    serialize_row(fout, "PROBE_GC_ENTRY",
                  clock_ticks_to_string(execution_time.probe_gc_entry));
    serialize_row(fout, "PROBE_GC_EXIT",
                  clock_ticks_to_string(execution_time.probe_gc_exit));
    serialize_row(
        fout, "PROBE_GC_PROMISE_UNMARKED",
        clock_ticks_to_string(execution_time.probe_gc_promise_unmarked));
    serialize_row(fout, "PROBE_JUMP_CTXT",
                  clock_ticks_to_string(execution_time.probe_jump_ctxt));
    serialize_row(fout, "PROBE_NEW_ENVIRONMENT",
                  clock_ticks_to_string(execution_time.probe_new_environment));
    serialize_row(fout, "PROBE_S3_GENERIC_ENTRY",
                  clock_ticks_to_string(execution_time.probe_S3_generic_entry));
    serialize_row(fout, "PROBE_S3_GENERIC_EXIT",
                  clock_ticks_to_string(execution_time.probe_S3_generic_exit));
    serialize_row(
        fout, "PROBE_S3_DISPATCH_ENTRY",
        clock_ticks_to_string(execution_time.probe_S3_dispatch_entry));
    serialize_row(fout, "PROBE_S3_DISPATCH_EXIT",
                  clock_ticks_to_string(execution_time.probe_S3_dispatch_exit));
    serialize_row(fout, "EXPRESSION",
                  clock_ticks_to_string(execution_time.expression));

#ifdef RDT_TIMER
    for (int i = 0; i < timer::number_of_timers; i++) {
        for (const auto &time :
             Timer::getInstance(static_cast<timer>(i)).stats()) {
            serialize_row(fout, time.first, time.second);
        }
    }
#endif
}

void MetadataAnalysis::serialize_row(std::ofstream &fout,
                                     const std::string &key,
                                     const std::string &value) {
    fout << key << " , " << value << std::endl;
}
