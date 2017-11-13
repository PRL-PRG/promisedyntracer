#ifndef __SQL_SERIALIZER_H__
#define __SQL_SERIALIZER_H__

#include "State.h"
#include "sqlite3.h"
#include "stdlibs.h"
#include "utilities.h"

class SqlSerializer {
  public:
    SqlSerializer(const std::string &database_path,
                  const std::string &schema_path, bool verbose = false);
    ~SqlSerializer();
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
    void serialize_promise_lifecycle(const prom_gc_info_t &info);
    void serialize_promise_argument_type(const prom_id_t prom_id,
                                         bool default_argument);
    void serialize_vector_alloc(const type_gc_info_t &info);
    void serialize_gc_exit(const gc_info_t &info);
    void serialize_unwind(const unwind_info_t &info);
    void serialize_new_environment(const env_id_t env_id, const fn_id_t fun_id);
    void serialize_metadatum(const std::string& key, const std::string& value);

  private:
    sqlite3_stmt *compile(const char *statement);
    void execute(sqlite3_stmt *statement);
    void open_database(const std::string database_path);
    void close_database();
    void create_tables(const std::string schema_path);
    void prepare_statements();
    void finalize_statements();
    void unindent();
    void indent();

    sqlite3_stmt *populate_promise_evaluation_statement(const prom_info_t &info,
                                                        const int type,
                                                        int clock_id);

    sqlite3_stmt *populate_call_statement(const call_info_t &info);

    sqlite3_stmt *
    populate_insert_promise_statement(const prom_basic_info_t &info);

    sqlite3_stmt *
    populate_promise_association_statement(const closure_info_t &info,
                                           int index);

    sqlite3_stmt *populate_metadata_statement(const string key,
                                              const string value);
    sqlite3_stmt *populate_function_statement(const call_info_t &info);

    sqlite3_stmt *populate_insert_argument_statement(const closure_info_t &info,
                                                     int index);
    bool verbose;
    int indentation;
    sqlite3 *database = nullptr;
    sqlite3_stmt *insert_metadata_statement = nullptr;
    sqlite3_stmt *insert_function_statement = nullptr;
    sqlite3_stmt *insert_argument_statement = nullptr;
    sqlite3_stmt *insert_call_statement = nullptr;
    sqlite3_stmt *insert_promise_statement = nullptr;
    sqlite3_stmt *insert_promise_association_statement = nullptr;
    sqlite3_stmt *insert_promise_evaluation_statement = nullptr;
    sqlite3_stmt *insert_promise_return_statement = nullptr;
    sqlite3_stmt *insert_promise_lifecycle_statement = nullptr;
    sqlite3_stmt *insert_promise_argument_type_statement = nullptr;
    sqlite3_stmt *insert_gc_trigger_statement = nullptr;
    sqlite3_stmt *insert_type_distribution_statement = nullptr;
    sqlite3_stmt *insert_environment_statement = nullptr;
};

#endif /* __SQL_SERIALIZER_H__ */
