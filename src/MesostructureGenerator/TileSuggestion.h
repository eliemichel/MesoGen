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

#include <array>
#include <unordered_set>
#include <unordered_map>
#include <iostream>

/**
 * Tools related to the voting system that suggests new tile to be added to
 * the dictionnary given the list of cases in which the solver failed.
 */
namespace TileSuggestion {

using Set = std::unordered_set<int>;

struct Tile {
	std::array<int, 4> labels;

	bool operator==(const Tile& other) const;
};

struct TileHasher
{
	std::size_t operator()(const Tile& tile) const;
};

struct TransformRequirement {
	bool flipX = false;
	bool flipY = false;
	bool rotation = false;

	bool operator==(const TransformRequirement& other) const;
};

struct TransformRequirementHasher {
	std::size_t operator()(const TransformRequirement& tr) const;
};

enum class Orientation {
	Deg0,
	Deg90,
	Deg180,
	Deg270,
};

struct Transform {
	bool flipX = false;
	bool flipY = false;
	Orientation orientation = Orientation::Deg0;

	TransformRequirement requirements() const;

	Tile apply(const Tile& tile) const;
	Tile applyInverse(const Tile& tile) const;

	static std::vector<Transform> ComputeAllTransforms();
};

struct Score {
	int totalVotes = 0;
	int flipCount = 0;
	int untransformedVotes = 0;
	int newEdgeCount = 0;

	bool operator>(const Score& other) const;
};

using VoteTable = std::unordered_map<Tile, std::unordered_map<TransformRequirement, int, TransformRequirementHasher>, TileHasher>;

// @param allowedTransforms Allowed transforms for the new tile to be created
void consolidateVotes(const std::vector<std::array<Set, 4>>& impossibleNeighborhoods, VoteTable& votes, const std::vector<Transform>& allowedTransforms);

/**
 * @param blacklist tiles not allowed to be elected
 * @return the best tile and the second best tile
 */
std::pair<Tile, Tile> findBest(const VoteTable& votes, const std::unordered_set<Tile, TileHasher>& blacklist);

} // namespace TileSuggestion

