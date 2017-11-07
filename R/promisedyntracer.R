create_dyntracer <- function(database, verbose=TRUE) {
  schema <- file.path(path.package("promisedyntracer"), "exec/schema.sql")
  .Call(C_create_dyntracer, database, schema, verbose)
}

destroy_dyntracer <- function(dyntracer)
  invisible(.Call(C_destroy_dyntracer, dyntracer))

dyntrace_promises <- function(expr, database, verbose=TRUE) {
  dyntracer <- create_dyntracer(database, verbose)
  result <- dyntrace(dyntracer, expr)
  destroy_dyntracer(dyntracer)
  result
}
