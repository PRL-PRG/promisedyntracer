#include "SqlSerializer.h"
#include "base64.h"

SqlSerializer::SqlSerializer(const std::string &database_filepath,
                             const std::string &schema_filepath, bool truncate,
                             bool verbose)
    : verbose(verbose), indentation(0), database_filepath(database_filepath) {
    open_database(database_filepath, truncate);
    open_trace(database_filepath, truncate);
    create_tables(schema_filepath);
    prepare_statements();
}

std::string SqlSerializer::get_database_filepath() const {
    return database_filepath;
}

void SqlSerializer::open_database(const std::string database_path,
                                  bool truncate) {
    if (file_exists(database_path)) {
        if (truncate) {
            remove(database_path.c_str());
        } else {
            cleanup();
            dyntrace_log_error(
                "database file '%s' already exists and truncate flag is false",
                database_path.c_str());
        }
    }
    if (sqlite3_open(database_path.c_str(), &database) != SQLITE_OK) {
        char message[SQLITE3_ERROR_MESSAGE_BUFFER_SIZE];
        copy_string(message, sqlite3_errmsg(database),
                    SQLITE3_ERROR_MESSAGE_BUFFER_SIZE);
        cleanup();
        dyntrace_log_error("unable to open database: %s", message);
    }
}

void SqlSerializer::open_trace(const std::string database_path, bool truncate) {
    size_t lastindex = database_path.find_last_of(".");
    std::string trace_path = database_path.substr(0, lastindex) + ".trace";
    if (file_exists(trace_path)) {
        if (truncate)
            remove(trace_path.c_str());
        else {
            cleanup();
            dyntrace_log_error(
                "trace file '%s' already exists and truncate flag is false",
                database_path.c_str());
        }
    }
    trace.open(trace_path);
    if (!trace.good()) {
        cleanup();
        dyntrace_log_error("invalid state of stream object associated with %s",
                           trace_path.c_str());
    }
}

void SqlSerializer::create_tables(const std::string schema_path) {
    // https://sqlite.org/c3ref/exec.HTML
    std::ifstream schema_file(schema_path);
    if (!schema_file.good()) {
        cleanup();
        dyntrace_log_error("unable to open schema file '%s'",
                           schema_path.c_str());
    }
    std::string sql = readfile(schema_file);
    schema_file.close();
    char *errormsg = nullptr;
    sqlite3_exec(database, sql.c_str(), nullptr, nullptr, &errormsg);
    if (errormsg != NULL) {
        char message[SQLITE3_ERROR_MESSAGE_BUFFER_SIZE];
        copy_string(message, errormsg, SQLITE3_ERROR_MESSAGE_BUFFER_SIZE);
        cleanup();
        dyntrace_log_error("unable to create schema from %s, %s",
                           schema_path.c_str(), message);
    }
}

void SqlSerializer::cleanup() {
    close_database();
    close_trace();
    finalize_statements();
}

SqlSerializer::~SqlSerializer() { cleanup(); }

void SqlSerializer::close_database() {
    if (database != NULL)
        sqlite3_close(database);
}
void SqlSerializer::close_trace() {
    if (trace.is_open())
        trace.close();
}

void SqlSerializer::finalize_statements() {
    sqlite3_finalize(insert_metadata_statement);
    sqlite3_finalize(insert_function_statement);
    sqlite3_finalize(insert_argument_statement);
    sqlite3_finalize(insert_call_statement);
    sqlite3_finalize(insert_promise_statement);
    sqlite3_finalize(insert_promise_association_statement);
    sqlite3_finalize(insert_promise_evaluation_statement);
    sqlite3_finalize(insert_promise_return_statement);
    sqlite3_finalize(insert_promise_lifecycle_statement);
    sqlite3_finalize(insert_gc_trigger_statement);
    sqlite3_finalize(insert_type_distribution_statement);
    sqlite3_finalize(insert_environment_statement);
    sqlite3_finalize(insert_variable_statement);
    sqlite3_finalize(insert_variable_action_statement);
}

