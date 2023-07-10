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
#include "TilingSolver.h"
#include "MesostructureData.h"
#include "MacrosurfaceData.h"
#include "TilesetController.h"
#include "TilingSolverData.h"
#include "TileVariantList.h"
#include "TileSuggestion.h"

#include "utils/miscutils.h"

#include <GlobalTimer.h>
#include <libwfc.h>
#include <Logger.h>

#include <optional>
#include <chrono>
#include <random>

using TileSuperposition = TilingSolverData::TileSuperposition;
using Ruleset = TilingSolverData::Ruleset;
using Solver = TilingSolverData::Solver;

#pragma region [Debug]
#include "Serialization.h"
struct TileSuggestionDebugInfo {
	std::vector<std::array<std::unordered_set<int>, 4>> impossibleNeighborhoodLabels;
	TileSuggestion::VoteTable votes;
	TileSuggestion::Tile tile;
	TileSuggestion::Tile alternativeTile;
};

namespace Serialization {
	template <>
	void serialize(const TileSuggestionDebugInfo& debug, Document& document, Value& json) {
		json.SetObject();
		AddMember("impossibleNeighborhoodLabels", debug.impossibleNeighborhoodLabels, document, json);
		AddMember("votes", debug.votes, document, json);
		AddMember("tile", debug.tile.labels, document, json);
		AddMember("alternativeTile", debug.alternativeTile.labels, document, json);
	}

	template <>
	void serialize(const std::unordered_set<int,std::hash<int>,std::equal_to<int>,std::allocator<int>>& set, Document& document, Value& json) {
		json.SetArray();
		auto& allocator = document.GetAllocator();
		for (int x : set) {
			Value v;
			v.SetInt(x);
			json.PushBack(v, allocator);
		}
	}

	template <>
	void serialize(const std::array<std::unordered_set<int, std::hash<int>, std::equal_to<int>, std::allocator<int>>,4>& arr, Document& document, Value& json) {
		json.SetArray();
		auto& allocator = document.GetAllocator();
		auto it = arr.cbegin();
		auto end = arr.cend();
		for (; it != end; ++it) {
			const std::unordered_set<int>& item = *it;
			Value v;
			serialize(item, document, v);
			json.PushBack(v, allocator);
		}
	}

	template <>
	void serialize(const std::vector<std::array<std::unordered_set<int, std::hash<int>, std::equal_to<int>, std::allocator<int>>, 4>>& vec, Document& document, Value& json) {
		json.SetArray();
		auto& allocator = document.GetAllocator();
		auto it = vec.cbegin();
		auto end = vec.cend();
		for (; it != end; ++it) {
			const std::array<std::unordered_set<int, std::hash<int>, std::equal_to<int>, std::allocator<int>>, 4>& item = *it;
			Value v;
			serialize(item, document, v);
			json.PushBack(v, allocator);
		}
	}

	template <>
	void serialize(const TileSuggestion::VoteTable& votes, Document& document, Value& json) {
		json.SetArray();
		auto& allocator = document.GetAllocator();
		for (const auto& [tile, counts] : votes) {
			Value v;
			v.SetObject();
			AddMember("tile", tile.labels, document, v);
			AddMember("counts", counts, document, v);
			json.PushBack(v, allocator);
		}
	}

	template <>
	void serialize(const std::unordered_map<TileSuggestion::TransformRequirement, int, TileSuggestion::TransformRequirementHasher>& counts, Document& document, Value& json) {
		json.SetArray();
		auto& allocator = document.GetAllocator();
		for (const auto& [transform, n] : counts) {
			Value v;
			v.SetObject();
			AddMember("transform", transform, document, v);
			v.AddMember("count", n, allocator);
			json.PushBack(v, allocator);
		}
	}

	template <>
	void serialize(const TileSuggestion::TransformRequirement& transform, Document& document, Value& json) {
		json.SetObject();
		auto& allocator = document.GetAllocator();
		json.AddMember("flipX", transform.flipX, allocator);
		json.AddMember("flipY", transform.flipY, allocator);
		json.AddMember("rotation", transform.rotation, allocator);
	}
}
#pragma endregion [Debug]

TilingSolver::TilingSolver()
	: m_tilingSolverData(std::make_shared<TilingSolverData>())
{}

