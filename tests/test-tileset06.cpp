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
#include "libwfc.h"
#include "Model.h"
#include "TilesetController.h"
#include "Serialization.h"
#include "BMesh.h"
#include "BMeshIo.h"

#include <utils/streamutils.h>
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <sstream>

using TileSuperposition = libwfc::NaiveTileSuperposition<int>;
using libwfc::GridRelation;
using libwfc::GridSlotTopology;
using Ruleset = libwfc::SignedWangRuleset<int, TileSuperposition, GridRelation>;
using libwfc::ndvector;

using Model::TileTransform;
using Model::TileOrientation;
using Model::TileEdge;

TEST_CASE("solve with tileset06", "[Model]") {
	auto model = std::make_shared<Model::Tileset>();
	Serialization::deserializeFrom(model, SHARE_DIR "/samples/test/tileset06.25d.json");

	REQUIRE(model->tileTypes.size() == 2);
	REQUIRE(model->edgeTypes.size() == 4);

	auto adapter = TilesetController::ConvertModelToSignedWangRuleset(*model);
	int variantCount = adapter.tileVariantList->variantCount();
	REQUIRE(variantCount == 8);
	const auto& table = adapter.table;

	REQUIRE(table.shape(0) == variantCount);
	REQUIRE(table.shape(1) == Model::TileEdge::Count);

	Ruleset ruleset(table);
	int w = 4, h = 3;
	GridSlotTopology topology(w, h);

	std::unordered_set<int> tileset;
	for (int i = 0; i < adapter.tileVariantList->variantCount(); ++i) {
		tileset.insert(i);
	}

	libwfc::Solver solver(topology, ruleset, TileSuperposition(tileset));
	
	std::vector<int> tileOccurrences(model->tileTypes.size(), 0);
	int mixCaseCount = 0;

	for (int i = 0; i < 100; ++i) {
		solver.options().randomSeed = i;
		REQUIRE(solver.solve());

		std::vector<int> localTileOccurrences(model->tileTypes.size(), 0);

		for (auto s : solver.slots()) {
			int variant = *s.begin();
			auto transformedTile = adapter.tileVariantList->variantToTile(variant);
			++tileOccurrences[transformedTile.tileIndex];
			++localTileOccurrences[transformedTile.tileIndex];
		}

		bool localMixCase = localTileOccurrences[0] > 0 && localTileOccurrences[1] > 0;
		if (localMixCase) {
			++mixCaseCount;
			LOG << "Mixed Case for seed " << i;
		}
	}

	int expectation = w * h * 100 / 2;
	for (int count : tileOccurrences) {
		REQUIRE(count > expectation / 10);
	}
	REQUIRE(mixCaseCount > 1);
}
