#include "SqlSerializer.h"
#include "base64.h"

#ifdef RDT_TIMER
#include "timer.h"
using namespace timing;
#endif

SqlSerializer::SqlSerializer(const std::string &database_filepath,
                             const std::string &schema_filepath, bool truncate,
                             int verbose)
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
    sqlite3_finalize(insert_jump_statement);
    sqlite3_finalize(insert_function_environment_action_statement);
    sqlite3_finalize(insert_promise_environment_action_statement);
    sqlite3_finalize(insert_aggregated_environment_action_statement);
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
        compile("insert into promises values (?,?,?,?,?,?,?,?,?);");

    insert_promise_association_statement =
        compile("insert into promise_associations values (?,?,?);");

    insert_promise_evaluation_statement =
        compile("insert into promise_evaluations values "
                "(?,?,?,?,?,?,?,?,?,?,?,?);");

    insert_promise_return_statement =
        compile("insert into promise_returns values (?,?,?);");

    insert_promise_lifecycle_statement =
        compile("insert into promise_lifecycle values (?,?,?,?,?,?,?);");

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

    insert_jump_statement = compile("insert into jumps values (?,?,?,?);");

    insert_function_environment_action_statement = compile(
        "insert into function_environment_actions values (?,?,?,?,?,?,?,?,?);");

    insert_promise_environment_action_statement = compile(
        "insert into promise_environment_actions values (?,?,?,?,?,?,?,?,?);");

    insert_aggregated_environment_action_statement =
        compile("insert into aggregated_environment_actions values "
                "(?,?,?,?,?);");
}

void SqlSerializer::serialize_start_trace() {
    indent();
    execute(compile("begin transaction;"));
    indent();
}

void SqlSerializer::serialize_metadatum(const std::string &key,
                                        const std::string &value) {
    
    execute(populate_metadata_statement(key, value));
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
    if (verbose < 0) {
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

    if (verbose < 0) {
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
    const prom_lifecycle_info_t &info, int in_force) {
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).reset();
#endif
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
    sqlite3_bind_int(insert_promise_lifecycle_statement, 7, in_force);

#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::PROMISE_LIFECYCLE_WRITE_SQL_BIND);
#endif

    execute(insert_promise_lifecycle_statement);

#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::PROMISE_LIFECYCLE_WRITE_SQL_EXECUTE);
#endif
}

void SqlSerializer::serialize_gc_exit(const gc_info_t &info) {
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).reset();
#endif
    sqlite3_bind_int(insert_gc_trigger_statement, 1, info.counter);
    sqlite3_bind_double(insert_gc_trigger_statement, 2, info.ncells);
    sqlite3_bind_double(insert_gc_trigger_statement, 3, info.vcells);
    sqlite3_bind_int(insert_gc_trigger_statement, 4, info.builtin_calls);
    sqlite3_bind_int(insert_gc_trigger_statement, 5, info.special_calls);
    sqlite3_bind_int(insert_gc_trigger_statement, 6, info.closure_calls);
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::GC_EXIT_WRITE_SQL_BIND);
#endif
    execute(insert_gc_trigger_statement);
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::GC_EXIT_WRITE_SQL_EXECUTE);
#endif
}

void SqlSerializer::serialize_vector_alloc(const type_gc_info_t &info) {
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).reset();
#endif
    sqlite3_bind_int(insert_type_distribution_statement, 1,
                     info.gc_trigger_counter);
    sqlite3_bind_int(insert_type_distribution_statement, 2, info.type);
    sqlite3_bind_int64(insert_type_distribution_statement, 3, info.length);
    sqlite3_bind_int64(insert_type_distribution_statement, 4, info.bytes);
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::VECTOR_ALLOC_WRITE_SQL_BIND);
#endif
    execute(insert_type_distribution_statement);
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::VECTOR_ALLOC_WRITE_SQL_EXECUTE);
#endif
}

void SqlSerializer::serialize_promise_expression_lookup(const prom_info_t &info,
                                                        int clock_id) {
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).reset();
#endif
    auto statement = populate_promise_evaluation_statement(
            info, RDT_SQL_LOOKUP_PROMISE_EXPRESSION, clock_id);
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::LOOKUP_PROMISE_EXPRESSION_WRITE_SQL_BIND);
#endif
    execute(statement);
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::LOOKUP_PROMISE_EXPRESSION_WRITE_SQL_EXECUTE);
#endif
}

