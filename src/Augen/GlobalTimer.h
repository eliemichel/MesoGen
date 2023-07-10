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


#pragma once

#include <OpenGL>

#include <glm/glm.hpp>
#include <refl.hpp>

#include <string>
#include <chrono>
#include <set>
#include <map>
#include <memory>
#include <fstream>

/**
 * Global timer is used as a singleton to record render timings using both CPU
 * side timings and GPU timer queries.
 */
class GlobalTimer {
public:
    typedef void* TimerHandle;
    struct StatsUi {
        bool visible = true;
        glm::vec3 color = glm::vec3(0.0);

        StatsUi();
    private:
        static int s_counter;
    };
    struct Stats {
        int sampleCount = 0;
        double cumulatedTime = 0.0;
        double cumulatedFrameOffset = 0.0; // time within a frame at which the timer started
        double cumulatedGpuTime = 0.0;
        double cumulatedGpuFrameOffset = 0.0;

        // for stats recording
        double lastTime = 0.0;
        double lastFrameOffset = 0.0;
        double lastGpuTime = 0.0;
        double lastGpuFrameOffset = 0.0;

        // for UI -- not reset by reset()
        mutable StatsUi ui;

        void reset() noexcept;
    };

public:
    // static API
    static std::shared_ptr<GlobalTimer> GetInstance() noexcept { if (!s_instance) s_instance = std::make_shared<GlobalTimer>(); return s_instance; }
    static TimerHandle Start(const std::string& message) noexcept { return GetInstance()->start(message); }
    static void Stop(TimerHandle handle) noexcept { GetInstance()->stop(handle); }
    static void StartFrame() noexcept { return GetInstance()->startFrame(); }
    static void StopFrame() noexcept { GetInstance()->stopFrame(); }

public:
    struct Properties {
        bool showDiagram = false;
        float decay = 0.05f;
    };
    Properties& properties() { return m_properties; }
    const Properties& properties() const { return m_properties; }

public:
    GlobalTimer();
    ~GlobalTimer();
    GlobalTimer& operator=(const GlobalTimer&) = delete;
    GlobalTimer(const GlobalTimer&) = delete;

    TimerHandle start(const std::string & message) noexcept;
    void stop(TimerHandle handle) noexcept;
    void startFrame() noexcept;
    void stopFrame() noexcept;
    const std::map<std::string, Stats>& stats() const noexcept { return m_stats; }
    void resetAllStats() noexcept;
    const Stats& frameStats() const noexcept { return m_frameStats; }

private:
    struct Timer {
        std::chrono::high_resolution_clock::time_point startTime;
        std::string message;
        GLuint queries[2]; // GPU timer queries for begin and end

        Timer();
        ~Timer();
        Timer(const Timer&) = delete;
        Timer & operator=(const Timer&) = delete;
    };

    // sampleCount must have been incremented first
    void addSample(double& accumulator, double dt, int sampleCount) noexcept;
    void gatherQueries() noexcept;
    void initStats() noexcept;
    void writeStats() noexcept;

private:
    static std::shared_ptr<GlobalTimer> s_instance;

private:
    Properties m_properties;
    Timer m_frameTimer; // special timer global to the frame
    Stats m_frameStats;
    std::set<Timer*> m_pool; // running timers
    std::set<Timer*> m_stopped; // stopped timers of which queries are waiting to get read back
    std::map<std::string, Stats> m_stats; // cumulated statistics

    // stats
    std::string m_outputStats;
    std::ofstream m_outputStatsFile;
    int m_statFrame = 0;
};

REFL_TYPE(GlobalTimer::Properties)
REFL_FIELD(showDiagram)
REFL_FIELD(decay)
REFL_END

class ScopedTimer {
public:
    ScopedTimer(const std::string& message) : m_handle(GlobalTimer::Start(message)) {}
    ~ScopedTimer() { GlobalTimer::Stop(m_handle); }
private:
    GlobalTimer::TimerHandle m_handle;
};
