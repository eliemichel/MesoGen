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
#include "TileSuggestion.h"
#include "TilingSolver.h"
#include "MacrosurfaceData.h"
#include "MesostructureData.h"
#include "Model.h"
#include "Serialization.h"
#include "GlobalTimer.h"

#include <TinyTimer.h>
#include <argparse/argparse.hpp>

#include <vector>
#include <iostream>
#include <cassert>
#include <cmath>

using namespace TileSuggestion;
using TinyTimer::Timer;

#pragma region [Types]

int g_debug = 0;

enum class PerfCounter {
	Solve,
	Suggest,
};

struct SuggestionTestConfig {
	std::string tileset;
	std::string macrosurface;
	int baseSeed = 0;
	int iterations = 10;
	int maxSuggestions = 10; // max number of solving stages (with a suggestion in between)
	int maxAttempts = 10; // max restart per solving stage
	bool verbose = false;
	TilingSolver::SuggestionStrategy strategy = TilingSolver::SuggestionStrategy::Voting;
};

struct SuggestionTestReport {
	std::vector<int> generatedTilesPerIteration;
	TinyTimer::PerformanceCounter perf;
	bool trivial = false;

	int successCount() const {
		int count = 0;
		for (int n : generatedTilesPerIteration) {
			if (n >= 0) ++count;
		}
		return count;
	}

	float meanGeneratedTiles() const {
		float sum = 0.0;
		int count = 0;
		for (int n : generatedTilesPerIteration) {
			if (n < 0) continue;
			sum += n;
			++count;
		}
		return count == 0 ? -1 : sum / count;
	}

	float stddevGeneratedTiles() const {
		float sum_sq = 0.0;
		int count = 0;
		for (int n : generatedTilesPerIteration) {
			if (n < 0) continue;
			sum_sq += n * n;
			++count;
		}
		if (count == 0) return -1;

		float mean = meanGeneratedTiles();
		float variance = sum_sq / count - mean * mean;
		return std::sqrt(std::max(0.0f, variance));
	}
};

struct SuggestionTestExperiment {
	SuggestionTestConfig config;
	SuggestionTestReport report;
};

struct Report {
	std::map<std::string, SuggestionTestExperiment> experiments;
	TinyTimer::PerformanceCounter solvePerf;
};

#pragma endregion [Types]

#pragma region [Serialization]

namespace Serialization {

template <>
void serialize(const TinyTimer::PerformanceCounter& perf, Document& document, Value& json) {
	json.SetObject();
	auto& allocator = document.GetAllocator();

	json.AddMember("average", perf.average() * 1000.0, allocator);
	json.AddMember("stddev", perf.stddev() * 1000.0, allocator);
	json.AddMember("samples", perf.samples(), allocator);
}

template <>
void serialize(const SuggestionTestConfig& config, Document& document, Value& json) {
	json.SetObject();
	auto& allocator = document.GetAllocator();

	json.AddMember("tileset", config.tileset, allocator);
	json.AddMember("macrosurface", config.macrosurface, allocator);
	json.AddMember("baseSeed", config.baseSeed, allocator);
	AddMember("iterations", config.iterations, document, json);
	AddMember("maxSuggestions", config.maxSuggestions, document, json);
	AddMember("maxAttempts", config.maxAttempts, document, json);
	json.AddMember("verbose", config.verbose, allocator);
	json.AddMember("strategy", std::string(magic_enum::enum_name(config.strategy)), allocator);
}

template <>
void serialize(const SuggestionTestReport& report, Document& document, Value& json) {
	json.SetObject();
	auto& allocator = document.GetAllocator();

	AddMember("generatedTilesPerIteration", report.generatedTilesPerIteration, document, json);
	json.AddMember("successCount", report.successCount(), allocator);
	json.AddMember("meanGeneratedTiles", report.meanGeneratedTiles(), allocator);
	json.AddMember("stddevGeneratedTiles", report.stddevGeneratedTiles(), allocator);
	AddMember("perf", report.perf, document, json);
	json.AddMember("trivial", report.trivial, allocator);
}

template <>
void serialize(const SuggestionTestExperiment& exp, Document& document, Value& json) {
	json.SetObject();
	auto& allocator = document.GetAllocator();

	AddMember("config", exp.config, document, json);
	AddMember("report", exp.report, document, json);
}

template <>
void serialize(const Report& report, Document& document, Value& json) {
	json.SetObject();
	auto& allocator = document.GetAllocator();

	Value experimentsJson(rapidjson::Type::kObjectType);
	for (auto pair : report.experiments) {
		AddMember(pair.first, pair.second, document, experimentsJson);
	}
	json.AddMember("experiments", experimentsJson, allocator);
	AddMember("solvePerf", report.solvePerf, document, json);
}

} // namespace Serialization

