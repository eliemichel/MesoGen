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

#include "VersionCounter.h"
#include "TilesetController.h"

#include <NonCopyable.h>
#include <libwfc.h>

/**
 * Data used only for the duration of solving but that we keep aroudn for debugging/displaying purpose.
 */
class TilingSolverData : public NonCopyable {
public:
	using TileSuperposition = libwfc::FastTileSuperposition<>;
	using Ruleset = libwfc::FastSignedWangRuleset<libwfc::MeshRelation>;
	//using AbstractSlotTopology = libwfc::AbstractSlotTopology<int, libwfc::MeshRelation>;
	using SlotTopology = libwfc::MeshSlotTopology;
	
	// Not sure this belongs here, but TilingSolverData is the source of truth for
	// TileSuperposition, Ruleset and Solver types.
	using BaseSolver = libwfc::Solver<TileSuperposition, int, libwfc::MeshRelation>;
	class Solver : public BaseSolver {
	public:
		Solver(
			const SlotTopology& topology,
			const Ruleset& ruleset,
			TileSuperposition tileSuperpositionExample,
			const std::unordered_set<int>& borderLabels,
			const std::unordered_set<int>& borderOnlyLabels
		);
	protected:
		bool applyInitialContraints() override;
		bool checkImpossibleNeighborhood(const std::array<std::optional<std::pair<TileSuperposition, libwfc::MeshRelation>>, 4>& neighborhood) const override;

		// Convert a tile-based neighborhood into a label-based neighborhood
		std::array<std::unordered_set<int>, 4> getNeighborhoodLabels(const std::array<std::optional<std::pair<TileSuperposition, libwfc::MeshRelation>>, 4>& neighborhood) const;

		// Check that a tile fits in its neighborhood
		bool fitsInNeighborhood(int tileVariantIndex, std::array<std::unordered_set<int>, 4> neighborhoodLabels) const;

		// When allowIncompatible, we allow tiles that are not compatible with neighbors
		// but still require that when a tile is not present, it is because none of the
		// neighbors is compatible with it.
		bool checkIntegrity(bool allowIncompatible = false) const override;
	private:
		const SlotTopology& m_topology;
		const Ruleset& m_ruleset;
		std::unordered_set<int> m_borderLabels;
		std::unordered_set<int> m_borderOnlyLabels;
	};

public:
	VersionCounter version;
	TilesetController::SignedWangRulesetAdapter adapter;
	std::unique_ptr<Ruleset> ruleset;
	std::unique_ptr<Solver> solver;
};