void TilingSolver::generate() {
	if (auto mesostructureData = m_mesostructureData.lock()) {
		solve(*mesostructureData);
	}
	else {
		WARN_LOG << "Could not generate: mesostructure not set!";
	}
}

bool TilingSolver::solve() {
	if (auto mesostructure = m_mesostructureData.lock()) {
		return solve(*mesostructure);
	}
	else {
		return false;
	}
}

bool TilingSolver::solve(MesostructureData& mesostructureData) {
	if (!mesostructureData.model) {
		WARN_LOG << "Could not solve: mesostructure has no associated model!";
		return false;
	}

	auto macrosurfacePtr = mesostructureData.macrosurface.lock();
	if (!macrosurfacePtr) {
		WARN_LOG << "Could not solve: mesostructure has no associated macrosurface!";
		return false;
	}

	const MacrosurfaceData& macrosurfaceData = *macrosurfacePtr;
	if (!macrosurfaceData.ready()) {
		WARN_LOG << "Could not solve: macrosurface not ready!";
		return false;
	}

	const auto& slotTopologyType = macrosurfaceData.properties().slotTopology;
	const auto& props = properties();
	switch (props.algorithm) {
	case Algorithm::ModelSynthesis:
		switch (slotTopologyType) {
		case MacrosurfaceData::SlotTopology::Grid:
			return solveGridMs(mesostructureData);
		}
	case Algorithm::WaveFunctionCollapse:
		switch (slotTopologyType) {
		case MacrosurfaceData::SlotTopology::Grid:
		case MacrosurfaceData::SlotTopology::Mesh:
			// Both use the same solver actually
			return solveMeshWfc(mesostructureData);
		}
	}
	ERR_LOG
		<< "Generation algorithm not imlemented: "
		<< "algorithm '" << magic_enum::enum_name(props.algorithm) << "' "
		<< "is not compatible with slot topology '" << magic_enum::enum_name(slotTopologyType) << "'";
	return false;
}

void TilingSolver::clear() {
	if (auto mesostructureData = m_mesostructureData.lock()) {
		mesostructureData->slotAssignments.clear();
		mesostructureData->slotAssignmentsVersion.increment();
	}
}

void TilingSolver::reset() {
	if (!m_tilingSolverData->solver) return;

	m_tilingSolverData->solver->reset();

	updateSlotAssignments();
}

void TilingSolver::restart() {
	if (!m_tilingSolverData->solver) return;

	m_tilingSolverData->solver->restart();

	updateSlotAssignments();
}

void TilingSolver::step() {
	if (!m_tilingSolverData->solver) return;

	Solver::Status status = m_tilingSolverData->solver->step();
	switch (status) {
	case Solver::Status::Failed:
		m_message = "Failed";
		break;
	case Solver::Status::Continue:
		m_message = "Continue";
		break;
	case Solver::Status::Finished:
		m_message = "Finished";
		break;
	}

	checkIntegrity();
	updateSlotAssignments();
}

