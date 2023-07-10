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

#include "TilingSolverData.h"

#include <NonCopyable.h>

#include <refl.hpp>

#include <string>
#include <vector>
#include <unordered_set>

class MacrosurfaceData;
class MesostructureData;
class TilesetController;
namespace TileSuggestion {
	struct Tile;
	struct Transform;
}

class TilingSolver : public NonCopyable {
public:
	enum class Algorithm {
		WaveFunctionCollapse,
#ifdef USE_MODEL_SYNTHESIS
		ModelSynthesis,
#endif
	};
	enum class SuggestionStrategy {
		Random,
		GuidedRandom, // Random accross the impossible neighborhoods
		GreedyNaive, // totally greedy, take the first label of each direction for the first impossible situation
		GreedyStochastic, // draw randomly from the first impossible situation
		Voting, // ours
		VotingGreedySanityCheck, // should be roughly equivalent to GreedyNaive
		VotingRandomSanityCheck, // should be better than pure Random
		BruteForce,
	};

public:
	TilingSolver();

	/**
	 * Action triggered from the UI
	 * (basically a stateful wrapper around solve())
	 */
	void generate();

	/**
	 * generate() uses this as input
	 */
	void setMesostructureData(std::weak_ptr<MesostructureData> mesostructureData) { m_mesostructureData = mesostructureData; }

	// hacky, only to call setDirty
	void setController(std::weak_ptr<TilesetController> controller) { m_controller = controller; }

	/**
	 * Orchestrate other solving methods depending on properties, and do some additional checks.
	 * @param mesostructureData must point to a valid model and a macrosurface.
	 */
	bool solve(); // use m_mesostructure
	bool solve(MesostructureData& mesostructureData);

	void clear();

	// Substeps
	void reset();
	void restart();
	void step();

	/**
	 * Inspect the solver's stats to see what new tile could be introduced to the dictionary.
	 */
	void computeTileSuggestion();

	/**
	 * Use carefully, prefer solve()
	 */
#ifdef USE_MODEL_SYNTHESIS
	bool solveGridMs(MesostructureData& mesostructureData);
#endif
	bool solveMeshWfc(MesostructureData& mesostructureData);

	const std::string& message() const { return m_message; }
	std::shared_ptr<TilingSolverData> tilingSolverData() { return m_tilingSolverData; }

private:
	void updateSlotAssignments();
	bool checkIntegrity();

	/**
	 * Variants of computeTileSuggestion
	 */
	bool computeTileSuggestion_Random(TileSuggestion::Tile& newTile);
	bool computeTileSuggestion_Guided(TileSuggestion::Tile& newTile, SuggestionStrategy strategy);
	bool computeTileSuggestion_Voting(TileSuggestion::Tile& newTile, SuggestionStrategy strategy);
	bool computeTileSuggestion_BruteForce(TileSuggestion::Tile& newTile);

	/**
	 * Convert neighbor into the list of possible wang labels facing towards the
	 * impossible slot (used by the tile suggestion mechanism).
	 */
	static std::unordered_set<int> GetWangLabelSuperposition(
		const std::optional<std::pair<TilingSolverData::TileSuperposition, libwfc::MeshRelation>>& neighbor,
		const libwfc::ndvector<int, 2>& table,
		std::unordered_set<int> defaultValue,
		std::unordered_set<int> borderOnlyValue
	);

	static std::vector<TileSuggestion::Transform> ComputeAllowedTransforms(
		const Model::TileTransformPermission& transformPerm
	);

public:
	struct Properties {
		Algorithm algorithm = Algorithm::WaveFunctionCollapse;
		SuggestionStrategy suggestionStrategy = SuggestionStrategy::Voting;
		int maxAttempts = 20;
		int randomSeed = 0;
		bool resetBefore = true; // reset before running the solver
		int debug = 0;
		std::string debugPath = "";
	};
	Properties& properties() { return m_properties; }
	const Properties& properties() const { return m_properties; }

private:
	Properties m_properties;
	std::string m_message;
	std::weak_ptr<TilesetController> m_controller;
	std::weak_ptr<MesostructureData> m_mesostructureData;
	std::shared_ptr<TilingSolverData> m_tilingSolverData;
};

#define _ ReflectionAttributes::
REFL_TYPE(TilingSolver::Properties)
//REFL_FIELD(algorithm)
REFL_FIELD(suggestionStrategy)
REFL_FIELD(maxAttempts, _ Range(1, 100))
REFL_FIELD(randomSeed, _ Range(0, 65536))
//REFL_FIELD(resetBefore)
REFL_END
#undef _
