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
#include "OutputSlotsObject.h"

#include <catch2/catch_test_macros.hpp>
#include <memory>

using TileSuperposition = libwfc::NaiveTileSuperposition<int>;
using libwfc::GridRelation;
using libwfc::Ruleset;
using libwfc::ndvector;

GridRelation constexpr _ = GridRelation::PosX;

TEST_CASE("model to libwfc conversion works", "[Model]") {
	auto model = std::make_shared<Model::Tileset>();
	
	model->edgeTypes.push_back(std::make_shared<Model::EdgeType>());
	model->edgeTypes.push_back(std::make_shared<Model::EdgeType>());
	model->edgeTypes.push_back(std::make_shared<Model::EdgeType>());
	model->edgeTypes.push_back(std::make_shared<Model::EdgeType>());

	model->tileTypes.push_back(std::make_shared<Model::TileType>());
	model->tileTypes.push_back(std::make_shared<Model::TileType>());

	model->tileTypes[0]->edges[Model::TileEdge::PosX].type = model->edgeTypes[0];
	model->tileTypes[0]->edges[Model::TileEdge::PosX].transform.flipped = false;
	model->tileTypes[0]->edges[Model::TileEdge::NegX].type = model->edgeTypes[1];
	model->tileTypes[0]->edges[Model::TileEdge::NegX].transform.flipped = true;
	model->tileTypes[0]->edges[Model::TileEdge::PosY].type = model->edgeTypes[2];
	model->tileTypes[0]->edges[Model::TileEdge::PosY].transform.flipped = false;
	model->tileTypes[0]->edges[Model::TileEdge::NegY].type = model->edgeTypes[2];
	model->tileTypes[0]->edges[Model::TileEdge::NegY].transform.flipped = true;

	model->tileTypes[1]->edges[Model::TileEdge::PosX].type = model->edgeTypes[1];
	model->tileTypes[1]->edges[Model::TileEdge::PosX].transform.flipped = false;
	model->tileTypes[1]->edges[Model::TileEdge::NegX].type = model->edgeTypes[0];
	model->tileTypes[1]->edges[Model::TileEdge::NegX].transform.flipped = true;
	model->tileTypes[1]->edges[Model::TileEdge::PosY].type = model->edgeTypes[3];
	model->tileTypes[1]->edges[Model::TileEdge::PosY].transform.flipped = false;
	model->tileTypes[1]->edges[Model::TileEdge::NegY].type = model->edgeTypes[3];
	model->tileTypes[1]->edges[Model::TileEdge::NegY].transform.flipped = true;

	ndvector<bool, 3> table = OutputSlotsObject::convertModelToWaveFunctionCollapse(*model);
	Ruleset<int, TileSuperposition, GridRelation> ruleset(table);
	int tileCount = static_cast<int>(model->tileTypes.size());

	SECTION("ruleset is symmetric") {
		for (int a = 0; a < tileCount; ++a) {
			for (int b = 0; b < tileCount; ++b) {
				for (int r = 0; r < 2; ++r) {
					int dualr = r + 2;
					INFO("Table at (" << a << ", " << b << ", relation=" << a << ")");
					REQUIRE(table.get_at(b, a, dualr) == table.get_at(a, b, r));
				}

				REQUIRE(ruleset.allows(a, GridRelation::PosX, b, _) == ruleset.allows(b, GridRelation::NegX, a, _));
				REQUIRE(ruleset.allows(a, GridRelation::NegX, b, _) == ruleset.allows(b, GridRelation::PosX, a, _));
				REQUIRE(ruleset.allows(a, GridRelation::PosY, b, _) == ruleset.allows(b, GridRelation::NegY, a, _));
				REQUIRE(ruleset.allows(a, GridRelation::NegY, b, _) == ruleset.allows(b, GridRelation::PosY, a, _));
			}
		}
	}

	SECTION("focus on a single rule") {
		int labelA = 0;
		int labelB = 0;

		int tileEdgeIndexA = Model::TileEdge::PosX;
		int tileEdgeIndexB = Model::TileEdge::NegX;

		auto edgeTypeA = model->tileTypes[labelA]->edges[tileEdgeIndexA].type.lock();
		auto flippedA = model->tileTypes[labelA]->edges[tileEdgeIndexA].transform.flipped;

		auto edgeTypeB = model->tileTypes[labelB]->edges[tileEdgeIndexB].type.lock();
		auto flippedB = model->tileTypes[labelB]->edges[tileEdgeIndexB].transform.flipped;

		REQUIRE(flippedA == false);
		REQUIRE(flippedB == true);

		REQUIRE(edgeTypeB != edgeTypeA);
		REQUIRE(flippedB != flippedA);

		bool allowed = edgeTypeB == edgeTypeA && flippedB != flippedA;

		REQUIRE(allowed == false);
	}

	SECTION("ruleset is consistent") {
		REQUIRE(ruleset.allows(0, GridRelation::PosX, 0, _) == false);
		REQUIRE(ruleset.allows(0, GridRelation::NegX, 0, _) == false);

		REQUIRE(ruleset.allows(0, GridRelation::PosX, 1, _) == true);
		REQUIRE(ruleset.allows(0, GridRelation::NegX, 1, _) == true);

		REQUIRE(ruleset.allows(1, GridRelation::PosX, 1, _) == false);
		REQUIRE(ruleset.allows(1, GridRelation::NegX, 1, _) == false);

		REQUIRE(ruleset.allows(0, GridRelation::PosY, 0, _) == true);
		REQUIRE(ruleset.allows(0, GridRelation::NegY, 0, _) == true);

		REQUIRE(ruleset.allows(0, GridRelation::PosY, 1, _) == false);
		REQUIRE(ruleset.allows(0, GridRelation::NegY, 1, _) == false);

		REQUIRE(ruleset.allows(1, GridRelation::PosY, 1, _) == true);
		REQUIRE(ruleset.allows(1, GridRelation::NegY, 1, _) == true);
	}
}
