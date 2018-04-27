#ifndef __SQL_SERIALIZER_H__
#define __SQL_SERIALIZER_H__

#include "State.h"
#include "sqlite3.h"
#include "stdlibs.h"
#include "utilities.h"

class SqlSerializer {
  public:
    SqlSerializer(const std::string &database_path,
                  const std::string &schema_path, bool truncate = false,
                  int verbose = 0);
    ~SqlSerializer();
    std::string get_database_filepath() const;
    void serialize_start_trace();
    void serialize_finish_trace();
    void serialize_function_entry(dyntrace_context_t *context,
                                  const closure_info_t &info);
    void serialize_function_exit(const closure_info_t &info);
    void serialize_builtin_entry(dyntrace_context_t *context,
                                 const builtin_info_t &info);
    void serialize_builtin_exit(const builtin_info_t &info);
    void serialize_promise_origin(prom_id_t id, bool default_argument);
    void serialize_force_promise_entry(dyntrace_context_t *context,
                                       const prom_info_t &info, int clock_id);
    void serialize_force_promise_exit(const prom_info_t &info, int clock_id);
    void serialize_promise_created(const prom_basic_info_t &info);
    void serialize_promise_lookup(const prom_info_t &info, int clock_id);
    void serialize_promise_expression_lookup(const prom_info_t &info,
                                             int clock_id);
    void serialize_promise_lifecycle(const prom_lifecycle_info_t &info, int);
    void serialize_promise_argument_type(const prom_id_t prom_id,
                                         bool default_argument);
    void serialize_vector_alloc(const type_gc_info_t &info);
    void serialize_gc_exit(const gc_info_t &info);
    void serialize_unwind(const unwind_info_t &info);
    void serialize_new_environment(const env_id_t env_id, const fn_id_t fun_id);
    void serialize_metadatum(const std::string &key, const std::string &value);
    void serialize_variable(var_id_t variable_id, const std::string &name,
                            env_id_t environment_id);
    void serialize_variable_action(prom_id_t promise_id, var_id_t variable_id,
                                   const std::string &action);
    void serialize_function_environment_action(fn_id_t function_id,
                                               const std::vector<int> &actions);
    void serialize_promise_environment_action(prom_id_t promise_id,
                                              const std::vector<int> &actions);
    void serialize_aggregated_environment_actions(const std::string context,
                                                  int end, int ena, int enr,
                                                  int enl);
    void serialize_trace(const std::string &opcode, const int id_1);
    void serialize_trace(const std::string &opcode, const int id_1,
                         const std::string id_2);
    void serialize_trace(const std::string &opcode, const int id_1,
                         const int id_2);
    void serialize_trace(const std::string &opcode, const int id_1,
                         const int id_2, const std::string &symbol,
                         const std::string sexptype);
    void serialize_trace(const std::string &opcode, const fn_id_t &id_1,
                         const int id_2, const int id_3);

  private:
    sqlite3_stmt *compile(const char *statement);
    void execute(sqlite3_stmt *statement);
    void open_database(const std::string database_path, bool truncate);
    void open_trace(const std::string database_path, bool truncate);
    void close_trace();
    void close_database();
    void create_tables(const std::string schema_path);
    void prepare_statements();
    void finalize_statements();
    void cleanup();
    void unindent();
    void indent();

    sqlite3_stmt *populate_promise_evaluation_statement(const prom_info_t &info,
                                                        const int type,
                                                        int clock_id);

    sqlite3_stmt *populate_call_statement(const call_info_t &info);

    sqlite3_stmt *populate_call_return_statement(const call_info_t &info);

    sqlite3_stmt *
    populate_insert_promise_statement(const prom_basic_info_t &info);

    sqlite3_stmt *
    populate_promise_association_statement(const closure_info_t &info,
                                           int index);

    sqlite3_stmt *populate_metadata_statement(const string &key,
                                              const string &value);
    sqlite3_stmt *populate_function_statement(const call_info_t &info);

    sqlite3_stmt *
    populate_insert_argument_statement(const closure_info_t &info,
                                       int actual_parameter_position);

    sqlite3_stmt *populate_trace_statement(const string &);

    std::string database_filepath;
    int verbose;
    int indentation;
    sqlite3 *database = NULL;
    std::ofstream trace;
    sqlite3_stmt *insert_metadata_statement = NULL;
    sqlite3_stmt *insert_function_statement = NULL;
    sqlite3_stmt *insert_argument_statement = NULL;
    sqlite3_stmt *insert_call_statement = NULL;
    sqlite3_stmt *insert_call_return_statement = NULL;
    sqlite3_stmt *insert_promise_statement = NULL;
    sqlite3_stmt *insert_promise_association_statement = NULL;
    sqlite3_stmt *insert_promise_evaluation_statement = NULL;
    sqlite3_stmt *insert_promise_return_statement = NULL;
    sqlite3_stmt *insert_promise_lifecycle_statement = NULL;
    sqlite3_stmt *insert_promise_argument_type_statement = NULL;
    sqlite3_stmt *insert_gc_trigger_statement = NULL;
    sqlite3_stmt *insert_type_distribution_statement = NULL;
    sqlite3_stmt *insert_environment_statement = NULL;
    sqlite3_stmt *insert_variable_statement = NULL;
    sqlite3_stmt *insert_variable_action_statement = NULL;
    sqlite3_stmt *insert_jump_statement = NULL;
    sqlite3_stmt *insert_function_environment_action_statement = NULL;
    sqlite3_stmt *insert_promise_environment_action_statement = NULL;
    sqlite3_stmt *insert_aggregated_environment_action_statement = NULL;
    sqlite3_stmt *insert_trace_statement = NULL;
};

#endif /* __SQL_SERIALIZER_H__ */
