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
#include "Model.h"
#include "TilesetController.h"
#include "Serialization.h"
#include "BMesh.h"
#include "BMeshIo.h"
#include "TileVariantList.h"

#include <TinyTimer.h>
#include <libwfc.h>
#include <catch2/catch_test_macros.hpp>
#include <sstream>
#include <unordered_set>

using TinyTimer::Timer;

using libwfc::ndvector;
using libwfc::MeshRelation;
using libwfc::GridSlotTopology;
using libwfc::Ruleset;
using libwfc::GridRelation;
using libwfc::Solver;

using FastTileSuperposition = libwfc::FastTileSuperposition<>;
using TileSuperposition = libwfc::NaiveTileSuperposition<int>;
using SignedWangRuleset = libwfc::SignedWangRuleset<int, TileSuperposition, MeshRelation>;
using FastSignedWangRuleset = libwfc::FastSignedWangRuleset<MeshRelation>;

enum PerfCounterType {
	BaselineSolverTime,
	FastSolverTime
};

TEST_CASE("FastSuperposition basic operations", "[FastSuperposition]") {
	int maxTileCount = 10;
	FastTileSuperposition superposition(maxTileCount);

	REQUIRE(superposition.tileCount() == 10);

	superposition.setToNone();

	REQUIRE(superposition.tileCount() == 0);

	superposition.add(8);

	REQUIRE(superposition.tileCount() == 1);

	{
		auto it = superposition.begin();
		auto end = superposition.end();
		REQUIRE(*it == 8);
		++it;
		REQUIRE(it == end);
	}

	superposition.add(1);

	REQUIRE(superposition.tileCount() == 2);

	{
		auto it = superposition.begin();
		auto end = superposition.end();
		REQUIRE(*it == 1);
		++it;
		REQUIRE(*it == 8);
		++it;
		REQUIRE(it == end);
	}

	superposition.add(8);

	REQUIRE(superposition.tileCount() == 2);

	{
		auto it = superposition.begin();
		auto end = superposition.end();
		REQUIRE(*it == 1);
		++it;
		REQUIRE(*it == 8);
		++it;
		REQUIRE(it == end);
	}

	superposition.add(0);

	{
		auto it = superposition.begin();
		REQUIRE(*it == 0);
	}

	{
		FastTileSuperposition superposition2(superposition);
		FastTileSuperposition other(maxTileCount);
		REQUIRE(superposition2.tileCount() == 3);
		REQUIRE(other.tileCount() == maxTileCount);
		bool changed = superposition2.maskBy(other);
		REQUIRE(superposition2.tileCount() == 3);
		REQUIRE(changed == false);

		{
			auto it = superposition2.begin();
			auto end = superposition2.end();
			REQUIRE(*it == 0);
			++it;
			REQUIRE(*it == 1);
			++it;
			REQUIRE(*it == 8);
			++it;
			REQUIRE(it == end);
		}
	}

	{
		FastTileSuperposition superposition2(superposition);
		FastTileSuperposition other(maxTileCount);
		other.setToNone();
		REQUIRE(superposition2.tileCount() == 3);
		REQUIRE(other.tileCount() == 0);
		bool changed = superposition2.maskBy(other);
		REQUIRE(superposition2.tileCount() == 0);
		REQUIRE(changed == true);
	}

	{
		FastTileSuperposition superposition2(superposition);
		FastTileSuperposition other(maxTileCount);
		other.setToNone();
		other.add(9);
		other.add(1);
		REQUIRE(superposition2.tileCount() == 3);
		REQUIRE(other.tileCount() == 2);
		bool changed = superposition2.maskBy(other);
		REQUIRE(superposition2.tileCount() == 1);
		REQUIRE(changed == true);

		{
			auto it = superposition2.begin();
			auto end = superposition2.end();
			REQUIRE(*it == 1);
			++it;
			REQUIRE(it == end);
		}
	}
}
/*
TEST_CASE("FastSuperposition matches NaiveSuperposition", "[FastSuperposition]") {
	auto bmesh = bmesh::loadObj(SHARE_DIR "/samples/test/torus2.obj");
	//auto bmesh = bmesh::loadObj(SHARE_DIR "/samples/test/grid.obj");
	auto topology = bmesh::asSlotTopology(*bmesh);

	auto model = std::make_shared<Model::Tileset>();
	Serialization::deserializeFrom(model, SHARE_DIR "/samples/test/tileset01.25d.json");

	auto adapter = TilesetController::ConvertModelToSignedWangRuleset(*model);

	// Naive
	std::vector<int> solutionNaive;
	{
		std::unordered_set<int> tileset;
		for (int i = 0; i < adapter.tileVariantList->variantCount(); ++i) {
			tileset.insert(i);
		}

		SignedWangRuleset ruleset(adapter.table);
		libwfc::Solver solver(*topology, ruleset, TileSuperposition(tileset));
		solver.reset();
		//solver.options().maxSteps = 10;
		//solver.options().maxAttempts = 1;
		Timer timer;
		solver.solve();
		PERF(BaselineSolverTime).add_sample(timer);
		for (const auto& s : solver.slots()) {
			solutionNaive.push_back(*s.begin());
		}
	}

	// Fast
	std::vector<int> solutionFast;
	{
		FastSignedWangRuleset ruleset(adapter.table, adapter.tileVariantList->variantCount());
		libwfc::Solver<FastTileSuperposition, int, MeshRelation, int> solver(*topology, ruleset, FastTileSuperposition(adapter.tileVariantList->variantCount()));
		solver.reset();
		//solver.options().maxSteps = 10;
		//solver.options().maxAttempts = 1;
		Timer timer;
		solver.solve();
		PERF(FastSolverTime).add_sample(timer);
		for (const auto& s : solver.slots()) {
			solutionFast.push_back(*s.begin());
		}
	}

	REQUIRE(solutionNaive == solutionFast);

	LOG << "Performance counters:";
	LOG << "BaselineSolverTime: " << PERF(BaselineSolverTime);
	LOG << "FastSolverTime:     " << PERF(FastSolverTime);
}
*/
TEST_CASE("FastSuperposition matches NaiveSuperposition example 2", "[FastSuperposition]") {
	PERF(BaselineSolverTime).reset();
	PERF(FastSolverTime).reset();

	auto bmesh = bmesh::loadObj(SHARE_DIR "/samples/test/smooth-shell.obj");
	//auto bmesh = bmesh::loadObj(SHARE_DIR "/samples/test/grid.obj");
	auto topology = bmesh::asSlotTopology(*bmesh);

	auto model = std::make_shared<Model::Tileset>();
	Serialization::deserializeFrom(model, SHARE_DIR "/samples/test/tileset.osier2e.json");

	auto adapter = TilesetController::ConvertModelToSignedWangRuleset(*model);

	// Fast
	std::vector<std::unordered_set<int>> solutionFast;
	{
		FastSignedWangRuleset ruleset(adapter.table, adapter.tileVariantList->variantCount());
		REQUIRE(adapter.tileVariantList->variantCount() == 80);
		REQUIRE(adapter.table.get_at(79, 0) == 2);
		REQUIRE(adapter.table.get_at(79, 1) == 2);
		REQUIRE(adapter.table.get_at(79, 2) == 2);
		REQUIRE(adapter.table.get_at(79, 3) == 2);
		libwfc::Solver<FastTileSuperposition, int, MeshRelation, int> solver(*topology, ruleset, FastTileSuperposition(adapter.tileVariantList->variantCount()));
		solver.reset();
		solver.options().maxSteps = 1000;
		solver.options().maxAttempts = 1;
		Timer timer;
		solver.solve();
		PERF(FastSolverTime).add_sample(timer);
		for (const auto& s : solver.slots()) {
			std::unordered_set<int> set;
			for (int x : s) set.insert(x);
			solutionFast.push_back(set);
		}
	}

	// Naive
	std::vector<std::unordered_set<int>> solutionNaive;
	{
		std::unordered_set<int> tileset;
		for (int i = 0; i < adapter.tileVariantList->variantCount(); ++i) {
			tileset.insert(i);
		}

		SignedWangRuleset ruleset(adapter.table);
		libwfc::Solver solver(*topology, ruleset, TileSuperposition(tileset));
		solver.reset();
		solver.options().maxSteps = 1000;
		solver.options().maxAttempts = 1;
		Timer timer;
		solver.solve();
		PERF(BaselineSolverTime).add_sample(timer);
		int slotIndex = 0;
		for (const auto& s : solver.slots()) {
			std::unordered_set<int> set;
			for (int x : s) set.insert(x);
			solutionNaive.push_back(set);
			

			INFO("slotIndex = %d", slotIndex);
			REQUIRE(set == solutionFast[slotIndex]);

			++slotIndex;
		}
	}

	REQUIRE(solutionNaive == solutionFast);

	LOG << "Performance counters:";
	LOG << "BaselineSolverTime: " << PERF(BaselineSolverTime);
	LOG << "FastSolverTime:     " << PERF(FastSolverTime);
}
