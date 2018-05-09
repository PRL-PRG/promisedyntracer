#ifdef DYNTRACE_ENABLE_TIMING

#include "Timer.h"

using namespace std;
using namespace std::chrono;

DEFINE_ENUM(TimerSegment, TIMER_SEGMENT_ENUM, timer_segment_name,
            timer_segment_value)

void Timer::start() { start_time = chrono::high_resolution_clock::now(); }

void Timer::reset() { start_time = chrono::high_resolution_clock::now(); }

void Timer::zero() {
    for (int i = 0; i < TimerSegment::TIMER_SEGMENT_COUNT; i++)
        timers[i] = 0;
}

void Timer::end_segment(TimerSegment segment) {
    auto end_time = chrono::high_resolution_clock::now();
    const long duration =
        chrono::duration_cast<chrono::nanoseconds>(end_time - start_time)
            .count();
    timers[segment] += duration;
    occurrences[segment]++;
    start_time = chrono::high_resolution_clock::now();
}

std::vector<std::pair<std::string, std::string>> Timer::stats() {
    vector<pair<string, string>> r;

    for (int i = 0; i < TimerSegment::TIMER_SEGMENT_COUNT; i++) {
        r.push_back(make_pair(
            "TIMER_" + name_ + "_" + timer_segment_name((TimerSegment)i),
            to_string(timers[i]) + "/" + to_string(occurrences[i])));
    }

    return r;
}

#endif /* DYNTRACE_ENABLE_TIMING */
