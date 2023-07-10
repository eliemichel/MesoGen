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

#include "GlobalTimer.h"
#include "Logger.h"

#include <filesystem>
namespace fs = std::filesystem;

int GlobalTimer::StatsUi::s_counter = 0;

GlobalTimer::StatsUi::StatsUi() {
	// Iterate through these colors
	constexpr glm::vec3 predefinedColors[] = {
		{225.f / 255.f,  25.f / 255.f,  96.f / 255.f},
		{225.f / 255.f,  96.f / 255.f,  25.f / 255.f},
		{ 15.f / 255.f, 167.f / 255.f, 134.f / 255.f},
		{106.f / 255.f,  81.f / 255.f, 200.f / 255.f},
	};
	color = predefinedColors[(s_counter++) % 4];
}

//-----------------------------------------------------------------------------

GlobalTimer::Timer::Timer()
{
	if (glad_glCreateQueries) {
		glCreateQueries(GL_TIMESTAMP, 2, queries);
	}
}

GlobalTimer::Timer::~Timer()
{
	if (glad_glCreateQueries) {
		glDeleteQueries(2, queries);
	}
}

//-----------------------------------------------------------------------------

std::shared_ptr<GlobalTimer> GlobalTimer::s_instance;

void GlobalTimer::Stats::reset() noexcept
{
	sampleCount = 0;
	cumulatedTime = 0.0;
	cumulatedFrameOffset = 0.0;
	cumulatedGpuTime = 0.0;
	cumulatedGpuFrameOffset = 0.0;
}

//-----------------------------------------------------------------------------

GlobalTimer::GlobalTimer()
{}

GlobalTimer::~GlobalTimer()
{
	gatherQueries();

	if (!m_pool.empty()) {
		WARN_LOG << "Program terminates but some timers are still running";
	}

	while (!m_pool.empty()) {
		auto it = m_pool.begin();
		delete *it;
		m_pool.erase(it);
	}
}

GlobalTimer::TimerHandle GlobalTimer::start(const std::string& message) noexcept
{
	Timer *timer = new Timer();
	m_pool.insert(timer);
	timer->message = message;
	if (glad_glQueryCounter) {
		glQueryCounter(timer->queries[0], GL_TIMESTAMP);
	}
	timer->startTime = std::chrono::high_resolution_clock::now();
	return static_cast<void*>(timer);
}

template <typename Duration>
static double milliseconds(Duration dt)
{
	return std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1, 1000>>>(dt).count();
}

void GlobalTimer::stop(TimerHandle handle) noexcept
{
	auto endTime = std::chrono::high_resolution_clock::now();
	Timer* timer = static_cast<Timer*>(handle);
	if (glad_glQueryCounter) {
		glQueryCounter(timer->queries[1], GL_TIMESTAMP);
	}
	if (m_pool.count(timer) == 0) {
		WARN_LOG << "Invalid TimerHandle";
		return;
	}
	m_pool.erase(timer);
	m_stopped.insert(timer);

	Stats& stats = m_stats[timer->message];
	stats.sampleCount++;
	stats.lastTime = milliseconds(endTime - timer->startTime);
	stats.lastFrameOffset = milliseconds(timer->startTime - m_frameTimer.startTime);
	addSample(stats.cumulatedTime, stats.lastTime, stats.sampleCount);
	addSample(stats.cumulatedFrameOffset, stats.lastFrameOffset, stats.sampleCount);
}

void GlobalTimer::startFrame() noexcept
{
	glQueryCounter(m_frameTimer.queries[0], GL_TIMESTAMP);
	m_frameTimer.startTime = std::chrono::high_resolution_clock::now();
}

void GlobalTimer::stopFrame() noexcept
{
	auto endTime = std::chrono::high_resolution_clock::now();
	glQueryCounter(m_frameTimer.queries[1], GL_TIMESTAMP);
	m_frameStats.sampleCount++;
	m_frameStats.lastTime = milliseconds(endTime - m_frameTimer.startTime);
	addSample(m_frameStats.cumulatedTime, m_frameStats.lastTime, m_frameStats.sampleCount);

	gatherQueries();
	writeStats();
}

void GlobalTimer::resetAllStats() noexcept
{
	m_frameStats.reset();
	for (auto& s : m_stats) {
		s.second.reset();
	}
}

