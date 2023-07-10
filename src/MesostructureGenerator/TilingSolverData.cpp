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
#include "TilingSolverData.h"

TilingSolverData::Solver::Solver(
	const SlotTopology& topology,
	const Ruleset& ruleset,
	TileSuperposition tileSuperpositionExample,
	const std::unordered_set<int>& borderLabels,
	const std::unordered_set<int>& borderOnlyLabels
)
	: BaseSolver(topology, ruleset, tileSuperpositionExample)
	, m_topology(topology)
	, m_ruleset(ruleset)
	, m_borderLabels(borderLabels)
	, m_borderOnlyLabels(borderOnlyLabels)
{}

bool TilingSolverData::Solver::applyInitialContraints() {
	if (slots().empty()) return true;
	TileSuperposition noTiles = slots()[0];
	noTiles.setToNone();

	// Once the constraint resolution failed (i.e., we know there is no solution),
	// we still continue in order to collect samples for the tile suggestion mechanism
	int failureCount = 0;

	// For all slots that are on a border, we only allow edges flagged as border edges
	auto slotIt = slots().begin();
	auto faceIt = m_topology.faces().begin();
	for (int i = 0; i < slots().size(); ++i, ++slotIt, ++faceIt) {
		auto& slot = *slotIt;
		auto& face = *faceIt;

		if (options().debug == -1) {
			LOG << "break";
		}

		if (failureCount > 0) {
			// Once in fail mode, we gather sample but these must not include failed
			// slots in their neighborhoods, se we skip such slots
			bool skip = false;
			auto neighborIt = face.neighbors.begin();
			for (auto neighborIdx : face.neighbors) {
				if (neighborIdx != -1 && slots()[neighborIdx].isEmpty()) {
					skip = true;
					break;
				}
			}
			if (skip) continue;
		}

		auto neighborIt = face.neighbors.begin();
		for (int k = 0; k < face.neighbors.size(); ++k, ++neighborIt) {
			if (*neighborIt == -1) {
				auto rel = static_cast<SlotTopology::RelationEnum>(k);

				// keep only tiles for which the edge type in direction 'rel' is a border edge
				TileSuperposition newSlot(noTiles);
				for (int x : slot) {
					int label = m_ruleset.table().get_at(x, static_cast<int>(rel));
					if (m_borderLabels.contains(label)) {
						newSlot.add(x);
					}
				}
				slot = newSlot;
			}
			else if (!m_borderOnlyLabels.empty()) {
				auto rel = static_cast<SlotTopology::RelationEnum>(k);

				// keep only tiles for which the edge type in direction 'rel' is NOT a border ONLY edge
				TileSuperposition newSlot(noTiles);
				for (int x : slot) {
					int label = m_ruleset.table().get_at(x, static_cast<int>(rel));
					if (!m_borderOnlyLabels.contains(label)) {
						newSlot.add(x);
					}
				}
				slot = newSlot;
			}
		}

		if (slot.isEmpty()) {
			logImpossibleNeighborhoods(i);
			++failureCount;
			if (failureCount >= options().maxAttempts) {
				return false;
			}
		}

		if (failureCount == 0) {
#ifndef NDEBUG
			//assert(checkIntegrity(true /* allowIncompatible */));
			if (!checkIntegrity(true /* allowIncompatible */)) {
				WARN_LOG << "Assertion warning!";
			}
#endif // NDEBUG
		}
	}

	if (failureCount == 0) {
		assert(checkIntegrity(true /* allowIncompatible */));
	}

	return failureCount == 0;
}

bool TilingSolverData::Solver::checkImpossibleNeighborhood(const std::array<std::optional<std::pair<TileSuperposition, libwfc::MeshRelation>>, 4>& neighborhood) const {
	std::array<std::unordered_set<int>, 4> neighborhoodLabels = getNeighborhoodLabels(neighborhood);

	// For each variant, check that it does not fit
	int variantCount = m_ruleset.table().shape(0);
	for (int variant = 0; variant < variantCount; ++variant) {
		if (fitsInNeighborhood(variant, neighborhoodLabels)) {
			return false;
		}
	}

	return true;
}

std::array<std::unordered_set<int>, 4> TilingSolverData::Solver::getNeighborhoodLabels(const std::array<std::optional<std::pair<TileSuperposition, libwfc::MeshRelation>>, 4>& neighborhood) const {
	const auto& table = m_ruleset.table();
	// Check that the neighborhood is indeed impossible
	std::array<std::unordered_set<int>, 4> neighborhoodLabels;
	for (int rel = 0; rel < 4; ++rel) {
		if (!neighborhood[rel].has_value()) {
			neighborhoodLabels[rel] = m_borderLabels;
		}
		else {
			for (const auto& variant : neighborhood[rel]->first) {
				int edgeIndex = table.get_at(variant, static_cast<int>(neighborhood[rel]->second));
				neighborhoodLabels[rel].insert(edgeIndex);
			}
		}
		assert(!neighborhoodLabels[rel].empty());
	}

	return neighborhoodLabels;
}

bool TilingSolverData::Solver::fitsInNeighborhood(int tileVariantIndex, std::array<std::unordered_set<int>, 4> neighborhoodLabels) const {
	const auto& table = m_ruleset.table();
	
	for (int rel = 0; rel < 4; ++rel) {
		int label = table.get_at(tileVariantIndex, rel);
		// The '-' is important since only labels of opposite signs are compatible
		if (!neighborhoodLabels[rel].contains(-label)) {
			return false;
		}
	}
	return true;
}

bool TilingSolverData::Solver::checkIntegrity(bool allowIncompatible) const {
	// For each slot, for each tile, we check that the tile is in the
	// superposition iff it is compatible with one or more neighbors

	for (int s = 0; s < slots().size(); ++s) {
		const auto& slot = slots()[s];

		// If the slot has been observed (reduced to 1 element), no check
		if (allowIncompatible && slot.tileCount() == 1) continue;

		auto neighborhoodLabels = getNeighborhoodLabels(getNeighborhood(s));
		for (int t = 0; t < slot.maxTileCount(); ++t) {
			bool compatibleWithNeighbors = fitsInNeighborhood(t, neighborhoodLabels);
			if (allowIncompatible) {
				if (compatibleWithNeighbors && !slot.contains(t)) {
					return false;
				}
			}
			else if (slot.tileCount() == 1) {
				// If the slot has been observed, only check that the only tile is allowed
				if (slot.contains(t) && !compatibleWithNeighbors) {
					return false;
				}
			}
			else {
				if (slot.contains(t) != compatibleWithNeighbors) {
					return false;
				}
			}
		}
	}

	return true;
}
