#ifndef __TIMER_H__
#define __TIMER_H__

#ifdef DYNTRACE_ENABLE_TIMING

#include "EnumFactory.h"
#include <chrono>
#include <string>
#include <vector>

#define TIMER_SEGMENT_ENUM(XX)                                                 \
    XX(BEGIN_SETUP, = 0)                                                       \
    XX(BEGIN_ANALYSIS, )                                                       \
                                                                               \
    XX(FUNCTION_ENTRY_RECORDER, )                                              \
    XX(FUNCTION_ENTRY_RECORDER_OTHER, )                                        \
    XX(FUNCTION_ENTRY_RECORDER_FUNCTION_ID, )                                  \
    XX(FUNCTION_ENTRY_RECORDER_CALL_ID, )                                      \
    XX(FUNCTION_ENTRY_RECORDER_PARENT_ID, )                                    \
    XX(FUNCTION_ENTRY_RECORDER_LOCATION, )                                     \
    XX(FUNCTION_ENTRY_RECORDER_EXPRESSION, )                                   \
    XX(FUNCTION_ENTRY_RECORDER_NAME, )                                         \
    XX(FUNCTION_ENTRY_RECORDER_ARGUMENTS, )                                    \
    XX(FUNCTION_ENTRY_RECORDER_DEFINITION, )                                   \
    XX(FUNCTION_ENTRY_RECORDER_RECURSIVE, )                                    \
    XX(FUNCTION_ENTRY_RECORDER_PARENT_PROMISE, )                               \
                                                                               \
    XX(FUNCTION_ENTRY_STACK, )                                                 \
    XX(FUNCTION_ENTRY_ANALYSIS, )                                              \
    XX(FUNCTION_ENTRY_ANALYSIS_PROMISE_MAPPER, )                               \
    XX(FUNCTION_ENTRY_ANALYSIS_FUNCTION, )                                     \
    XX(FUNCTION_ENTRY_ANALYSIS_PROMISE_TYPE, )                                 \
    XX(FUNCTION_ENTRY_ANALYSIS_STRICTNESS, )                                   \
    XX(FUNCTION_ENTRY_WRITE_TRACE, )                                           \
                                                                               \
    XX(FUNCTION_EXIT_RECORDER, )                                               \
    XX(FUNCTION_EXIT_RECORDER_OTHER, )                                         \
    XX(FUNCTION_EXIT_RECORDER_FUNCTION_ID, )                                   \
    XX(FUNCTION_EXIT_RECORDER_CALL_ID, )                                       \
    XX(FUNCTION_EXIT_RECORDER_PARENT_ID, )                                     \
    XX(FUNCTION_EXIT_RECORDER_LOCATION, )                                      \
    XX(FUNCTION_EXIT_RECORDER_EXPRESSION, )                                    \
    XX(FUNCTION_EXIT_RECORDER_NAME, )                                          \
    XX(FUNCTION_EXIT_RECORDER_ARGUMENTS, )                                     \
    XX(FUNCTION_EXIT_RECORDER_DEFINITION, )                                    \
    XX(FUNCTION_EXIT_RECORDER_RECURSIVE, )                                     \
    XX(FUNCTION_EXIT_RECORDER_PARENT_PROMISE, )                                \
                                                                               \
    XX(FUNCTION_EXIT_STACK, )                                                  \
    XX(FUNCTION_EXIT_ANALYSIS, )                                               \
    XX(FUNCTION_EXIT_ANALYSIS_FUNCTION, )                                      \
    XX(FUNCTION_EXIT_ANALYSIS_STRICTNESS, )                                    \
    XX(FUNCTION_EXIT_WRITE_TRACE, )                                            \
                                                                               \
    XX(BUILTIN_ENTRY_RECORDER, )                                               \
    XX(BUILTIN_ENTRY_STACK, )                                                  \
    XX(BUILTIN_ENTRY_ANALYSIS, )                                               \
    XX(BUILTIN_ENTRY_ANALYSIS_FUNCTION, )                                      \
    XX(BUILTIN_ENTRY_WRITE_TRACE, )                                            \
                                                                               \
    XX(BUILTIN_EXIT_RECORDER, )                                                \
    XX(BUILTIN_EXIT_STACK, )                                                   \
    XX(BUILTIN_EXIT_ANALYSIS, )                                                \
    XX(BUILTIN_EXIT_ANALYSIS_FUNCTION, )                                       \
    XX(BUILTIN_EXIT_WRITE_TRACE, )                                             \
                                                                               \
    XX(CREATE_PROMISE_RECORDER, )                                              \
    XX(CREATE_PROMISE_ANALYSIS, )                                              \
    XX(CREATE_PROMISE_ANALYSIS_PROMISE_MAPPER, )                               \
    XX(CREATE_PROMISE_ANALYSIS_FUNCTION, )                                     \
    XX(CREATE_PROMISE_ANALYSIS_PROMISE_TYPE, )                                 \
    XX(CREATE_PROMISE_WRITE_TRACE, )                                           \
                                                                               \
    XX(FORCE_PROMISE_ENTRY_RECORDER, )                                         \
    XX(FORCE_PROMISE_ENTRY_STACK, )                                            \
    XX(FORCE_PROMISE_ENTRY_ANALYSIS, )                                         \
    XX(FORCE_PROMISE_ENTRY_ANALYSIS_PROMISE_MAPPER, )                          \
    XX(FORCE_PROMISE_ENTRY_ANALYSIS_PROMISE_EVALUATION_DISTANCE, )             \
    XX(FORCE_PROMISE_ENTRY_ANALYSIS_STRICTNESS, )                              \
    XX(FORCE_PROMISE_ENTRY_WRITE_TRACE, )                                      \
                                                                               \
    XX(FORCE_PROMISE_EXIT_RECORDER, )                                          \
    XX(FORCE_PROMISE_EXIT_STACK, )                                             \
    XX(FORCE_PROMISE_EXIT_ANALYSIS, )                                          \
    XX(FORCE_PROMISE_EXIT_ANALYSIS_PROMISE_TYPE, )                             \
    XX(FORCE_PROMISE_EXIT_WRITE_TRACE, )                                       \
                                                                               \
    XX(LOOKUP_PROMISE_VALUE_RECORDER, )                                        \
    XX(LOOKUP_PROMISE_VALUE_ANALYSIS, )                                        \
    XX(LOOKUP_PROMISE_VALUE_ANALYSIS_PROMISE_MAPPER, )                         \
    XX(LOOKUP_PROMISE_VALUE_ANALYSIS_STRICTNESS, )                             \
    XX(LOOKUP_PROMISE_VALUE_WRITE_TRACE, )                                     \
                                                                               \
    XX(LOOKUP_PROMISE_EXPRESSION_RECORDER, )                                   \
    XX(LOOKUP_PROMISE_EXPRESSION_ANALYSIS, )                                   \
    XX(LOOKUP_PROMISE_EXPRESSION_ANALYSIS_PROMISE_MAPPER, )                    \
    XX(LOOKUP_PROMISE_EXPRESSION_ANALYSIS_STRICTNESS, )                        \
    XX(LOOKUP_PROMISE_EXPRESSION_WRITE_TRACE, )                                \
                                                                               \
    XX(LOOKUP_PROMISE_ENVIRONMENT_RECORDER, )                                  \
    XX(LOOKUP_PROMISE_ENVIRONMENT_ANALYSIS, )                                  \
    XX(LOOKUP_PROMISE_ENVIRONMENT_ANALYSIS_PROMISE_MAPPER, )                   \
    XX(LOOKUP_PROMISE_ENVIRONMENT_ANALYSIS_STRICTNESS, )                       \
    XX(LOOKUP_PROMISE_ENVIRONMENT_WRITE_TRACE, )                               \
                                                                               \
    XX(SET_PROMISE_VALUE_RECORDER, )                                           \
    XX(SET_PROMISE_VALUE_ANALYSIS, )                                           \
    XX(SET_PROMISE_VALUE_ANALYSIS_PROMISE_MAPPER, )                            \
    XX(SET_PROMISE_VALUE_ANALYSIS_STRICTNESS, )                                \
    XX(SET_PROMISE_VALUE_WRITE_TRACE, )                                        \
                                                                               \
    XX(SET_PROMISE_EXPRESSION_RECORDER, )                                      \
    XX(SET_PROMISE_EXPRESSION_ANALYSIS, )                                      \
    XX(SET_PROMISE_EXPRESSION_ANALYSIS_PROMISE_MAPPER, )                       \
    XX(SET_PROMISE_EXPRESSION_ANALYSIS_STRICTNESS, )                           \
    XX(SET_PROMISE_EXPRESSION_WRITE_TRACE, )                                   \
                                                                               \
    XX(SET_PROMISE_ENVIRONMENT_RECORDER, )                                     \
    XX(SET_PROMISE_ENVIRONMENT_ANALYSIS, )                                     \
    XX(SET_PROMISE_ENVIRONMENT_ANALYSIS_PROMISE_MAPPER, )                      \
    XX(SET_PROMISE_ENVIRONMENT_ANALYSIS_STRICTNESS, )                          \
    XX(SET_PROMISE_ENVIRONMENT_WRITE_TRACE, )                                  \
                                                                               \
    XX(GC_PROMISE_UNMARKED_RECORDER, )                                         \
    XX(GC_PROMISE_UNMARKED_ANALYSIS, )                                         \
    XX(GC_PROMISE_UNMARKED_ANALYSIS_PROMISE_MAPPER, )                          \
    XX(GC_PROMISE_UNMARKED_ANALYSIS_STRICTNESS, )                              \
    XX(GC_PROMISE_UNMARKED_ANALYSIS_PROMISE_TYPE, )                            \
    XX(GC_PROMISE_UNMARKED_RECORD_KEEPING, )                                   \
                                                                               \
    XX(GC_FUNCTION_UNMARKED_RECORD_KEEPING, )                                  \
                                                                               \
    XX(GC_ENTRY_RECORDER, )                                                    \
                                                                               \
    XX(GC_EXIT_RECORDER, )                                                     \
                                                                               \
    XX(VECTOR_ALLOC_RECORDER, )                                                \
    XX(VECTOR_ALLOC_ANALYSIS, )                                                \
    XX(VECTOR_ALLOC_ANALYSIS_OBJECT_COUNT_SIZE, )                              \
                                                                               \
    XX(NEW_ENVIRONMENT_RECORDER, )                                             \
    XX(NEW_ENVIRONMENT_WRITE_TRACE, )                                          \
                                                                               \
    XX(CONTEXT_ENTRY_STACK, )                                                  \
                                                                               \
    XX(CONTEXT_JUMP_STACK, )                                                   \
    XX(CONTEXT_JUMP_ANALYSIS, )                                                \
    XX(CONTEXT_JUMP_ANALYSIS_STRICTNESS, )                                     \
                                                                               \
    XX(CONTEXT_EXIT_STACK, )                                                   \
                                                                               \
    XX(ENVIRONMENT_ACTION_RECORDER, )                                          \
    XX(ENVIRONMENT_ACTION_ANALYSIS, )                                          \
    XX(ENVIRONMENT_ACTION_ANALYSIS_SIDE_EFFECT, )                              \
    XX(ENVIRONMENT_ACTION_WRITE_TRACE, )                                       \
                                                                               \
    XX(END_CHECK, )                                                            \
    XX(END_ANALYSIS, )                                                         \
    XX(END_ANALYSIS_PROMISE_MAPPER, )                                          \
    XX(END_ANALYSIS_PROMISE_TYPE, )                                            \
    XX(END_ANALYSIS_METADATA, )                                                \
    XX(END_ANALYSIS_OBJECT_COUNT_SIZE, )                                       \
    XX(END_ANALYSIS_FUNCTION, )                                                \
    XX(END_ANALYSIS_PROMISE, )                                                 \
    XX(END_ANALYSIS_PROMISE_EVALUATION_DISTANCE, )                             \
    XX(END_ANALYSIS_STRICTNESS, )                                              \
    XX(END_ANALYSIS_SIDE_EFFECT, )                                             \
                                                                               \
    XX(TIMER_SEGMENT_COUNT, )