void GlobalTimer::addSample(double& accumulator, double dt, int sampleCount) noexcept
{
	double decay = properties().decay;
	if (sampleCount == 1) {
		accumulator = dt;
	}
	else if (decay == 0) {
		double c = static_cast<double>(sampleCount);
		accumulator = (dt + accumulator * (c - 1)) / c;
	}
	else {
		accumulator = dt * decay + accumulator * (1 - decay);
	}
}

void GlobalTimer::gatherQueries() noexcept
{
	GLuint64 frameStartNs = 0, frameEndNs = 0;
	if (glad_glGetQueryObjectui64v) {
		glGetQueryObjectui64v(m_frameTimer.queries[0], GL_QUERY_RESULT, &frameStartNs);
		glGetQueryObjectui64v(m_frameTimer.queries[1], GL_QUERY_RESULT, &frameEndNs);
	}

	m_frameStats.lastGpuTime = static_cast<double>(frameEndNs - frameStartNs) * 1e-6;
	addSample(m_frameStats.cumulatedGpuTime, m_frameStats.lastGpuTime, m_frameStats.sampleCount);
	
	while (!m_stopped.empty()) {
		auto it = m_stopped.begin();
		Timer* timer = *it;
		m_stopped.erase(it);

		GLuint64 startNs = 0, endNs = 0;
		if (glad_glGetQueryObjectui64v) {
			glGetQueryObjectui64v(timer->queries[0], GL_QUERY_RESULT, &startNs);
			glGetQueryObjectui64v(timer->queries[1], GL_QUERY_RESULT, &endNs);
		}

		Stats& stats = m_stats[timer->message];
		stats.lastGpuTime = static_cast<double>(endNs - startNs) * 1e-6;
		stats.lastGpuFrameOffset = static_cast<double>(startNs - frameStartNs) * 1e-6;
		addSample(stats.cumulatedGpuTime, stats.lastGpuTime, stats.sampleCount);
		addSample(stats.cumulatedGpuFrameOffset, stats.lastGpuFrameOffset, stats.sampleCount);

		delete timer;
	}
}

void GlobalTimer::initStats() noexcept
{
	//m_outputStats = ResourceManager::resolveResourcePath(m_outputStats);
	fs::create_directories(fs::path(m_outputStats).parent_path());
	m_outputStatsFile.open(m_outputStats);
	m_outputStatsFile << "frame;raw counters(json);smoothed counters(json)\n";
	m_statFrame = 0;
}

void GlobalTimer::writeStats() noexcept
{
	if (!m_outputStatsFile.is_open()) return;

	m_outputStatsFile << m_statFrame << ";";

	m_outputStatsFile
		<< "{\"Frame\": {"
		<< "\"time\": " << m_frameStats.lastTime << ", "
		<< "\"frameOffset\": " << m_frameStats.lastFrameOffset << ", "
		<< "\"gpuTime\": " << m_frameStats.lastGpuTime << ", "
		<< "\"gpuFrameOffset\": " << m_frameStats.lastGpuFrameOffset << "}";
	for (const auto& s : m_stats) {
		m_outputStatsFile
			<< ", \"" << s.first << "\": {"
			<< "\"time\": " << s.second.lastTime << ", "
			<< "\"frameOffset\": " << s.second.lastFrameOffset << ", "
			<< "\"gpuTime\": " << s.second.lastGpuTime << ", "
			<< "\"gpuFrameOffset\": " << s.second.lastGpuFrameOffset << "}";
	}
	m_outputStatsFile << "};";

	m_outputStatsFile
		<< "{\"Frame\": {"
		<< "\"time\": " << m_frameStats.cumulatedTime << ", "
		<< "\"frameOffset\": " << m_frameStats.cumulatedFrameOffset << ", "
		<< "\"gpuTime\": " << m_frameStats.cumulatedGpuTime << ", "
		<< "\"gpuFrameOffset\": " << m_frameStats.cumulatedGpuFrameOffset << "}";
	for (const auto& s : m_stats) {
		m_outputStatsFile
			<< ", \"" << s.first << "\": {"
			<< "\"time\": " << s.second.cumulatedTime << ", "
			<< "\"frameOffset\": " << s.second.cumulatedFrameOffset << ", "
			<< "\"gpuTime\": " << s.second.cumulatedGpuTime << ", "
			<< "\"gpuFrameOffset\": " << s.second.cumulatedGpuFrameOffset << "}";
	}
	m_outputStatsFile << "}\n";

	++m_statFrame;
}