void SqlSerializer::prepare_statements() {
    insert_metadata_statement = compile("insert into metadata values (?,?);");

    insert_function_statement =
        compile("insert into functions values (?,?,?,?,?);");

    insert_call_statement =
        compile("insert into calls values (?,?,?,?,?,?,?,?,?,?);");

    insert_call_return_statement =
        compile("insert into call_returns values (?,?);");

    insert_argument_statement =
        compile("insert into arguments values (?,?,?,?,?);");

    insert_promise_statement =
        compile("insert into promises values (?,?,?,?,?,?,?,?);");

    insert_promise_association_statement =
        compile("insert into promise_associations values (?,?,?);");

    insert_promise_evaluation_statement =
        compile("insert into promise_evaluations values "
                "(?,?,?,?,?,?,?,?,?,?,?,?);");

    insert_promise_return_statement =
        compile("insert into promise_returns values (?,?,?);");

    insert_promise_lifecycle_statement =
        compile("insert into promise_lifecycle values (?,?,?,?,?,?);");

    insert_promise_argument_type_statement =
        compile("insert into promise_argument_types values (?,?);");

    insert_gc_trigger_statement =
        compile("insert into gc_trigger values (?,?,?,?,?,?);");

    insert_type_distribution_statement =
        compile("insert into type_distribution values (?,?,?,?);");

    insert_environment_statement =
        compile("insert into environments values (?,?);");

    insert_variable_statement =
        compile("insert into variables values (?,?,?);");

    insert_variable_action_statement =
        compile("insert into variable_actions values (?,?,?);");
}

void SqlSerializer::serialize_start_trace() {
    indent();
    execute(compile("begin transaction;"));
    indent();
}

void SqlSerializer::serialize_metadatum(const std::string &key,
                                        const std::string &value) {
    execute(populate_metadata_statement(key.c_str(), value.c_str()));
}

void SqlSerializer::serialize_finish_trace() {
    unindent();
    execute(compile("commit;"));
    unindent();
}

sqlite3_stmt *SqlSerializer::compile(const char *statement) {
    sqlite3_stmt *prepared_statement;
    if (sqlite3_prepare_v2(database, statement, -1, &prepared_statement,
                           NULL) != SQLITE_OK) {
        char message[SQLITE3_ERROR_MESSAGE_BUFFER_SIZE];
        copy_string(message, sqlite3_errmsg(database),
                    SQLITE3_ERROR_MESSAGE_BUFFER_SIZE);
        cleanup();
        dyntrace_log_error("unable to prepare statement '%s', %s", statement,
                           message);
    }
    return prepared_statement;
}

void SqlSerializer::unindent() {
    if (verbose) {
        for (int i = 1; i < indentation; ++i) {
            std::cerr << "│  ";
        }
        std::cerr << "┴" << std::endl;
    }
    indentation--;
}

void SqlSerializer::indent() { indentation++; }

void SqlSerializer::execute(sqlite3_stmt *statement) {
    if (sqlite3_step(statement) != SQLITE_DONE) {
        char message[SQLITE3_ERROR_MESSAGE_BUFFER_SIZE];
        char query[SQLITE3_EXPANDED_SQL_BUFFER_SIZE];
        char *expanded_sql = sqlite3_expanded_sql(statement);
        copy_string(message, sqlite3_errmsg(database),
                    SQLITE3_ERROR_MESSAGE_BUFFER_SIZE);
        copy_string(query, expanded_sql, SQLITE3_EXPANDED_SQL_BUFFER_SIZE);
        sqlite3_free(expanded_sql);
        cleanup();
        dyntrace_log_error("unable to execute statement: %s, %s", query,
                           message);
    }

    if (verbose) {
        for (int i = 1; i < indentation; ++i) {
            std::cerr << "│  ";
        }
        if (indentation > 0) {
            std::cerr << "├─ ";
        }
        std::cerr << sqlite3_expanded_sql(statement) << std::endl;
    }
    sqlite3_reset(statement);
}

void SqlSerializer::serialize_promise_lifecycle(
    const prom_lifecycle_info_t &info) {
    sqlite3_bind_int(insert_promise_lifecycle_statement, 1, info.promise_id);
    sqlite3_bind_int(insert_promise_lifecycle_statement, 2, info.event);
    sqlite3_bind_int(insert_promise_lifecycle_statement, 3,
                     info.gc_trigger_counter);
    sqlite3_bind_int(insert_promise_lifecycle_statement, 4,
                     info.builtin_counter);
    sqlite3_bind_int(insert_promise_lifecycle_statement, 5,
                     info.special_counter);
    sqlite3_bind_int(insert_promise_lifecycle_statement, 6,
                     info.closure_counter);

    execute(insert_promise_lifecycle_statement);
}

void SqlSerializer::serialize_gc_exit(const gc_info_t &info) {
    sqlite3_bind_int(insert_gc_trigger_statement, 1, info.counter);
    sqlite3_bind_double(insert_gc_trigger_statement, 2, info.ncells);
    sqlite3_bind_double(insert_gc_trigger_statement, 3, info.vcells);
    sqlite3_bind_int(insert_gc_trigger_statement, 4, info.builtin_calls);
    sqlite3_bind_int(insert_gc_trigger_statement, 5, info.special_calls);
    sqlite3_bind_int(insert_gc_trigger_statement, 6, info.closure_calls);
    execute(insert_gc_trigger_statement);
}