DECLARE_ENUM(TimerSegment, TIMER_SEGMENT_ENUM, timer_segment_name,
             timer_segment_value)

class Timer {
  public:
    void start();
    void reset();
    void zero();
    void end_segment(TimerSegment segment);
    std::vector<std::pair<std::string, std::string>> stats();

    Timer(Timer const &) = delete;
    Timer(Timer &&) = delete;
    Timer &operator=(Timer const &) = delete;
    Timer &operator=(Timer &&) = delete;

    static Timer &main_timer() {
        static Timer main_timer("MAIN");
        return main_timer;
    }

    static Timer &recorder_timer() {
        static Timer recorder_timer("RECORDER");
        return recorder_timer;
    }

    static Timer &analysis_timer() {
        static Timer analysis_timer("ANALYSIS");
        return analysis_timer;
    }

  private:
    Timer(const std::string &name) : name_(name){};

    std::string name_;
    long timers[TimerSegment::TIMER_SEGMENT_COUNT] = {0};
    long occurrences[TimerSegment::TIMER_SEGMENT_COUNT] = {0};
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
};

#define MAIN_TIMER_RESET() Timer::main_timer().reset();

#define MAIN_TIMER_END_SEGMENT(segment_name)                                   \
    Timer::main_timer().end_segment(TimerSegment::segment_name);

#define RECORDER_TIMER_RESET() Timer::recorder_timer().reset();

#define RECORDER_TIMER_END_SEGMENT(segment_name)                               \
    Timer::recorder_timer().end_segment(TimerSegment::segment_name);

#define ANALYSIS_TIMER_RESET() Timer::recorder_timer().reset();

#define ANALYSIS_TIMER_END_SEGMENT(segment_name)                               \
    Timer::analysis_timer().end_segment(TimerSegment::segment_name);

#else /* DYNTRACE_ENABLE_TIMING */

#define MAIN_TIMER_RESET()                                                     \
    {}
#define MAIN_TIMER_END_SEGMENT(segment_name)                                   \
    {}
#define RECORDER_TIMER_RESET()                                                 \
    {}
#define RECORDER_TIMER_END_SEGMENT(segment_name)                               \
    {}
#define ANALYSIS_TIMER_RESET()                                                 \
    {}
#define ANALYSIS_TIMER_END_SEGMENT(segment_name)                               \
    {}

#endif /* DYNTRACE_ENABLE_TIMING */

#endif /* __TIMER_H__ */