void SqlSerializer::serialize_promise_lookup(const prom_info_t &info,
                                             int clock_id) {
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).reset();
#endif
    auto statement = populate_promise_evaluation_statement(info, RDT_SQL_LOOKUP_PROMISE,
                                          clock_id);
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::LOOKUP_PROMISE_VALUE_WRITE_SQL_BIND);
#endif
    execute(statement);
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::LOOKUP_PROMISE_VALUE_WRITE_SQL_EXECUTE);
#endif
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
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).reset();
#endif

    indent();
    bool need_to_insert = register_inserted_function(context, info.fn_id);

#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::FUNCTION_ENTRY_WRITE_SQL_RECORDKEEPING);
#endif

    if (need_to_insert) {
    #ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::FUNCTION_ENTRY_WRITE_SQL_BIND);
#endif
        auto statement = populate_function_statement(info);
    #ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::FUNCTION_ENTRY_WRITE_SQL_EXECUTE);
#endif
        execute(statement);
    }

    for (unsigned int index = 0; index < info.arguments.size(); ++index) {
        auto statement = populate_insert_argument_statement(info, index);
    #ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::FUNCTION_ENTRY_WRITE_SQL_BIND);
#endif
        execute(statement);
    #ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::FUNCTION_ENTRY_WRITE_SQL_EXECUTE);
#endif
    }

    {
        auto statement = populate_call_statement(info);
    #ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::FUNCTION_ENTRY_WRITE_SQL_BIND);
#endif
        execute(statement);
    #ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::FUNCTION_ENTRY_WRITE_SQL_EXECUTE);
#endif
    }

    for (unsigned int index = 0; index < info.arguments.size(); ++index) {
        auto statement = populate_promise_association_statement(info, index);
    #ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::FUNCTION_ENTRY_WRITE_SQL_BIND);
#endif
        execute(statement);
    #ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::FUNCTION_ENTRY_WRITE_SQL_EXECUTE);
#endif
    }
}

void SqlSerializer::serialize_function_exit(const closure_info_t &info) {
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).reset();
#endif
    unindent();
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::FUNCTION_EXIT_WRITE_SQL_RECORDKEEPING);
#endif
    auto statement = populate_call_return_statement(info);
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::FUNCTION_EXIT_WRITE_SQL_BIND);
#endif
    execute(statement);
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::FUNCTION_EXIT_WRITE_SQL_EXECUTE);
#endif
}

void SqlSerializer::serialize_builtin_entry(dyntrace_context_t *context,
                                            const builtin_info_t &info) {
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).reset();
#endif
    indent();
    bool need_to_insert = register_inserted_function(context, info.fn_id);
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::BUILTIN_ENTRY_WRITE_SQL_RECORDKEEPING);
#endif
    if (need_to_insert) {
        auto statement = populate_function_statement(info);
    #ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::BUILTIN_ENTRY_WRITE_SQL_BIND);
#endif
        execute(statement);
    #ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::BUILTIN_ENTRY_WRITE_SQL_EXECUTE);
#endif
    }

    auto statement = populate_call_statement(info);
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::BUILTIN_ENTRY_WRITE_SQL_BIND);
#endif
    execute(statement);
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::BUILTIN_ENTRY_WRITE_SQL_EXECUTE);
#endif
}

void SqlSerializer::serialize_builtin_exit(const builtin_info_t &info) {
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).reset();
#endif
    unindent();
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::BUILTIN_EXIT_WRITE_SQL_RECORDKEEPING);
#endif
    auto statement = populate_call_return_statement(info);
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::BUILTIN_EXIT_WRITE_SQL_BIND);
#endif
    execute(statement);
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::BUILTIN_EXIT_WRITE_SQL_EXECUTE);
#endif
}

void SqlSerializer::serialize_force_promise_entry(dyntrace_context_t *context,
                                                  const prom_info_t &info,
                                                  int clock_id) {
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).reset();
#endif
    indent();
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::FORCE_PROMISE_ENTRY_WRITE_SQL_RECORDKEEPING);
#endif
    if (info.prom_id < 0) // if this is a promise from the outside
        if (!negative_promise_already_inserted(context, info.prom_id)) {
            auto statement = populate_insert_promise_statement(info);
        #ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::FORCE_PROMISE_ENTRY_WRITE_SQL_BIND);
#endif
            execute(statement);
        #ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::FORCE_PROMISE_ENTRY_WRITE_SQL_EXECUTE);
#endif
        }

    execute(populate_promise_evaluation_statement(info, RDT_SQL_FORCE_PROMISE,
                                                  clock_id));
}

