#ifdef DYNTRACE_ENABLE_TIMING

#include "Timer.h"

DEFINE_ENUM(TimerSegment, TIMER_SEGMENT_ENUM, timer_segment_name,
            timer_segment_value)

void Timer::start() { start_time = std::chrono::high_resolution_clock::now(); }

void Timer::reset() { start_time = std::chrono::high_resolution_clock::now(); }

void Timer::zero() {
    for (int i = 0; i < TimerSegment::TIMER_SEGMENT_COUNT; i++)
        timers[i] = 0;
}

void Timer::end_segment(TimerSegment segment) {
    auto end_time = std::chrono::high_resolution_clock::now();
    const long duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
                              end_time - start_time)
                              .count();
    timers[segment] += duration;
    occurrences[segment]++;
    start_time = std::chrono::high_resolution_clock::now();
}

std::vector<std::pair<std::string, std::string>> Timer::stats() {
    std::vector<std::pair<std::string, std::string>> r;

    for (int i = 0; i < TimerSegment::TIMER_SEGMENT_COUNT; i++) {
        r.push_back(std::make_pair(
            "TIMER_" + name_ + "_" + timer_segment_name((TimerSegment)i),
            std::to_string(timers[i]) + "/" + std::to_string(occurrences[i])));
    }

    return r;
}

#endif /* DYNTRACE_ENABLE_TIMING */
