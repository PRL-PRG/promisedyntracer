create_dyntracer <- function(trace_filepath, output_dir,
                             truncate=FALSE, verbose=FALSE,
                             enable_analysis = TRUE) {
  .Call(C_create_dyntracer, trace_filepath, truncate, verbose, output_dir, enable_analysis)
}

destroy_dyntracer <- function(dyntracer)
   invisible(.Call(C_destroy_dyntracer, dyntracer))

dyntrace_promises <- function(expr, trace_filepath, output_dir,
                              truncate=FALSE, verbose=FALSE,
                              enable_analysis = TRUE) {
  dyntracer <- create_dyntracer(trace_filepath, output_dir, truncate,
                                verbose, enable_analysis)
  result <- dyntrace(dyntracer, expr)
  destroy_dyntracer(dyntracer)
  result
}