void SqlSerializer::serialize_force_promise_exit(const prom_info_t &info,
                                                 int clock_id) {
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).reset();
#endif
    sqlite3_bind_int(insert_promise_return_statement, 1,
                     to_underlying_type(info.return_type));
    sqlite3_bind_int(insert_promise_return_statement, 2, info.prom_id);
    sqlite3_bind_int(insert_promise_return_statement, 3, clock_id);
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::FORCE_PROMISE_EXIT_WRITE_SQL_BIND);
#endif
    execute(insert_promise_return_statement);
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::FORCE_PROMISE_EXIT_WRITE_SQL_EXECUTE);
#endif
    unindent();
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::FORCE_PROMISE_EXIT_WRITE_SQL_RECORDKEEPING);
#endif
}

void SqlSerializer::serialize_promise_created(const prom_basic_info_t &info) {
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).reset();
#endif
    auto statement = populate_insert_promise_statement(info);
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::CREATE_PROMISE_WRITE_SQL_BIND);
#endif
    execute(statement);
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::CREATE_PROMISE_WRITE_SQL_EXECUTE);
#endif
}

void SqlSerializer::serialize_promise_argument_type(const prom_id_t prom_id,
                                                    bool default_argument) {
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).reset();
#endif
    sqlite3_bind_int(insert_promise_argument_type_statement, 1, prom_id);
    sqlite3_bind_int(insert_promise_argument_type_statement, 2,
                     default_argument);
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::FUNCTION_ENTRY_WRITE_SQL_BIND);
#endif
    execute(insert_promise_argument_type_statement);
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::FUNCTION_ENTRY_WRITE_SQL_EXECUTE);
#endif
}

void SqlSerializer::serialize_new_environment(const env_id_t env_id,
                                              const fn_id_t fun_id) {
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).reset();
#endif
    sqlite3_bind_int(insert_environment_statement, 1, env_id);
    sqlite3_bind_text(insert_environment_statement, 2, fun_id.c_str(),
                      fun_id.length(), NULL);
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::NEW_ENVIRONMENT_WRITE_SQL_BIND);
#endif
    execute(insert_environment_statement);
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::NEW_ENVIRONMENT_WRITE_SQL_EXECUTE);
#endif
}

void SqlSerializer::serialize_unwind(const unwind_info_t &info) {
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).reset();
#endif
    sqlite3_bind_int(insert_jump_statement, 1, info.restart);
    sqlite3_bind_int(insert_jump_statement, 2, info.unwound_contexts.size());
    sqlite3_bind_int(insert_jump_statement, 3, info.unwound_calls.size());
    sqlite3_bind_int(insert_jump_statement, 4, info.unwound_promises.size());
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::CONTEXT_JUMP_WRITE_SQL_BIND);
#endif
    execute(insert_jump_statement);

#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::CONTEXT_JUMP_WRITE_SQL_EXECUTE);
#endif

    size_t unwindings =
        info.unwound_calls.size() + info.unwound_promises.size();
    for (size_t i = 1; i <= unwindings; ++i) {
        unindent();
    }
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::CONTEXT_JUMP_WRITE_SQL_RECORDKEEPING);
#endif
}

void SqlSerializer::serialize_variable(var_id_t variable_id,
                                       const std::string &name,
                                       env_id_t environment_id) {
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).reset();
#endif
    sqlite3_bind_int(insert_variable_statement, 1, variable_id);
    sqlite3_bind_text(insert_variable_statement, 2, name.c_str(), name.length(),
                      NULL);
    sqlite3_bind_int(insert_variable_statement, 3, environment_id);
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::ENVIRONMENT_ACTION_WRITE_SQL_BIND);
#endif
    execute(insert_variable_statement);
#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::ENVIRONMENT_ACTION_WRITE_SQL_EXECUTE);
#endif
}

void SqlSerializer::serialize_variable_action(prom_id_t promise_id,
                                              var_id_t variable_id,
                                              const std::string &action) {
    // This is to avoid serializing data to this table.
    // We believe it is not needed anymore.
    return;
    sqlite3_bind_int(insert_variable_action_statement, 1, promise_id);
    sqlite3_bind_int(insert_variable_action_statement, 2, variable_id);
    sqlite3_bind_text(insert_variable_action_statement, 3, action.c_str(),
                      action.length(), NULL);
    execute(insert_variable_action_statement);
}

void SqlSerializer::serialize_function_environment_action(
    fn_id_t function_id, const std::vector<int> &actions) {

#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).reset();
#endif

    sqlite3_bind_text(insert_function_environment_action_statement, 1,
                      function_id.c_str(), -1, SQLITE_STATIC);

    for (int i = 0; i < 8; ++i)
        sqlite3_bind_int(insert_function_environment_action_statement, i + 2,
                         actions[i]);

#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::FUNCTION_EXIT_WRITE_SQL_BIND);
#endif // also builtin, I'll fudge this

    execute(insert_function_environment_action_statement);

#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::FUNCTION_EXIT_WRITE_SQL_EXECUTE);
#endif
}

