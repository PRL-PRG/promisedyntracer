create_dyntracer <- function(trace_filepath, output_dir,
                             truncate=FALSE, enable_trace = TRUE,
                             verbose=FALSE, analysis_switch = emptyenv()) {
  .Call(C_create_dyntracer, trace_filepath, truncate,
        enable_trace, verbose, output_dir, analysis_switch)
}

destroy_dyntracer <- function(dyntracer)
   invisible(.Call(C_destroy_dyntracer, dyntracer))

dyntrace_promises <- function(expr, trace_filepath, output_dir,
                              truncate=FALSE, enable_trace = TRUE,
                              verbose=FALSE, analysis_switch = emptyenv()) {
  dyntracer <- create_dyntracer(trace_filepath, output_dir, truncate,
                                enable_trace, verbose, analysis_switch)
  result <- dyntrace(dyntracer, expr)
  destroy_dyntracer(dyntracer)
  result
}