void TilingSolver::computeTileSuggestion() {
	if (!m_tilingSolverData->solver) return;
	auto mesostructure = m_mesostructureData.lock(); if (!mesostructure) return;

	auto timer = GlobalTimer::Start("TilingSolver::computeTileSuggestion");

	TileSuggestion::Tile newTile;
	bool success = false;

	switch (properties().suggestionStrategy) {
	case SuggestionStrategy::Random:
		success = computeTileSuggestion_Random(newTile);
		break;
	case SuggestionStrategy::GuidedRandom:
		success = computeTileSuggestion_Guided(newTile, properties().suggestionStrategy);
		break;
	case SuggestionStrategy::GreedyNaive:
		success = computeTileSuggestion_Guided(newTile, properties().suggestionStrategy);
		break;
	case SuggestionStrategy::GreedyStochastic:
		success = computeTileSuggestion_Guided(newTile, properties().suggestionStrategy);
		break;
	default:
	case SuggestionStrategy::Voting:
		success = computeTileSuggestion_Voting(newTile, properties().suggestionStrategy);
		break;
	case SuggestionStrategy::VotingGreedySanityCheck:
		success = computeTileSuggestion_Voting(newTile, properties().suggestionStrategy);
		break;
	case SuggestionStrategy::VotingRandomSanityCheck:
		success = computeTileSuggestion_Voting(newTile, properties().suggestionStrategy);
		break;
	case SuggestionStrategy::BruteForce:
		success = computeTileSuggestion_BruteForce(newTile);
		break;
	}

	if (!success) {
		return;
	}

	// Add new tile type
	auto tileType = std::make_shared<Model::TileType>();
	tileType->transformPermission = mesostructure->model->defaultTileTransformPermission;
	int newEdgeLabel = -1;
	for (int k = 0; k < Model::TileEdge::SolveCount; ++k) {
		int edgeLabel = abs(newTile.labels[k]);
		// Create new edge if really needed
		if (edgeLabel == 0) {
			if (newEdgeLabel == -1) {
				if (auto controller = m_controller.lock()) {
					controller->addEdgeType();
				}
				else {
					mesostructure->model->edgeTypes.push_back(std::make_shared<Model::EdgeType>());
				}
				newEdgeLabel = static_cast<int>(mesostructure->model->edgeTypes.size());
			}
			edgeLabel = newEdgeLabel;
		}
		tileType->edges[k].type = mesostructure->model->edgeTypes[edgeLabel - 1];
		tileType->edges[k].transform.flipped = newTile.labels[k] < 0;
	}
	mesostructure->model->tileTypes.push_back(tileType);
	mesostructure->tileListVersion.increment();
	mesostructure->tileVersions.push_back(VersionCounter());
	mesostructure->slotAssignmentsVersion.increment();
	if (auto controller = m_controller.lock()) {
		controller->setDirty();
	}

	GlobalTimer::Stop(timer);
}

bool TilingSolver::computeTileSuggestion_Random(TileSuggestion::Tile& newTile) {
	auto mesostructure = m_mesostructureData.lock();
	int edgeTypeCount = static_cast<int>(mesostructure->model->edgeTypes.size());
	Solver& solver = *m_tilingSolverData->solver;

	for (int rel = 0; rel < 4; ++rel) {
		int sign = solver.randInt(2) == 0 ? -1 : 1;
		newTile.labels[rel] = sign * (1 + solver.randInt(edgeTypeCount));
	}

	return true;
}

bool TilingSolver::computeTileSuggestion_Guided(TileSuggestion::Tile& newTile, SuggestionStrategy strategy) {
	auto mesostructure = m_mesostructureData.lock();

	Solver& solver = *m_tilingSolverData->solver;
	const auto& impossibleNeighborhoods = solver.stats().impossibleNeighborhoods;
	const auto& table = m_tilingSolverData->adapter.table;
	int edgeTypeCount = static_cast<int>(mesostructure->model->edgeTypes.size());

	if (impossibleNeighborhoods.empty()) {
		WARN_LOG << "The generation must have failed for the tile suggestion mechanism to work";
		return false;
	}

	std::unordered_set<int> borderLabels; // all borders that are allowed on borders
	std::unordered_set<int> borderOnlyLabels; // all borders that are only allowed on borders
	borderLabels.reserve(2 * edgeTypeCount);
	for (int i = 0; i < edgeTypeCount; ++i) {
		if (mesostructure->model->edgeTypes[i]->borderEdge) {
			borderLabels.insert(i + 1);
			borderLabels.insert(-(i + 1));
			if (mesostructure->model->edgeTypes[i]->borderOnly) {
				borderOnlyLabels.insert(i + 1);
				borderOnlyLabels.insert(-(i + 1));
			}
		}
	}
	
	auto neighborhoodsIt = impossibleNeighborhoods.cbegin();
	if (strategy == SuggestionStrategy::GuidedRandom) {
		std::advance(neighborhoodsIt, solver.randInt((int)impossibleNeighborhoods.size()));
	}
	const auto& neighborhood = *neighborhoodsIt;
	std::array<std::unordered_set<int>, 4> neighborhoodLabels;
	for (int rel = 0; rel < 4; ++rel) {
		TileSuggestion::Set possibleLabels = GetWangLabelSuperposition(neighborhood[rel], table, borderLabels, borderOnlyLabels);
		if (possibleLabels.empty()) {
			WARN_LOG << "Empty possible label set!";
			newTile.labels[rel] = 0;
			continue;
		}
		else {
			auto it = possibleLabels.cbegin();
			if (strategy == SuggestionStrategy::GreedyStochastic || strategy == SuggestionStrategy::GuidedRandom) {
				std::advance(it, solver.randInt((int)possibleLabels.size()));
			}
			// The '-' is important as *it is the neighbor label and only labels of opposite signs are compatible
			newTile.labels[rel] = -*it;
		}
	}

	return true;
}

