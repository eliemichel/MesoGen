/**
 * This file is part of MesoGen, the reference implementation of:
 *
 *   Michel, Élie and Boubekeur, Tamy (2023).
 *   MesoGen: Designing Procedural On-Surface Stranded Mesostructures,
 *   ACM Transaction on Graphics (SIGGRAPH '23 Conference Proceedings),
 *   https://doi.org/10.1145/3588432.3591496
 *
 * Copyright (c) 2020 - 2023 -- Télécom Paris (Élie Michel <emichel@adobe.com>)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * The Software is provided "as is", without warranty of any kind, express or
 * implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose and non-infringement. In no event shall the
 * authors or copyright holders be liable for any claim, damages or other
 * liability, whether in an action of contract, tort or otherwise, arising
 * from, out of or in connection with the software or the use or other dealings
 * in the Software.
 */
/**
 * A simple timer library.
 *
 * Basic usage:
 *    using TinyTimer::Timer;
 *    Timer timer;
 *    // .. do something
 *    cout << "Something took " << timer.ellapsed() << " seconds" << endl;
 *
 * One can also consolidate several timings to measure standard deviation:
 *    PerformanceCounter perf;
 *    for (...) {
 *        Timer timer;
 *        // .. do something
 *        perf.add_sample(timer);
 *    }
 *    cout << "Something took " << perf.average() << "±" << perf.stddev() << " seconds" << endl;
 *
 * To make it easier to measure performances at any point of the code, a
 * global pool of counters is defined *statically* (there is no runtime cost
 * in accessing the counter):
 *    // Somewhere deep in your code:
 *    Timer timer;
 *    // .. do something
 *    PERF(42).add_sample(timer);
 *
 *    // Somewhere else, for instance at the end of main():
 *    cout << "Something took " << PERF(42).average() << "±" << PERF(42).stddev() << " seconds" << endl;
 *
 * If only the index "42" is used in the code then only one global PerformanceCounter
 * is defined. Counter index can only be int, or at least something that might be cast
 * into an int, so the recommended way of specifying it is by defining an enum:
 *    enum PerfCounterType {
 *        SomethingTime,
 *        SomethingElseTime,
 *    };
 *
 *    Timer timer;
 *    // .. do something
 *    PERF(SomethingTime).add_sample(timer);
 *
 *    timer.reset();
 *    // .. do something else
 *    PERF(SomethingElseTime).add_sample(timer);
 *
 *    cout << "Something:      " << PERF(SomethingTime) << endl;
 *    cout << "Something else: " << PERF(SomethingElseTime) << endl;
 *
 * Actually using enum values is more flexible because then you may mix keys
 * from different enums:
 *    enum PerfCounterType {
 *        SomethingTime = 0,
 *    };
 *    enum AnotherPerfCounterType {
 *        SomethingElseTime = 0, // even if they correspond to same value!
 *    };
 *    assert(PERF(SomethingTime) != PERF(SomethingElseTime))
 */

#pragma once

#include <chrono>
#include <cmath>
#include <algorithm>
#include <string>
#include <sstream>

namespace TinyTimer {

class Timer
{
public:
    Timer() noexcept {
        reset();
    }

    void reset() noexcept {
        begin = ::std::chrono::high_resolution_clock::now();
    }

    /**
     * Ellapsed time since either construction or last call to reset(), in nanoseconds
     */
    ::std::chrono::nanoseconds ellapsedNanoseconds() const noexcept {
        auto end = ::std::chrono::high_resolution_clock::now();
        auto elapsed = ::std::chrono::duration_cast<::std::chrono::nanoseconds>(end - begin);
        return elapsed;
    }

    /**
     * Ellapsed time since either construction or last call to reset(), in seconds
     */
    double ellapsed() const noexcept {
        return ellapsedNanoseconds().count() * 1e-9;
    }

private:
    ::std::chrono::time_point<::std::chrono::high_resolution_clock> begin;
};

/**
 * Accumulate time measurements, providing standard deviation
 */
class PerformanceCounter
{
public:
    PerformanceCounter() noexcept {}

    void reset() noexcept {
        m_sample_count = 0;
        m_accumulated = 0;
        m_accumulated_sq = 0;
    }

    int samples() const noexcept {
        return m_sample_count;
    }

    double average() const noexcept {
        return m_sample_count == 0 ? 0 : m_accumulated / static_cast<double>(m_sample_count);
    }

    double stddev() const noexcept {
        if (m_sample_count == 0) return 0;
        double avg = average();
        double var = m_accumulated_sq / static_cast<double>(m_sample_count) - avg * avg;
        return ::std::sqrt(::std::max(0.0, var));
    }

    void add_sample(const Timer& timer) noexcept {
        add_sample(timer.ellapsed());
    }

    /**
     * Add a sample measure, in seconds
     */
    void add_sample(double ellapsed) noexcept {
        ++m_sample_count;
        m_accumulated += ellapsed;
        m_accumulated_sq += ellapsed * ellapsed;
    }

    ::std::string summary() const noexcept {
        ::std::ostringstream ss;
        ss << average() * 1e3 << "ms";
        ss << " (+-" << stddev() * 1e3 << "ms,";
        ss << " " << m_sample_count << " samples)";
        return ss.str();
    }

private:
    unsigned int m_sample_count = 0;
    double m_accumulated = 0;
    double m_accumulated_sq = 0;
};

inline ::std::ostream& operator<<(::std::ostream& os, const PerformanceCounter& counter) {
    return os << "PerformanceCounter(" << counter.summary() << ")";
}

template<typename Enum, Enum key>
struct counters {
    static inline PerformanceCounter Value = PerformanceCounter();
};

#define PERF(key) TinyTimer::counters<decltype(key), key>::Value

}
