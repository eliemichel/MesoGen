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
#include "Model25D.h"
#include "Tileset25DController.h"
#include "Serialization.h"
#include "BMesh.h"
#include "BMeshIo.h"

#include <utils/streamutils.h>
#include <catch2/catch_test_macros.hpp>
#include <memory>

using TileSuperposition = libwfc::NaiveTileSuperposition<int>;
using libwfc::MeshRelation;
using libwfc::MeshSlotTopology;
using SignedWangRuleset = libwfc::SignedWangRuleset<int, TileSuperposition, MeshRelation>;
using libwfc::ndvector;

using Model25D::TileTransform;
using Model25D::TileOrientation;
using Model25D::TileEdge;

TEST_CASE("mesh based slot topology, regular grid", "[Model25D]") {
	auto bmesh = bmesh::loadObj(SHARE_DIR "/samples/test/grid.obj");
	auto topology = bmesh::asSlotTopology(*bmesh);

	REQUIRE(topology->slotCount() == 9);

	auto model = std::make_shared<Model25D::Tileset>();
	Serialization::deserializeFrom(model, SHARE_DIR "/samples/test/tileset01.25d.json");

	auto adapter = Tileset25DController::ConvertModelToSignedWangRuleset(*model);
	SignedWangRuleset ruleset(adapter.table);

	std::unordered_set<int> tileset;
	for (int i = 0; i < adapter.tileVariantList->variantCount(); ++i) {
		tileset.insert(i);
	}

	libwfc::Solver solver(*topology, ruleset, TileSuperposition(tileset));
	solver.reset();
	REQUIRE(solver.solve());
}

int neighborFaceIndex(bmesh::BMesh &bmesh, bmesh::Face *face, bmesh::Face* neighborFace) {
	int i = 0;
	for (auto nf : bmesh.NeighborFaces(face)) {
		if (nf == neighborFace) return i;
		++i;
	}
	return -1;
}

TEST_CASE("mesh based slot topology, singular grid", "[Model25D]") {
	auto bmesh = bmesh::loadObj(SHARE_DIR "/samples/test/singular-grid.obj");
	auto topology = bmesh::asSlotTopology(*bmesh);

	REQUIRE(topology->slotCount() == 12);

	auto model = std::make_shared<Model25D::Tileset>();
	Serialization::deserializeFrom(model, SHARE_DIR "/samples/test/tileset03.25d.json");

	REQUIRE(model->tileTypes.size() == 2);

	auto adapter = Tileset25DController::ConvertModelToSignedWangRuleset(*model);
	auto& table = adapter.table;
	SignedWangRuleset ruleset(table);

	REQUIRE(table.shape(0) == adapter.tileVariantList->variantCount());
	REQUIRE(table.shape(1) == 4);

	std::unordered_set<int> tileset;
	for (int i = 0; i < adapter.tileVariantList->variantCount(); ++i) {
		tileset.insert(i);
	}

	libwfc::Solver solver(*topology, ruleset, TileSuperposition(tileset));
	solver.reset();
	REQUIRE(solver.solve());

	solver.slots();

	std::map<bmesh::Face*, int> faceToSlot;
	int i = 0;
	for (auto& f : bmesh->faces) {
		faceToSlot[f] = i;
		++i;
	}

	int edgeIndex = 0;
	for (auto& e : bmesh->edges) {
		auto f0 = e->loop->face;
		auto f1 = e->loop->radialNext->face;
		bool isBorder = f1 == f0;
		REQUIRE(e->loop->radialNext->radialNext->face == f0);

		int s0 = faceToSlot[f0];
		int s1 = faceToSlot[f1];
		
		int direction0to1 = neighborFaceIndex(*bmesh, f0, f1);
		int direction1to0 = neighborFaceIndex(*bmesh, f1, f0);
		if (!isBorder) REQUIRE(direction0to1 != -1);
		if (!isBorder) REQUIRE(direction1to0 != -1);

		INFO("Edge #" << edgeIndex << ": " << s0 << " --" << direction0to1 << "--" << direction1to0 << "--> " << s1);

		auto relation0to1 = static_cast<MeshRelation>(direction0to1);
		auto relation1to0 = static_cast<MeshRelation>(direction1to0);

		auto n0 = topology->neighborOf(s0, relation0to1);
		REQUIRE(n0.has_value() == !isBorder);
		if (n0.has_value()) {
			REQUIRE(n0->first == s1);
			REQUIRE(n0->second == relation1to0);
		}

		auto n1 = topology->neighborOf(s1, relation1to0);
		REQUIRE(n1.has_value() == !isBorder);
		if (n1.has_value()) {
			REQUIRE(n1->first == s0);
			REQUIRE(n1->second == relation0to1);
		}

		++edgeIndex;
	}
}