bool TilingSolver::computeTileSuggestion_Voting(TileSuggestion::Tile& newTile, SuggestionStrategy strategy) {
	auto mesostructure = m_mesostructureData.lock();

	Solver& solver = *m_tilingSolverData->solver;
	const auto& impossibleNeighborhoods = solver.stats().impossibleNeighborhoods;
	const auto& table = m_tilingSolverData->adapter.table;
	int variantCount = table.shape(0);// m_tilingSolverData->adapter.tileVariantList->variantCount(); // might be null for some reason...
	int edgeTypeCount = static_cast<int>(mesostructure->model->edgeTypes.size());

	if (impossibleNeighborhoods.empty()) {
		WARN_LOG << "The generation must have failed for the tile suggestion mechanism to work";
		return false;
	}

	std::unordered_set<int> borderLabels; // all borders that are allowed on borders
	std::unordered_set<int> borderOnlyLabels; // all borders that are only allowed on borders
	borderLabels.reserve(2 * edgeTypeCount);
	for (int i = 0; i < edgeTypeCount; ++i) {
		if (mesostructure->model->edgeTypes[i]->borderEdge) {
			borderLabels.insert(i + 1);
			borderLabels.insert(-(i + 1));
			if (mesostructure->model->edgeTypes[i]->borderOnly) {
				borderOnlyLabels.insert(i + 1);
				borderOnlyLabels.insert(-(i + 1));
			}
		}
	}

	std::vector<std::array<std::unordered_set<int>, 4>> impossibleNeighborhoodLabels;
	impossibleNeighborhoodLabels.reserve(impossibleNeighborhoods.size());

	// Skip some neighborhood when there are too many
	bool skip = impossibleNeighborhoods.size() > 20;
	int period = static_cast<int>(impossibleNeighborhoods.size() / 20);
	int nid = 0;

	for (const auto& neighborhood : impossibleNeighborhoods) {
		++nid;
		if (skip && nid % period != 0) continue;

		std::array<std::unordered_set<int>, 4> neighborhoodLabels;
		for (int rel = 0; rel < 4; ++rel) {
			neighborhoodLabels[rel] = GetWangLabelSuperposition(neighborhood[rel], table, borderLabels, borderOnlyLabels);
			assert(!neighborhoodLabels[rel].empty());
		}
		impossibleNeighborhoodLabels.push_back(neighborhoodLabels);
	}

	// Gather existing tiles to avoid suggesting one again
	// elie: how is it possible to have an existing tile among the neighborhoods
	// deemed as "impossible" to fulfill with the tileset?
	std::unordered_set<TileSuggestion::Tile, TileSuggestion::TileHasher> existingTiles;
	auto allTransforms = TileSuggestion::Transform::ComputeAllTransforms();
	for (int variant = 0; variant < variantCount; ++variant) {
		TileSuggestion::Tile tile{
			table.get_at(variant, 0),
			table.get_at(variant, 1),
			table.get_at(variant, 2),
			table.get_at(variant, 3)
		};
		// Variant already correspond to transformed tiles, no need to transform again
		/*
		for (const auto& transform : allTransforms) {
			TileSuggestion::Tile transformedTile = transform.apply(tile);
			existingTiles.insert(transformedTile);
		}*/
		existingTiles.insert(tile);
	}

	TileSuggestion::VoteTable votes;
	auto subtimer = GlobalTimer::Start("consolidateVotes");

	consolidateVotes(impossibleNeighborhoodLabels, votes, ComputeAllowedTransforms(mesostructure->model->defaultTileTransformPermission));
	GlobalTimer::Stop(subtimer);

	// Sanity checks
	switch (strategy) {
	case SuggestionStrategy::VotingGreedySanityCheck:
		newTile = votes.begin()->first;
		return true;
	case SuggestionStrategy::VotingRandomSanityCheck: {
		auto it = votes.cbegin();
		std::advance(it, solver.randInt((int)votes.size()));
		newTile = it->first;
		return true;
	}
	default:
		break;
	}

	subtimer = GlobalTimer::Start("findBest");
	auto result = findBest(votes, existingTiles);
	GlobalTimer::Stop(subtimer);

	TileSuggestion::Tile tile = result.first;
	TileSuggestion::Tile alternativeTile = result.second;

	if (0) {
		std::ostringstream ss;
		ss << "================\nOverall suggestion:\n";
		for (int rel = 0; rel < 4; ++rel) {
			ss << "In direction " << magic_enum::enum_name(static_cast<Model::TileEdge::Direction>(rel)) << ": ";
			ss << tile.labels[rel] << "\n";
		}
		LOG << ss.str();
	}

	if (0) {
		std::ostringstream ss;
		ss << "================\nAlternative suggestion:\n";
		for (int rel = 0; rel < 4; ++rel) {
			ss << "In direction " << magic_enum::enum_name(static_cast<Model::TileEdge::Direction>(rel)) << ": ";
			ss << alternativeTile.labels[rel] << "\n";
		}
		LOG << ss.str();
	}


	if (!properties().debugPath.empty()) {
		Serialization::serializeTo(TileSuggestionDebugInfo{ impossibleNeighborhoodLabels, votes, tile, alternativeTile }, properties().debugPath);
	}


	newTile = tile;
	return true;
}