void SqlSerializer::serialize_vector_alloc(const type_gc_info_t &info) {
    sqlite3_bind_int(insert_type_distribution_statement, 1,
                     info.gc_trigger_counter);
    sqlite3_bind_int(insert_type_distribution_statement, 2, info.type);
    sqlite3_bind_int64(insert_type_distribution_statement, 3, info.length);
    sqlite3_bind_int64(insert_type_distribution_statement, 4, info.bytes);
    execute(insert_type_distribution_statement);
}

void SqlSerializer::serialize_promise_expression_lookup(const prom_info_t &info,
                                                        int clock_id) {
    execute(populate_promise_evaluation_statement(
        info, RDT_SQL_LOOKUP_PROMISE_EXPRESSION, clock_id));
}

void SqlSerializer::serialize_promise_lookup(const prom_info_t &info,
                                             int clock_id) {
    execute(populate_promise_evaluation_statement(info, RDT_SQL_LOOKUP_PROMISE,
                                                  clock_id));
}

// TODO - can type be included in the info struct itself ?
// TODO - better way for state access ?
sqlite3_stmt *SqlSerializer::populate_promise_evaluation_statement(
    const prom_info_t &info, const int type, int clock_id) {

    sqlite3_bind_int(insert_promise_evaluation_statement, 1, clock_id);
    sqlite3_bind_int(insert_promise_evaluation_statement, 2, type);
    sqlite3_bind_int(insert_promise_evaluation_statement, 3, info.prom_id);
    sqlite3_bind_int(insert_promise_evaluation_statement, 4, info.from_call_id);
    sqlite3_bind_int(insert_promise_evaluation_statement, 5, info.in_call_id);
    sqlite3_bind_int(insert_promise_evaluation_statement, 6, info.in_prom_id);
    sqlite3_bind_int(insert_promise_evaluation_statement, 7,
                     to_underlying_type(info.lifestyle));
    sqlite3_bind_int(insert_promise_evaluation_statement, 8,
                     info.effective_distance_from_origin);
    sqlite3_bind_int(insert_promise_evaluation_statement, 9,
                     info.actual_distance_from_origin);
    sqlite3_bind_int(insert_promise_evaluation_statement, 10,
                     to_underlying_type(info.parent_on_stack.type));

    switch (info.parent_on_stack.type) {
        case stack_type::NONE:
            sqlite3_bind_null(insert_promise_evaluation_statement, 11);
            break;
        case stack_type::CALL:
            sqlite3_bind_int(insert_promise_evaluation_statement, 11,
                             (int)info.parent_on_stack.call_id);
            break;
        case stack_type::PROMISE:
            sqlite3_bind_int(insert_promise_evaluation_statement, 11,
                             (int)info.parent_on_stack.promise_id);
            break;
    }

    sqlite3_bind_int(insert_promise_evaluation_statement, 12, info.depth);

    // in_call_id = current call
    // from_call_id = parent call, for which the promise was created
    return insert_promise_evaluation_statement;
}

void SqlSerializer::serialize_function_entry(dyntrace_context_t *context,
                                             const closure_info_t &info) {
    indent();
    bool need_to_insert = register_inserted_function(context, info.fn_id);

    if (need_to_insert) {
        execute(populate_function_statement(info));
    }

    for (unsigned int index = 0; index < info.arguments.size(); ++index) {
        execute(populate_insert_argument_statement(info, index));
    }

    execute(populate_call_statement(info));

    for (unsigned int index = 0; index < info.arguments.size(); ++index) {
        execute(populate_promise_association_statement(info, index));
    }
}

void SqlSerializer::serialize_function_exit(const closure_info_t &info) {
    unindent();
    execute(populate_call_return_statement(info));
}

void SqlSerializer::serialize_builtin_entry(dyntrace_context_t *context,
                                            const builtin_info_t &info) {
    indent();
    bool need_to_insert = register_inserted_function(context, info.fn_id);

    if (need_to_insert) {
        execute(populate_function_statement(info));
    }

    execute(populate_call_statement(info));
}

void SqlSerializer::serialize_builtin_exit(const builtin_info_t &info) {
    unindent();
    execute(populate_call_return_statement(info));
}

void SqlSerializer::serialize_force_promise_entry(dyntrace_context_t *context,
                                                  const prom_info_t &info,
                                                  int clock_id) {
    indent();
    if (info.prom_id < 0) // if this is a promise from the outside
        if (!negative_promise_already_inserted(context, info.prom_id)) {
            execute(populate_insert_promise_statement(info));
        }

    execute(populate_promise_evaluation_statement(info, RDT_SQL_FORCE_PROMISE,
                                                  clock_id));
}

