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
using libwfc::MeshRelation;
using libwfc::MeshSlotTopology;
using Ruleset = libwfc::SignedWangRuleset<int, TileSuperposition, MeshRelation>;
using libwfc::ndvector;

using Model::TileTransform;
using Model::TileOrientation;
using Model::TileEdge;

TEST_CASE("solve with tileset05", "[Model]") {
	auto bmesh = bmesh::loadObj(SHARE_DIR "/samples/test/sorted-grid.obj");
	auto topology = bmesh::asSlotTopology(*bmesh);

	REQUIRE(topology->slotCount() == 9);

	auto model = std::make_shared<Model::Tileset>();
	Serialization::deserializeFrom(model, SHARE_DIR "/samples/test/tileset05.25d.json");

	REQUIRE(model->tileTypes.size() == 2);
	REQUIRE(model->edgeTypes.size() == 3);

	auto adapter = TilesetController::ConvertModelToSignedWangRuleset(*model);
	int variantCount = adapter.tileVariantList->variantCount();
	REQUIRE(variantCount == 6);
	const auto& table = adapter.table;

	REQUIRE(table.shape(0) == variantCount);
	REQUIRE(table.shape(1) == Model::TileEdge::Count);

	Ruleset ruleset(table);

	std::unordered_set<int> tileset;
	for (int i = 0; i < adapter.tileVariantList->variantCount(); ++i) {
		tileset.insert(i);
	}

	libwfc::Solver solver(*topology, ruleset, TileSuperposition(tileset));
	solver.options().randomSeed = 0;
	REQUIRE(solver.solve());

	std::ostringstream ss;
	const auto& slots = solver.slots();
	int w = 3, h = 3;
	libwfc::ndvector<int, 2> output(w, h);
	for (int j = h - 1; j >= 0; --j) {
		for (int i = 0; i < w; ++i) {
			int variant = *slots[j * w + i].begin();
			output.set_at(variant, i, j);
			auto transformedTile = adapter.tileVariantList->variantToTile(variant);
			ss << (transformedTile.transform.flipX ? "x" : ".");
			ss << (transformedTile.transform.flipY ? "y" : ".");
			ss << transformedTile.tileIndex << "\t";
		}
		ss << "\n";
	}
	LOG << "Output:\n" << ss.str();

	SECTION ("Check slot topology") {

		auto PosX = MeshRelation::Neighbor0;
		auto PosY = MeshRelation::Neighbor1;
		auto NegX = MeshRelation::Neighbor2;
		auto NegY = MeshRelation::Neighbor3;

		REQUIRE(bmesh->faces[0]->loop->vertex == bmesh->vertices[14]);
		REQUIRE(bmesh->faces[0]->loop->next->vertex == bmesh->vertices[13]);

		auto& faces = topology->faces();
		for (int a = 0; a < 9; ++a) {
			INFO("a = " << a);
			REQUIRE(faces[a].index == a);
			std::vector<bmesh::Face*> neighbors = bmesh->NeighborFaces(bmesh->faces[a]);

			int expected;
			expected = a + w < 9 ? a + w : -1;
			if (expected != -1) REQUIRE(neighbors[static_cast<int>(PosY)] == bmesh->faces[expected]);
			REQUIRE(faces[a].neighbors[static_cast<int>(PosY)] == expected);

			expected = (a%w) + 1 < w ? a + 1 : -1;
			if (expected != -1) REQUIRE(neighbors[static_cast<int>(PosX)] == bmesh->faces[expected]);
			REQUIRE(faces[a].neighbors[static_cast<int>(PosX)] == expected);
		}

		for (int a = 0; a < 9; ++a) {
			INFO("a = " << a);

			{
				int b = a + 1;
				auto neighbor = topology->neighborOf(a, PosX);
				REQUIRE(neighbor.has_value() == (a % w) + 1 < w);
				if (neighbor.has_value()) {
					REQUIRE(neighbor->first == b);
					REQUIRE(neighbor->second == NegX);
				}
			}

			{
				int b = a + w;
				auto neighbor = topology->neighborOf(a, PosY);
				REQUIRE(neighbor.has_value() == b < 9);
				if (neighbor.has_value()) {
					REQUIRE(neighbor->first == b);
					REQUIRE(neighbor->second == NegY);
				}
			}
		}

		std::map<MeshRelation, TileEdge::Direction> relationLut = {
			{PosX, TileEdge::Direction::PosX},
			{NegX, TileEdge::Direction::NegX},
			{PosY, TileEdge::Direction::PosY},
			{NegY, TileEdge::Direction::NegY},
		};

		std::map<MeshRelation, MeshRelation> dualRelation = {
			{PosX, NegX},
			{NegX, PosX},
			{PosY, NegY},
			{NegY, PosY},
		};

		for (int a = 0; a < 9; ++a) {
			INFO("a = " << a);
			int variantA = *slots[a].begin();
			auto transformedTileA = adapter.tileVariantList->variantToTile(variantA);
			auto& tileA = *model->tileTypes[transformedTileA.tileIndex];

			if (false && a == 0) {
				REQUIRE(transformedTileA.tileIndex == 0);
				REQUIRE(transformedTileA.transform.flipX == true);
				REQUIRE(transformedTileA.transform.flipY == true);
			}

			for (int i = 0; i < 4; ++i) {
				auto rel = static_cast<MeshRelation>(i);

				auto neighbor = topology->neighborOf(a, rel);
				if (neighbor.has_value()) {
					auto transformedEdgeA = TilesetController::GetEdgeTypeInDirection(tileA, relationLut[rel], transformedTileA.transform);
					REQUIRE(neighbor->second == dualRelation[rel]);

					int b = neighbor->first;
					int variantB = *slots[b].begin();
					auto transformedTileB = adapter.tileVariantList->variantToTile(variantB);
					auto& tileB = *model->tileTypes[transformedTileB.tileIndex];

					auto transformedEdgeB = TilesetController::GetEdgeTypeInDirection(tileB, relationLut[neighbor->second], transformedTileB.transform);

					INFO("b = " << b);
					INFO("rel = " << magic_enum::enum_name(rel) << " (" << static_cast<int>(rel) << ")");
					INFO("variantA = " << variantA);

					int labelA = table.get_at(variantA, static_cast<int>(relationLut[rel]));
					int labelB = table.get_at(variantB, static_cast<int>(relationLut[neighbor->second]));

					if (false && a == 0 && rel == PosX) {
						REQUIRE(b == 1);
						REQUIRE(transformedTileB.tileIndex == 0);
						REQUIRE(transformedTileB.transform.flipX == true);
						REQUIRE(transformedTileB.transform.flipY == false);

						REQUIRE(transformedEdgeA.edge == model->edgeTypes[2]);
						REQUIRE(transformedEdgeA.transform.flipped == true);

						REQUIRE(transformedEdgeB.edge == model->edgeTypes[0]);
						REQUIRE(transformedEdgeB.transform.flipped == true);

						REQUIRE(labelA == -3);
						REQUIRE(labelB == -1);
					}

					REQUIRE(ruleset.allows(variantA, rel, variantB, neighbor->second));
					REQUIRE(TilesetController::AreEdgesCompatible(transformedEdgeA, transformedEdgeB));
				}
			}
		}
	}
}