bool TilingSolver::computeTileSuggestion_BruteForce(TileSuggestion::Tile& newTile) {
	auto mesostructure = m_mesostructureData.lock();

	Solver& solver = *m_tilingSolverData->solver;
	const auto& impossibleNeighborhoods = solver.stats().impossibleNeighborhoods;
	const auto& table = m_tilingSolverData->adapter.table;
	int variantCount = table.shape(0);// m_tilingSolverData->adapter.tileVariantList->variantCount(); // might be null for some reason...
	int edgeTypeCount = static_cast<int>(mesostructure->model->edgeTypes.size());

	if (impossibleNeighborhoods.empty()) {
		WARN_LOG << "The generation must have failed for the tile suggestion mechanism to work";
		return false;
	}

	std::unordered_set<int> borderLabels; // all borders that are allowed on borders
	std::unordered_set<int> borderOnlyLabels; // all borders that are only allowed on borders
	borderLabels.reserve(2 * edgeTypeCount);
	for (int i = 0; i < edgeTypeCount; ++i) {
		if (mesostructure->model->edgeTypes[i]->borderEdge) {
			borderLabels.insert(i + 1);
			borderLabels.insert(-(i + 1));
			if (mesostructure->model->edgeTypes[i]->borderOnly) {
				borderOnlyLabels.insert(i + 1);
				borderOnlyLabels.insert(-(i + 1));
			}
		}
	}

	std::vector<std::array<std::unordered_set<int>, 4>> impossibleNeighborhoodLabels;
	impossibleNeighborhoodLabels.reserve(impossibleNeighborhoods.size());

	// Skip some neighborhood when there are too many
	bool skip = impossibleNeighborhoods.size() > 20;
	int period = static_cast<int>(impossibleNeighborhoods.size() / 20);
	int nid = 0;

	for (const auto& neighborhood : impossibleNeighborhoods) {
		++nid;
		if (skip && nid % period != 0) continue;

		std::array<std::unordered_set<int>, 4> neighborhoodLabels;
		for (int rel = 0; rel < 4; ++rel) {
			neighborhoodLabels[rel] = GetWangLabelSuperposition(neighborhood[rel], table, borderLabels, borderOnlyLabels);
		}
		impossibleNeighborhoodLabels.push_back(neighborhoodLabels);
	}

	// Gather existing tiles to avoid suggesting one again
	// elie: how is it possible to have an existing tile among the neighborhoods
	// deemed as "impossible" to fulfill with the tileset?
	std::unordered_set<TileSuggestion::Tile, TileSuggestion::TileHasher> existingTiles;
	auto allTransforms = TileSuggestion::Transform::ComputeAllTransforms();
	for (int variant = 0; variant < variantCount; ++variant) {
		TileSuggestion::Tile tile{
			table.get_at(variant, 0),
			table.get_at(variant, 1),
			table.get_at(variant, 2),
			table.get_at(variant, 3)
		};
		for (const auto& transform : allTransforms) {
			TileSuggestion::Tile transformedTile = transform.apply(tile);
			existingTiles.insert(transformedTile);
		}
	}

	TileSuggestion::VoteTable votes;
	auto subtimer = GlobalTimer::Start("consolidateVotes");
	consolidateVotes(impossibleNeighborhoodLabels, votes, ComputeAllowedTransforms(mesostructure->model->defaultTileTransformPermission));
	GlobalTimer::Stop(subtimer);

	// We try each solution from impossibleNeighborhoodLabels and stop when
	// solving is possible.
	for (auto const& [tile, tr] : votes) {
		(void)tr;
		// Very hacky but just for testing: we temporarily add the new tile
		auto tileType = std::make_shared<Model::TileType>();
		int newEdgeLabel = -1;
		for (int k = 0; k < Model::TileEdge::SolveCount; ++k) {
			int edgeLabel = abs(tile.labels[k]);
			// Create new edge if really needed
			if (edgeLabel == 0) {
				if (newEdgeLabel == -1) {
					if (auto controller = m_controller.lock()) {
						controller->addEdgeType();
					}
					else {
						mesostructure->model->edgeTypes.push_back(std::make_shared<Model::EdgeType>());
					}
					newEdgeLabel = static_cast<int>(mesostructure->model->edgeTypes.size());
				}
				edgeLabel = newEdgeLabel;
			}
			tileType->edges[k].type = mesostructure->model->edgeTypes[edgeLabel - 1];
			tileType->edges[k].transform.flipped = tile.labels[k] < 0;
		}
		mesostructure->model->tileTypes.push_back(tileType);
		mesostructure->tileListVersion.increment();
		mesostructure->tileVersions.push_back(VersionCounter());
		mesostructure->slotAssignmentsVersion.increment();
		if (auto controller = m_controller.lock()) {
			controller->setDirty();
		}

		// Try solving
		bool success = solve();
		
		// Rollback model change
		mesostructure->model->tileTypes.pop_back();
		mesostructure->tileListVersion.increment();
		mesostructure->tileVersions.pop_back();
		mesostructure->slotAssignmentsVersion.increment();
		if (auto controller = m_controller.lock()) {
			controller->setDirty();
		}

		if (success) {
			newTile = tile;
			return true;
		}
	}

	// fallback
	return computeTileSuggestion_Voting(newTile, SuggestionStrategy::Voting);
}

