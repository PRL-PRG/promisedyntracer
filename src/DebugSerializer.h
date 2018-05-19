#ifndef PROMISEDYNTRACER_DEBUG_SERIALIZER_H
#define PROMISEDYNTRACER_DEBUG_SERIALIZER_H
#include "State.h"
#include <string>

class DebugSerializer {
  public:
    DebugSerializer(bool verbose);
    ~DebugSerializer();

    void serialize_start_trace();
    void serialize_finish_trace();
    void serialize_function_entry(const closure_info_t &);
    void serialize_function_exit(const closure_info_t &);
    void serialize_builtin_entry(const builtin_info_t &);
    void serialize_builtin_exit(const builtin_info_t &);
    void serialize_promise_origin(prom_id_t id, bool default_argument);
    void serialize_force_promise_entry(const prom_info_t &);
    void serialize_force_promise_exit(const prom_info_t &);
    void serialize_promise_created(const prom_basic_info_t &);
    void serialize_promise_lookup(const prom_info_t &);
    void serialize_promise_expression_lookup(const prom_info_t &);
    void serialize_promise_argument_type(const prom_id_t prom_id,
                                         bool default_argument);
    void serialize_vector_alloc(const type_gc_info_t &);
    void serialize_gc_exit(const gc_info_t &);
    void serialize_begin_ctxt(const RCNTXT *);
    void serialize_unwind(const unwind_info_t &);
    void serialize_end_ctxt(const RCNTXT *);
    void serialize_new_environment(const env_id_t env_id, const fn_id_t fun_id);
    void serialize_metadatum(const std::string &key, const std::string &value);
    void serialize_variable(var_id_t variable_id, const std::string &name,
                            env_id_t environment_id);
    void serialize_variable_action(prom_id_t promise_id, var_id_t variable_id,
                                   const std::string &action);
    void serialize_interference_information(const std::string &information);

    void setState(tracer_state_t *);

    bool needsState();

  private:
    tracer_state_t *state;
    bool has_state;
    int indentation;
    bool verbose;
    std::string prefix();
    void indent();
    std::string unindent();

    std::string log_line(const RCNTXT *cptr);
    std::string log_line(const stack_event_t &event);
    std::string log_line(const function_type &type);
    std::string log_line(const arglist_t &arguments);
    std::string log_line(const sexptype_t &type);
    std::string log_line(const full_sexp_type &type);
    std::string log_line(const builtin_info_t &);
    std::string log_line(const closure_info_t &);
    std::string log_line(const prom_basic_info_t &);
    std::string log_line(const prom_info_t &);
    std::string log_line(const unwind_info_t &);
    std::string log_line(const gc_info_t &);
    std::string log_line(const type_gc_info_t &);

    std::string print_stack();
};

#endif // PROMISEDYNTRACER_DEBUG_SERIALIZER_H
