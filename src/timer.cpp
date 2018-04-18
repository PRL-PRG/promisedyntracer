//
// Created by kondziu on 18.4.18.
//

#include "timer.h"
using namespace std;
using namespace std::chrono;

namespace timing {

    string segment_names[] = {
        "FUNCTION_ENTRY_RECORDER",
        "FUNCTION_ENTRY_STACK",
        "FUNCTION_ENTRY_WRITE_SQL",
        "FUNCTION_ENTRY_WRITE_TRACE",

        "FUNCTION_EXIT_RECORDER",
        "FUNCTION_EXIT_STACK",
        "FUNCTION_EXIT_WRITE_SQL",
        "FUNCTION_EXIT_WRITE_TRACE",

        "BUILTIN_ENTRY_RECORDER",
        "BUILTIN_ENTRY_STACK",
        "BUILTIN_ENTRY_WRITE_SQL",
        "BUILTIN_ENTRY_WRITE_TRACE",

        "BUILTIN_EXIT_RECORDER",
        "BUILTIN_EXIT_STACK",
        "BUILTIN_EXIT_WRITE_SQL",
        "BUILTIN_EXIT_WRITE_TRACE",

        "CREATE_PROMISE_RECORDER",
        "CREATE_PROMISE_WRITE_SQL",
        "CREATE_PROMISE_WRITE_TRACE",

        "FORCE_PROMISE_ENTRY_RECORDER",
        "FORCE_PROMISE_ENTRY_STACK",
        "FORCE_PROMISE_ENTRY_WRITE_SQL",
        "FORCE_PROMISE_ENTRY_WRITE_TRACE",

        "FORCE_PROMISE_EXIT_RECORDER",
        "FORCE_PROMISE_EXIT_STACK",
        "FORCE_PROMISE_EXIT_WRITE_SQL",
        "FORCE_PROMISE_EXIT_WRITE_TRACE",

        "LOOKUP_PROMISE_VALUE_RECORDER",
        "LOOKUP_PROMISE_VALUE_WRITE_SQL",
        "LOOKUP_PROMISE_VALUE_WRITE_TRACE",

        "LOOKUP_PROMISE_EXPRESSION_RECORDER",
        "LOOKUP_PROMISE_EXPRESSION_WRITE_SQL",
        "LOOKUP_PROMISE_EXPRESSION_WRITE_TRACE",

        "LOOKUP_PROMISE_ENVIRONMENT_RECORDER",
        "LOOKUP_PROMISE_ENVIRONMENT_WRITE_SQL",
        "LOOKUP_PROMISE_ENVIRONMENT_WRITE_TRACE",

        "SET_PROMISE_VALUE_RECORDER",
        "SET_PROMISE_VALUE_WRITE_SQL",
        "SET_PROMISE_VALUE_WRITE_TRACE",

        "SET_PROMISE_EXPRESSION_RECORDER",
        "SET_PROMISE_EXPRESSION_WRITE_SQL",
        "SET_PROMISE_EXPRESSION_WRITE_TRACE",

        "SET_PROMISE_ENVIRONMENT_RECORDER",
        "SET_PROMISE_ENVIRONMENT_WRITE_SQL",
        "SET_PROMISE_ENVIRONMENT_WRITE_TRACE",

        "GC_PROMISE_UNMARKED_RECORDER",
        "GC_PROMISE_UNMARKED_WRITE_SQL",

        "GC_ENTRY_RECORDER",
        "GC_ENTRY_WRITE_SQL",

        "GC_EXIT_RECORDER",
        "GC_EXIT_WRITE_SQL",

        "VECTOR_ALLOC_RECORDER",
        "VECTOR_ALLOC_WRITE_SQL",

        "NEW_ENVIRONMENT_RECORDER",
        "NEW_ENVIRONMENT_WRITE_SQL",
        "NEW_ENVIRONMENT_WRITE_TRACE",

        "CONTEXT_ENTRY_STACK",
        "CONTEXT_JUMP_STACK",
        "CONTEXT_EXIT_STACK"
    };



    void Timer::start() {
        start_time = chrono::high_resolution_clock::now();
    }

    void Timer::reset() {
        start_time = chrono::high_resolution_clock::now();
    }

    void Timer::zero() {
        for (int i = 0; i < number_of_segments; i++)
                timers[i] = 0;
    }

    void Timer::endSegment(segment s) {
        auto end_time = chrono::high_resolution_clock::now();
        const long duration = chrono::duration_cast<chrono::nanoseconds>(end_time-start_time).count();
        timers[s] += duration;
        occurances[s]++;
    }

    std::vector<std::pair<std::string, std::string>> Timer::stats() {
        vector<pair<string, string>> r;

        for (int i = 0; i < number_of_segments; i++) {
            r.push_back(make_pair("TIMING_" + segment_names[i], to_string(timers[i])));
            r.push_back(make_pair("OCCURANCE_" + segment_names[i], to_string(occurances[i])));
        }

        return r;
    }
}