bool TilingSolver::solveMeshWfc(MesostructureData& mesostructureData) {
	auto timer = GlobalTimer::Start("TilingSolver::solveMeshWfc");
	LOG << "Solving tiling using Wave Function Collapse...";
	std::chrono::microseconds synthesisTime{ 0 };

	const MacrosurfaceData& macrosurfaceData = *mesostructureData.macrosurface.lock();

	auto adapter = TilesetController::ConvertModelToSignedWangRuleset(*mesostructureData.model);
	mesostructureData.tileVariantList = std::move(adapter.tileVariantList);

	int tileTypeCount = mesostructureData.tileVariantList->variantCount();

	auto& slotTopology = *macrosurfaceData.slotTopology();

	m_tilingSolverData->adapter = adapter;
	m_tilingSolverData->ruleset = std::make_unique<Ruleset>(m_tilingSolverData->adapter.table, tileTypeCount);
	Ruleset& ruleset = *m_tilingSolverData->ruleset;
	m_tilingSolverData->solver = std::make_unique<Solver>(slotTopology, ruleset, TileSuperposition(tileTypeCount), adapter.borderLabels, adapter.borderOnlyLabels);
	Solver& solver = *m_tilingSolverData->solver;
	solver.options().randomSeed = properties().randomSeed;
	solver.options().maxSteps = 100000;
	solver.options().maxAttempts = properties().maxAttempts;
	solver.options().debug = properties().debug;

	auto subtimer = GlobalTimer::Start("solve");
	bool success = solver.solve(properties().resetBefore);
	GlobalTimer::Stop(subtimer);

	if (success) {
		m_message = "Generation succeeded";
	}
	else {
		m_message = "Generation failed";
	}

	//LOG << "Generation took " << (synthesisTime.count() / 1000.0) << " ms";

	// save output in this class's attribute before synthesizer frees it
	
	updateSlotAssignments();

	GlobalTimer::Stop(timer);
	return success;
}

