create_dyntracer <- function(output_dir, database, truncate=FALSE, verbose=FALSE) {
  schema <- file.path(path.package("promisedyntracer"), "exec/schema.sql")
  .Call(C_create_dyntracer, output_dir, database, schema, truncate, verbose)
}

destroy_dyntracer <- function(dyntracer)
  invisible(.Call(C_destroy_dyntracer, dyntracer))

dyntrace_promises <- function(expr, output_dir, database, truncate=FALSE, verbose=FALSE) {
  dyntracer <- create_dyntracer(output_dir, database, truncate, verbose)
  result <- dyntrace(dyntracer, expr)
  destroy_dyntracer(dyntracer)
  result
}