void SqlSerializer::serialize_force_promise_exit(const prom_info_t &info,
                                                 int clock_id) {
    sqlite3_bind_int(insert_promise_return_statement, 1,
                     to_underlying_type(info.return_type));
    sqlite3_bind_int(insert_promise_return_statement, 2, info.prom_id);
    sqlite3_bind_int(insert_promise_return_statement, 3, clock_id);
    execute(insert_promise_return_statement);
    unindent();
}

void SqlSerializer::serialize_promise_created(const prom_basic_info_t &info) {
    execute(populate_insert_promise_statement(info));
}

void SqlSerializer::serialize_promise_argument_type(const prom_id_t prom_id,
                                                    bool default_argument) {
    sqlite3_bind_int(insert_promise_argument_type_statement, 1, prom_id);
    sqlite3_bind_int(insert_promise_argument_type_statement, 2,
                     default_argument);
    execute(insert_promise_argument_type_statement);
}

void SqlSerializer::serialize_new_environment(const env_id_t env_id,
                                              const fn_id_t fun_id) {
    sqlite3_bind_int(insert_environment_statement, 1, env_id);
    sqlite3_bind_text(insert_environment_statement, 2, fun_id.c_str(),
                      fun_id.length(), NULL);
    execute(insert_environment_statement);
}

void SqlSerializer::serialize_unwind(const unwind_info_t &info) {
    size_t unwindings =
        info.unwound_calls.size() + info.unwound_promises.size();
    for (size_t i = 1; i <= unwindings; ++i) {
        unindent();
    }
}

void SqlSerializer::serialize_variable(var_id_t variable_id,
                                       const std::string &name,
                                       env_id_t environment_id) {
    sqlite3_bind_int(insert_variable_statement, 1, variable_id);
    sqlite3_bind_text(insert_variable_statement, 2, name.c_str(), name.length(),
                      NULL);
    sqlite3_bind_int(insert_variable_statement, 3, environment_id);
    execute(insert_variable_statement);
}

void SqlSerializer::serialize_variable_action(prom_id_t promise_id,
                                              var_id_t variable_id,
                                              const std::string &action) {
    sqlite3_bind_int(insert_variable_action_statement, 1, promise_id);
    sqlite3_bind_int(insert_variable_action_statement, 2, variable_id);
    sqlite3_bind_text(insert_variable_action_statement, 3, action.c_str(),
                      action.length(), NULL);
    execute(insert_variable_action_statement);
}

void SqlSerializer::serialize_interference_information(
    const std::string &info) {
    trace << info << std::endl;
}

sqlite3_stmt *SqlSerializer::populate_insert_promise_statement(
    const prom_basic_info_t &info) {
    sqlite3_bind_int(insert_promise_statement, 1, (int)info.prom_id);
    sqlite3_bind_int(insert_promise_statement, 2,
                     to_underlying_type(info.prom_type));

    if (info.full_type.empty()) {
        sqlite3_bind_null(insert_promise_statement, 3);
    } else {
        string full_type = full_sexp_type_to_number_string(info.full_type);
        sqlite3_bind_text(insert_promise_statement, 3, full_type.c_str(), -1,
                          SQLITE_TRANSIENT);
    }

    sqlite3_bind_int(insert_promise_statement, 4, info.in_prom_id);
    sqlite3_bind_int(insert_promise_statement, 5,
                     to_underlying_type(info.parent_on_stack.type));

    switch (info.parent_on_stack.type) {
        case stack_type::NONE:
            sqlite3_bind_null(insert_promise_statement, 6);
            break;
        case stack_type::CALL:
            sqlite3_bind_int(insert_promise_statement, 6,
                             (int)info.parent_on_stack.call_id);
            break;
        case stack_type::PROMISE:
            sqlite3_bind_int(insert_promise_statement, 6,
                             (int)info.parent_on_stack.promise_id);
            break;
    }
    sqlite3_bind_int(insert_promise_statement, 7, info.depth);
    sqlite3_bind_text(insert_promise_statement, 8, info.expression.c_str(), -1,
                      SQLITE_TRANSIENT);
    return insert_promise_statement;
}

