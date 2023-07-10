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

#include "TileSuggestion.h"
#include "Logger.h"

#include <cassert>

using namespace TileSuggestion;

//-----------------------------------------------------------------------------
// Tile

bool Tile::operator==(const Tile& other) const {
	for (int i = 0; i < 4; ++i) {
		if (labels[i] != other.labels[i]) return false;
	}
	return true;
}

std::size_t TileHasher::operator()(const Tile& tile) const
{
	size_t hash = 0;
	for (int i = 0; i < 4; ++i) {
		hash = (hash + (324723947 + tile.labels[i])) ^ 93485734985;
	}
	return hash;
}

//-----------------------------------------------------------------------------
// TransformRequirement

bool TransformRequirement::operator==(const TransformRequirement& other) const {
	return (
		(other.flipX == flipX) &&
		(other.flipY == flipY) &&
		(other.rotation == rotation)
		);
}

std::size_t TransformRequirementHasher::operator()(const TransformRequirement& tr) const
{
	size_t hash = 0;
	if (tr.flipX) hash += 0b001;
	if (tr.flipY) hash += 0b010;
	if (tr.rotation) hash += 0b100;
	return hash;
}

//-----------------------------------------------------------------------------
// Transform

TransformRequirement Transform::requirements() const {
	return TransformRequirement{
		flipX,
		flipY,
		orientation != Orientation::Deg0
	};
}

Tile Transform::apply(const Tile& tile) const {
	Tile transformedTile;
	for (int i = 0; i < 4; ++i) {
		int label = tile.labels[i];
		int j = i;
		j = (j + 4 - static_cast<int>(orientation)) % 4;
		if (flipX) {
			label = -label;
			if (j % 2 == 0) j = (j + 2) % 4;
		}
		if (flipY) {
			label = -label;
			if (j % 2 == 1) j = (j + 2) % 4;
		}
		transformedTile.labels[j] = label;
	}
	return transformedTile;
}

Tile Transform::applyInverse(const Tile& tile) const {
	Tile transformedTile;
	for (int i = 0; i < 4; ++i) {
		int label = tile.labels[i];
		int j = i;
		if (flipX) {
			label = -label;
			if (j % 2 == 0) j = (j + 2) % 4;
		}
		if (flipY) {
			label = -label;
			if (j % 2 == 1) j = (j + 2) % 4;
		}
		j = (j + static_cast<int>(orientation)) % 4;
		transformedTile.labels[j] = label;
	}
	return transformedTile;
}

std::vector<Transform> Transform::ComputeAllTransforms() {
	std::vector<Transform> allTransforms;
	for (int fx = 0; fx < 2; ++fx) {
		for (int fy = 0; fy < 2; ++fy) {
			for (int rot = 0; rot < 4; ++rot) {
				allTransforms.push_back(Transform{
					fx == 1,
					fy == 1,
					static_cast<Orientation>(rot),
					});
			}
		}
	}
	return allTransforms;
}

//-----------------------------------------------------------------------------
// Score

bool Score::operator>(const Score& other) const {
	if (newEdgeCount < other.newEdgeCount) return true;
	if (newEdgeCount > other.newEdgeCount) return false;
	if (totalVotes > other.totalVotes) return true;
	if (totalVotes < other.totalVotes) return false;
	if (flipCount < other.flipCount) return true;
	if (flipCount > other.flipCount) return false;
	if (untransformedVotes > other.untransformedVotes) return true;
	if (untransformedVotes < other.untransformedVotes) return false;
	return false;
}

//-----------------------------------------------------------------------------
// VoteTable

void TileSuggestion::consolidateVotes(
	const std::vector<std::array<Set, 4>>& impossibleNeighborhoods,
	VoteTable& votes,
	const std::vector<Transform>& allowedTransforms
) {
	for (const auto& neighborhood : impossibleNeighborhoods) {
		Set neighborhood0(neighborhood[0]); if(neighborhood0.empty()) neighborhood0.insert(0);
		Set neighborhood1(neighborhood[1]); if (neighborhood1.empty()) neighborhood1.insert(0);
		Set neighborhood2(neighborhood[2]); if (neighborhood2.empty()) neighborhood2.insert(0);
		Set neighborhood3(neighborhood[3]); if (neighborhood3.empty()) neighborhood3.insert(0);
		for (const auto& label0 : neighborhood0) {
			for (const auto& label1 : neighborhood1) {
				for (const auto& label2 : neighborhood2) {
					for (const auto& label3 : neighborhood3) {

						Tile transformedTile{ -label0, -label1, -label2, -label3 };
						// For all possible transform
						// Inverse transform the tile
						for (const auto& transform : allowedTransforms) {
							Tile sourceTile = transform.applyInverse(transformedTile);
							TransformRequirement tr = transform.requirements();
							votes[sourceTile][tr]++;
						}

					}
				}
			}
		}
	}
}

std::pair<Tile,Tile> TileSuggestion::findBest(const VoteTable& votes, const std::unordered_set<Tile, TileHasher>& blacklist) {
	TransformRequirement untransformed;
	Score bestScore;
	Tile bestTile = {0, 0, 0, 0};
	Tile secondBestTile = { 0, 0, 0, 0 };
	bool first = true;

	for (auto const& [tile, entry] : votes)
	{
		if (blacklist.count(tile) > 0) {
			assert(false); // not supposed to occur
			ERR_LOG << "Blacklist should not affect findBest()!";
			continue;
		}

		Score score;
		for (int i = 0; i < 4; ++i) {
			if (tile.labels[i] < 0) score.flipCount++;
			if (tile.labels[i] == 0) score.newEdgeCount++;
		}

		score.totalVotes = 0;
		for (auto const& [transformRequirement, count] : entry) {
			score.totalVotes += count;

			if (transformRequirement == untransformed) {
				score.untransformedVotes = count;
			}
		}

		if (first || score > bestScore) {
			secondBestTile = bestTile;
			bestTile = tile;
			bestScore = score;
			first = false;
		}
	}

#if 1
	std::cout << std::endl;
	std::cout << "Best Tile [" << bestTile.labels[0] << "," << bestTile.labels[1] << "," << bestTile.labels[2] << "," << bestTile.labels[3] << "]" << std::endl;
	std::cout
		<< "Best Score { "
		<< "totalVotes: " << bestScore.totalVotes << ", "
		<< "flipCount: " << bestScore.flipCount << ", "
		<< "untransformedVotes: " << bestScore.untransformedVotes << " }"
		<< std::endl;
#endif

	return std::make_pair(bestTile, secondBestTile);
}