void testSuggestionScenarioStep(const SuggestionTestConfig& config, SuggestionTestReport& report, int randomSeed) {
	auto macrosurfaceData = std::make_shared<MacrosurfaceData>();
	macrosurfaceData->disableDrawCall();
	macrosurfaceData->properties().slotTopology = MacrosurfaceData::SlotTopology::Mesh;
	macrosurfaceData->properties().modelFilename = config.macrosurface;
	macrosurfaceData->loadMesh();

	auto model = std::make_shared<Model::Tileset>();
	Serialization::deserializeFrom(*model, config.tileset);

	auto mesostructureData = std::make_shared<MesostructureData>();
	mesostructureData->macrosurface = macrosurfaceData;
	mesostructureData->model = model;

	TilingSolver solver;
	solver.properties().randomSeed = randomSeed;
	solver.properties().maxAttempts = config.maxAttempts;
	solver.properties().suggestionStrategy = config.strategy;
	//solver.properties().debugPath = "C:/tmp/tile-suggestion.json";
	solver.setMesostructureData(mesostructureData);

	int suggestionCount = -1;
	for (int j = 0; j < config.maxSuggestions; ++j) {
		solver.properties().debug = g_debug; ++g_debug;
		Timer timer;
		bool success = solver.solve();
		PERF(PerfCounter::Solve).add_sample(timer);
		if (config.verbose) {
			LOG << "Attempt #" << j << ": Solver success = " << success;
		}
		if (success) {
#ifndef NDEBUG
			auto tilingSolverData = solver.tilingSolverData();
			const auto& slots = tilingSolverData->solver->slots();
			for (const auto& ts : slots) {
				assert(ts.tileCount() == 1);
			}
#endif // DEBUG
			suggestionCount = j;
			break;
		}
		else {
			Timer timer;
			solver.computeTileSuggestion();
			report.perf.add_sample(timer);
		}
	}

	report.generatedTilesPerIteration.push_back(suggestionCount);

	if (config.verbose) {
		if (suggestionCount >= 0) {
			LOG << "Success report: success after " << suggestionCount << " suggestions.";
		}
		else {
			LOG << "Success report: failure!";
		}
	}
}

void testSuggestionScenario(const SuggestionTestConfig& config, SuggestionTestReport& report) {
	report.perf.reset();
	report.generatedTilesPerIteration.clear();

	for (int i = 0; i < config.iterations; ++i) {
		testSuggestionScenarioStep(config, report, config.baseSeed + i);
		if (report.generatedTilesPerIteration.back() == 0) {
			// This will always be solvable, no need to run the experiment multiple times
			assert(i == 0);
			report.trivial = true;
			break;
		}
	}
}

#pragma endregion [Serialization]