void TilingSolver::updateSlotAssignments() {
	if (!m_tilingSolverData->solver) return;

	m_tilingSolverData->version.increment();

	if (auto mesostructureData = m_mesostructureData.lock()) {
		size_t slotCount = m_tilingSolverData->solver->slots().size();

		mesostructureData->slotAssignments.resize(slotCount);
		for (int i = 0; i < slotCount; ++i) {
			const auto& superposition = m_tilingSolverData->solver->slots()[i];
			mesostructureData->slotAssignments[i] = superposition.isEmpty() ? -1 : *superposition.begin();
		}
		mesostructureData->slotAssignmentsVersion.increment();
	}
}

bool TilingSolver::checkIntegrity() {
	if (!m_tilingSolverData->solver) return false;
	auto mesostructureData = m_mesostructureData.lock(); if (!mesostructureData) return false;
	auto macrosurfaceData = mesostructureData->macrosurface.lock(); if (!macrosurfaceData) return false;

	const auto& slots = m_tilingSolverData->solver->slots();
	auto& slotTopology = *macrosurfaceData->slotTopology();
	auto& ruleset = *m_tilingSolverData->ruleset;

	assert(slots.size() == slotTopology.slotCount());

	for (int i = 0; i < slots.size(); ++i) {
		for (int relXIndex = 0; relXIndex < 4; ++relXIndex) {
			auto maybeNeighbor = slotTopology.neighborOf(i, static_cast<libwfc::MeshSlotTopology::RelationEnum>(relXIndex));
			if (!maybeNeighbor.has_value()) continue;
			int j = maybeNeighbor->first;
			auto relX = static_cast<libwfc::MeshRelation>(relXIndex);
			auto relY = static_cast<libwfc::MeshRelation>(maybeNeighbor->second);
			for (auto x : slots[i]) {
				bool allowed = false;
				for (auto y : slots[j]) {
					if (ruleset.allows(x, relX, y, relY)) {
						allowed = true;
					}
				}
				if (!allowed) {
					ERR_LOG
						<< "Inconsistency detected: In slot #" << i << ": state " << mesostructureData->tileVariantList->variantRepr(x)
						<< " is incompatible with all neighbor states in slot #" << j
						<< " (in direction " << magic_enum::enum_name(relX) << " -> " << magic_enum::enum_name(relY) << ")";
					return false;
				}
			}
		}
	}

	return true;
}

std::unordered_set<int> TilingSolver::GetWangLabelSuperposition(
	const std::optional<std::pair<TileSuperposition, libwfc::MeshRelation>>& neighbor,
	const libwfc::ndvector<int, 2>& table,
	std::unordered_set<int> defaultValue,
	std::unordered_set<int> borderOnlyValue
) {
	if (!neighbor.has_value()) {
		return defaultValue;
	}
	std::unordered_set<int> possibleLabels;
	for (const auto& variant : neighbor->first) {
		int label = table.get_at(variant, static_cast<int>(neighbor->second));
		if (!borderOnlyValue.contains(label)) {
			possibleLabels.insert(label);
		}
	}
	return possibleLabels;
}

std::vector<TileSuggestion::Transform> TilingSolver::ComputeAllowedTransforms(
	const Model::TileTransformPermission& transformPerm
) {
	std::vector<TileSuggestion::Transform> transforms = { {} };
	if (transformPerm.flipX) {
		std::vector<TileSuggestion::Transform> prevTransforms(transforms);
		transforms.resize(0);
		for (auto tr : prevTransforms) {
			transforms.push_back(tr);
			tr.flipX = !tr.flipX;
			transforms.push_back(tr);
		}
	}
	if (transformPerm.flipY) {
		std::vector<TileSuggestion::Transform> prevTransforms(transforms);
		transforms.resize(0);
		for (auto tr : prevTransforms) {
			transforms.push_back(tr);
			tr.flipY = !tr.flipY;
			transforms.push_back(tr);
		}
	}
	if (transformPerm.rotation) {
		std::vector<TileSuggestion::Transform> prevTransforms(transforms);
		transforms.resize(0);
		for (auto tr : prevTransforms) {
			TileSuggestion::Orientation baseOrientation = tr.orientation;
			for (int i = 0; i < magic_enum::enum_count<TileSuggestion::Orientation>(); ++i) {
				tr.orientation = static_cast<TileSuggestion::Orientation>(static_cast<int>(baseOrientation) + i);
				transforms.push_back(tr);
			}
		}
	}
	return transforms;
}