void SqlSerializer::serialize_promise_environment_action(
    prom_id_t promise_id, const std::vector<int> &actions) {

#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).reset();
#endif

    sqlite3_bind_int(insert_promise_environment_action_statement, 1,
                     promise_id);

    for (int i = 0; i < 8; ++i)
        sqlite3_bind_int(insert_promise_environment_action_statement, i + 2,
                         actions[i]);

#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::FORCE_PROMISE_EXIT_WRITE_SQL_BIND);
#endif

    execute(insert_promise_environment_action_statement);

#ifdef RDT_TIMER
    Timer::getInstance(timer::SQL).endSegment(segment::FORCE_PROMISE_EXIT_WRITE_SQL_EXECUTE);
#endif
}

void SqlSerializer::serialize_aggregated_environment_actions(
    const std::string context, int end, int ena, int enr, int enl) {
    sqlite3_bind_text(insert_aggregated_environment_action_statement, 1,
                      context.c_str(), context.length(), SQLITE_STATIC);
    sqlite3_bind_int(insert_aggregated_environment_action_statement, 2, end);
    sqlite3_bind_int(insert_aggregated_environment_action_statement, 3, ena);
    sqlite3_bind_int(insert_aggregated_environment_action_statement, 4, enr);
    sqlite3_bind_int(insert_aggregated_environment_action_statement, 5, enl);
    execute(insert_aggregated_environment_action_statement);
}

void SqlSerializer::serialize_trace(const std::string &opcode, const int id_1) {

    trace << opcode << " " << id_1 << std::endl;
}

void SqlSerializer::serialize_trace(const std::string &opcode, const int id_1,
                                    const std::string id_2) {
    trace << opcode << " " << id_1 << " " << id_2 << std::endl;
}

void SqlSerializer::serialize_trace(const std::string &opcode, const int id_1,
                                    const int id_2) {

    trace << opcode << " " << id_1 << " " << id_2 << std::endl;
}

void SqlSerializer::serialize_trace(const std::string &opcode, const int id_1,
                                    const int id_2, const std::string &symbol,
                                    const std::string sexptype) {

    trace << opcode << " " << id_1 << " " << id_2 << " " << symbol << " "
          << sexptype << std::endl;
}

void SqlSerializer::serialize_trace(const std::string &opcode,
                                    const fn_id_t &id_1, const int id_2,
                                    const int id_3) {
    trace << opcode << " " << id_1 << " " << id_2 << " " << id_3 << std::endl;
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
                          SQLITE_STATIC);
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
                      SQLITE_STATIC);
    sqlite3_bind_text(insert_promise_statement, 9, info.expression_id.c_str(),
                      -1, SQLITE_STATIC);
    return insert_promise_statement;
}

sqlite3_stmt *SqlSerializer::populate_call_statement(const call_info_t &info) {
    sqlite3_bind_int(insert_call_statement, 1, (int)info.call_id);
    if (info.name.empty())
        sqlite3_bind_null(insert_call_statement, 2);
    else
        sqlite3_bind_text(insert_call_statement, 2, info.name.c_str(), -1,
                          SQLITE_STATIC);

    if (info.callsite_location.empty())
        sqlite3_bind_null(insert_call_statement, 3);
    else
        sqlite3_bind_text(insert_call_statement, 3,
                          info.callsite_location.c_str(), -1, SQLITE_STATIC);

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
                      -1, SQLITE_STATIC);
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

sqlite3_stmt *SqlSerializer::populate_metadata_statement(const std::string &key,
                                                         const std::string &value) {
    if (key.empty())
        sqlite3_bind_null(insert_metadata_statement, 1);
    else
        sqlite3_bind_text(insert_metadata_statement, 1, key.c_str(), -1,
                          SQLITE_STATIC);

    if (value.empty())
        sqlite3_bind_null(insert_metadata_statement, 2);
    else
        sqlite3_bind_text(insert_metadata_statement, 2, value.c_str(), -1,
                          SQLITE_STATIC);

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
                          SQLITE_STATIC);

    if (info.fn_definition.empty())
        sqlite3_bind_null(insert_function_statement, 3);
    else
        sqlite3_bind_text(insert_function_statement, 3,
                          info.fn_definition.c_str(), -1, SQLITE_STATIC);

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
                      -1, SQLITE_STATIC); // XXX ??
    sqlite3_bind_int(insert_argument_statement, 3,
                     actual_parameter_position); // FIXME broken or
                                                 // unnecessary (pick one)
    sqlite3_bind_int(insert_argument_statement, 4, get<4>(argument));
    sqlite3_bind_int(insert_argument_statement, 5, info.call_id);
    return insert_argument_statement;
}
