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

#include <utils/streamutils.h>
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <sstream>

using TileSuperposition = libwfc::NaiveTileSuperposition<int>;
using libwfc::GridRelation;
using libwfc::GridSlotTopology;
using Ruleset = libwfc::Ruleset<int, TileSuperposition, GridRelation>;
using libwfc::ndvector;

using Model::TileTransform;
using Model::TileOrientation;
using Model::TileEdge;

GridRelation constexpr _ = GridRelation::PosX;

TEST_CASE("solve with tileset04", "[Model]") {
	auto model = std::make_shared<Model::Tileset>();
	Serialization::deserializeFrom(model, SHARE_DIR "/samples/test/tileset04.25d.json");

	REQUIRE(model->tileTypes.size() == 2);
	REQUIRE(model->edgeTypes.size() == 3);

	auto adapter = TilesetController::ConvertModelToRuleset(*model);
	int variantCount = adapter.tileVariantList->variantCount();
	REQUIRE(variantCount == 3);
	const auto& table = adapter.table;

	REQUIRE(table.shape(0) == variantCount);
	REQUIRE(table.shape(1) == variantCount);
	REQUIRE(table.shape(2) == Model::TileEdge::Count);

	Ruleset ruleset(table);

	std::unordered_set<int> tileset;
	for (int i = 0; i < adapter.tileVariantList->variantCount(); ++i) {
		tileset.insert(i);
	}

	int w = 4, h = 3;
	GridSlotTopology slotTopology(w, h);
	libwfc::Solver solver(slotTopology, ruleset, TileSuperposition(tileset));
	solver.options().randomSeed = 0;
	REQUIRE(solver.solve());

	std::ostringstream ss;
	const auto& slots = solver.slots();
	libwfc::ndvector<int, 2> output(w, h);
	for (int j = h - 1; j >= 0; --j) {
		for (int i = 0; i < w; ++i) {
			int variant = *slots[j * w + i].begin();
			output.set_at(variant, i, j);
			auto transformedTile = adapter.tileVariantList->variantToTile(variant);
			ss << (transformedTile.transform.flipX ? "f" : " ");
			ss << transformedTile.tileIndex << "\t";
		}
		ss << "\n";
	}
	LOG << "Output:\n" << ss.str();

	SECTION ("Check output consistency") {
		for (int a = 0; a < slotTopology.slotCount(); ++a) {
			for (int b = 0; b < slotTopology.slotCount(); ++b) {
				int sa = *slots[a].begin();
				int sb = *slots[b].begin();

				int ia, ja, ib, jb;
				slotTopology.fromIndex(a, ia, ja);
				slotTopology.fromIndex(b, ib, jb);

				if (!(ia == ib && jb == ja + 1)) continue;

				INFO("a = (" << ia << ", " << ja << "), b = (" << ib << ", " << jb << ")");

				REQUIRE(a == slotTopology.index(ia, ja));
				REQUIRE(b == slotTopology.index(ib, jb));

				{
					auto neighbor = slotTopology.neighborOf(a, GridRelation::PosY);
					REQUIRE(neighbor.has_value());
					REQUIRE(neighbor->first == b);
					REQUIRE(neighbor->second == GridRelation::NegY);
				}
				{
					auto neighbor = slotTopology.neighborOf(b, GridRelation::NegY);
					REQUIRE(neighbor.has_value());
					REQUIRE(neighbor->first == a);
					REQUIRE(neighbor->second == GridRelation::PosY);
				}

				REQUIRE(ruleset.allows(sa, GridRelation::PosY, sb, GridRelation::NegY));
				REQUIRE(ruleset.allows(sb, GridRelation::NegY, sa, GridRelation::PosY));

				auto transformedTileA = adapter.tileVariantList->variantToTile(sa);
				auto& tileTypeA = *model->tileTypes[transformedTileA.tileIndex];
				int tileEdgeIndex = static_cast<int>(Model::TileEdge::Direction::PosY);
				auto transformedEdgeA = TilesetController::GetEdgeTypeInDirection(tileTypeA, Model::TileEdge::Direction::PosY, transformedTileA.transform);

				bool fx = transformedTileA.transform.flipX;
				bool fy = transformedTileA.transform.flipY;
				REQUIRE(transformedEdgeA.transform.flipped == (fx ? !fy : fy));

				auto transformedTileB = adapter.tileVariantList->variantToTile(sb);
				auto& tileTypeB = *model->tileTypes[transformedTileB.tileIndex];
				auto transformedEdgeB = TilesetController::GetEdgeTypeInDirection(tileTypeB, Model::TileEdge::Direction::NegY, transformedTileB.transform);
			}
		}
	}
}
