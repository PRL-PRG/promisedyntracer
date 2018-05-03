create_dyntracer <- function(output_dir, database,
                             truncate=FALSE, verbose=FALSE,
                             enable_analysis = TRUE) {
  schema <- file.path(path.package("promisedyntracer"), "exec/schema.sql")
  .Call(C_create_dyntracer, output_dir, database, schema, truncate, verbose, enable_analysis)
}

destroy_dyntracer <- function(dyntracer)
   invisible(.Call(C_destroy_dyntracer, dyntracer))

dyntrace_promises <- function(expr, output_dir, database,
                              truncate=FALSE, verbose=FALSE,
                              enable_analysis = TRUE) {
  dyntracer <- create_dyntracer(output_dir, database, truncate, verbose, enable_analysis)
  result <- dyntrace(dyntracer, expr)
  destroy_dyntracer(dyntracer)
  result
}