int main(int argc, char* argv[]) {
	argparse::ArgumentParser program("test-tile-suggestion");

	program.add_argument("-t", "--tileset")
		.default_value(std::string{ DATA_DIR "/tileset.osier2b.json" });
	program.add_argument("-m", "--macrosurface")
		.default_value(std::string{ DATA_DIR "/tshirt.obj" });
	program.add_argument("-r", "--report")
		.default_value(std::string{ DATA_DIR "/results/test-tile-suggestion.json" });
	program.add_argument("-i", "--iterations")
		.default_value(10)
		.scan<'i', int>();
	program.add_argument("-s", "--max-suggestions")
		.default_value(10)
		.scan<'i', int>();
	program.add_argument("-a", "--max-attempts")
		.default_value(10)
		.scan<'i', int>();
	program.add_argument("-x", "--base-seed")
		.default_value(0)
		.scan<'i', int>();

	//program.add_argument("square")
	//	.help("display the square of a given integer")
	//	.scan<'i', int>();

	try {
		program.parse_args(argc, argv);
	}
	catch (const std::runtime_error& err) {
		std::cerr << err.what() << std::endl;
		std::cerr << program;
		std::exit(1);
	}

	SuggestionTestExperiment expPrototype;
	expPrototype.config.tileset = program.get<std::string>("tileset");
	expPrototype.config.macrosurface = program.get<std::string>("macrosurface");
	expPrototype.config.iterations = program.get<int>("iterations");
	expPrototype.config.maxSuggestions = program.get<int>("max-suggestions");
	expPrototype.config.maxAttempts = program.get<int>("max-attempts");
	expPrototype.config.baseSeed = program.get<int>("base-seed");
	expPrototype.config.verbose = false;

	Report report;
	auto report_path = program.get<std::string>("report");
	auto saveReport = [&report, &report_path]() {
		Serialization::serializeTo(report, report_path);
	};

	if (1) {
		SuggestionTestExperiment exp = expPrototype;
		exp.config.strategy = TilingSolver::SuggestionStrategy::Random;
		testSuggestionScenario(exp.config, exp.report);
		report.experiments.insert({ "Random", exp });
		saveReport();
	}
	if (1) {
		SuggestionTestExperiment exp = expPrototype;
		exp.config.strategy = TilingSolver::SuggestionStrategy::GuidedRandom;
		testSuggestionScenario(exp.config, exp.report);
		report.experiments.insert({ "GuidedRandom", exp });
		saveReport();
	}
	if (0) {
		SuggestionTestExperiment exp = expPrototype;
		exp.config.strategy = TilingSolver::SuggestionStrategy::GreedyNaive;
		testSuggestionScenario(exp.config, exp.report);
		report.experiments.insert({ "GreedyNaive", exp });
		saveReport();
	}
	if (0) {
		SuggestionTestExperiment exp = expPrototype;
		exp.config.strategy = TilingSolver::SuggestionStrategy::GreedyStochastic;
		testSuggestionScenario(exp.config, exp.report);
		report.experiments.insert({ "GreedyStochastic", exp });
		saveReport();
	}
	if (1)  {
		SuggestionTestExperiment exp = expPrototype;
		exp.config.strategy = TilingSolver::SuggestionStrategy::Voting;
		testSuggestionScenario(exp.config, exp.report);
		report.experiments.insert({ "Voting", exp });
		saveReport();
	}
	if (0) {
		SuggestionTestExperiment exp = expPrototype;
		exp.config.strategy = TilingSolver::SuggestionStrategy::VotingGreedySanityCheck;
		testSuggestionScenario(exp.config, exp.report);
		report.experiments.insert({ "VotingGreedySanityCheck", exp });
		saveReport();
	}
	if (0) {
		SuggestionTestExperiment exp = expPrototype;
		exp.config.strategy = TilingSolver::SuggestionStrategy::VotingRandomSanityCheck;
		testSuggestionScenario(exp.config, exp.report);
		report.experiments.insert({ "VotingRandomSanityCheck", exp });
		saveReport();
	}
	if (0) {
		SuggestionTestExperiment exp = expPrototype;
		exp.config.iterations = 5;
		exp.config.maxSuggestions = 10;
		exp.config.strategy = TilingSolver::SuggestionStrategy::BruteForce;
		testSuggestionScenario(exp.config, exp.report);
		report.experiments.insert({ "BruteForce", exp });
		saveReport();
	}

	std::ostringstream ss;
	ss << " - Solve: " << PERF(PerfCounter::Solve).summary() << "\n";
	//ss << " - Suggest: " << PERF(PerfCounter::Suggest).summary() << "\n";
	LOG << "Performance report:\n" << ss.str();
	report.solvePerf = PERF(PerfCounter::Solve);

	saveReport();
	return 0;
}