sqlite3_stmt *SqlSerializer::populate_call_statement(const call_info_t &info) {
    sqlite3_bind_int(insert_call_statement, 1, (int)info.call_id);
    if (info.name.empty())
        sqlite3_bind_null(insert_call_statement, 2);
    else
        sqlite3_bind_text(insert_call_statement, 2, info.name.c_str(), -1,
                          SQLITE_TRANSIENT);

    if (info.callsite_location.empty())
        sqlite3_bind_null(insert_call_statement, 3);
    else
        sqlite3_bind_text(insert_call_statement, 3,
                          info.callsite_location.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_bind_int(insert_call_statement, 4, info.fn_compiled ? 1 : 0);
    sqlite3_bind_text(insert_call_statement, 5, info.fn_id.c_str(),
                      info.fn_id.length(), NULL);
    sqlite3_bind_int(insert_call_statement, 6, (int)info.parent_call_id);
    sqlite3_bind_int(insert_call_statement, 7, (int)info.in_prom_id);
    sqlite3_bind_int(insert_call_statement, 8,
                     to_underlying_type(info.parent_on_stack.type));

    switch (info.parent_on_stack.type) {
        case stack_type::NONE:
            sqlite3_bind_null(insert_call_statement, 9);
            break;
        case stack_type::CALL:
            sqlite3_bind_int(insert_call_statement, 9,
                             (int)info.parent_on_stack.call_id);
            break;
        case stack_type::PROMISE:
            sqlite3_bind_int(insert_call_statement, 9,
                             (int)info.parent_on_stack.promise_id);
            break;
    }
    sqlite3_bind_text(insert_call_statement, 10, info.call_expression.c_str(),
                      -1, SQLITE_TRANSIENT);
    return insert_call_statement;
}

sqlite3_stmt *
SqlSerializer::populate_call_return_statement(const call_info_t &info) {
    sqlite3_bind_int(insert_call_return_statement, 1, (int)info.call_id);
    sqlite3_bind_int(insert_call_return_statement, 2,
                     (int)info.return_value_type);
    return insert_call_return_statement;
}

sqlite3_stmt *SqlSerializer::populate_promise_association_statement(
    const closure_info_t &info, int index) {

    const arg_t &argument = info.arguments.all()[index].get();
    arg_id_t arg_id = get<1>(argument);
    prom_id_t promise = get<2>(argument);

    if (promise != RID_INVALID)
        sqlite3_bind_int(insert_promise_association_statement, 1, promise);
    else
        sqlite3_bind_null(insert_promise_association_statement, 1);
    sqlite3_bind_int(insert_promise_association_statement, 2, info.call_id);
    sqlite3_bind_int(insert_promise_association_statement, 3, arg_id);

    return insert_promise_association_statement;
}

sqlite3_stmt *SqlSerializer::populate_metadata_statement(const string key,
                                                         const string value) {
    if (key.empty())
        sqlite3_bind_null(insert_metadata_statement, 1);
    else
        sqlite3_bind_text(insert_metadata_statement, 1, key.c_str(), -1,
                          SQLITE_TRANSIENT);

    if (value.empty())
        sqlite3_bind_null(insert_metadata_statement, 2);
    else
        sqlite3_bind_text(insert_metadata_statement, 2, value.c_str(), -1,
                          SQLITE_TRANSIENT);

    return insert_metadata_statement;
}

sqlite3_stmt *
SqlSerializer::populate_function_statement(const call_info_t &info) {
    sqlite3_bind_text(insert_function_statement, 1, info.fn_id.c_str(),
                      info.fn_id.length(), NULL);

    if (info.definition_location.empty())
        sqlite3_bind_null(insert_function_statement, 2);
    else
        sqlite3_bind_text(insert_function_statement, 2,
                          info.definition_location.c_str(), -1,
                          SQLITE_TRANSIENT);

    if (info.fn_definition.empty())
        sqlite3_bind_null(insert_function_statement, 3);
    else
        sqlite3_bind_text(insert_function_statement, 3,
                          info.fn_definition.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_bind_int(insert_function_statement, 4,
                     to_underlying_type(info.fn_type));

    sqlite3_bind_int(insert_function_statement, 5, info.fn_compiled ? 1 : 0);

    return insert_function_statement;
}

sqlite3_stmt *SqlSerializer::populate_insert_argument_statement(
    const closure_info_t &info, int actual_parameter_position) {
    const arg_t &argument =
        info.arguments.all()[actual_parameter_position].get();
    sqlite3_bind_int(insert_argument_statement, 1, get<1>(argument));
    sqlite3_bind_text(insert_argument_statement, 2, get<0>(argument).c_str(),
                      -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(
        insert_argument_statement, 3,
        actual_parameter_position); // FIXME broken or unnecessary (pick one)
    sqlite3_bind_int(insert_argument_statement, 4, get<4>(argument));
    sqlite3_bind_int(insert_argument_statement, 5, info.call_id);
    return insert_argument_statement;
}
