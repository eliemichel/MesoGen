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

#include <utils/streamutils.h>
#include <catch2/catch_test_macros.hpp>
#include <memory>

using TileSuperposition = libwfc::NaiveTileSuperposition<int>;
using libwfc::GridRelation;
using libwfc::GridSlotTopology;
using Ruleset = libwfc::Ruleset<int, TileSuperposition, GridRelation>;
using libwfc::ndvector;

using Model25D::TileTransform;
using Model25D::TileOrientation;
using Model25D::TileEdge;

GridRelation constexpr _ = GridRelation::PosX;

TEST_CASE("using tile flip in solving", "[Model25D]") {
	auto model = std::make_shared<Model25D::Tileset>();
	Serialization::deserializeFrom(model, SHARE_DIR "/samples/test/tileset01.25d.json");

	for (auto& tile : model->tileTypes) {
		tile->transformPermission.flipX = false;
		tile->transformPermission.flipY = false;
		tile->transformPermission.rotation = false;
	}

	REQUIRE(model->tileTypes.size() == 2);
	REQUIRE(model->edgeTypes.size() == 3);

	SECTION("no transform") {
		auto adapter = Tileset25DController::ConvertModelToRuleset(*model);
		REQUIRE(adapter.tileVariantList->variantCount() == 2);
		const auto& table = adapter.table;

		REQUIRE(table.shape(0) == 2);
		REQUIRE(table.shape(1) == 2);
		REQUIRE(table.shape(2) == Model25D::TileEdge::Count);

		Ruleset ruleset(table);

		std::unordered_set<int> set{ 0, 1 };
		TileSuperposition tileA(set); tileA.add(0);
		TileSuperposition tileB(set); tileB.add(1);
		TileSuperposition none(set);
		TileSuperposition both(set); both.setToAll();

		REQUIRE(ruleset.allowedStates(tileA, GridRelation::PosX, _) == tileB);
		REQUIRE(ruleset.allowedStates(tileA, GridRelation::PosY, _) == both);
		REQUIRE(ruleset.allowedStates(tileA, GridRelation::NegX, _) == tileB);
		REQUIRE(ruleset.allowedStates(tileA, GridRelation::NegY, _) == both);

		REQUIRE(ruleset.allowedStates(tileB, GridRelation::PosX, _) == tileA);
		REQUIRE(ruleset.allowedStates(tileB, GridRelation::PosY, _) == both);
		REQUIRE(ruleset.allowedStates(tileB, GridRelation::NegX, _) == tileA);
		REQUIRE(ruleset.allowedStates(tileB, GridRelation::NegY, _) == both);

		GridSlotTopology slotTopology(10, 1);
		libwfc::Solver solver(slotTopology, ruleset, TileSuperposition(set));
		solver.reset();
		solver.slots()[0].maskBy(tileA);
		REQUIRE(solver.solve(false));
		//REQUIRE(solver.stats().choiceCount == 0);
	}

	SECTION("flipX") {
		model->tileTypes[1]->transformPermission.flipX = true;
		REQUIRE(model->tileTypes[0]->transformPermission.flipX == false);

		auto adapter = Tileset25DController::ConvertModelToRuleset(*model);
		const auto& variants = adapter.tileVariantList;
		const auto& table = adapter.table;

		REQUIRE(variants->variantCount() == 3);
		{
			auto p = variants->variantToTile(0);
			REQUIRE(p.tileIndex == 0);
			REQUIRE(p.transform.flipX == false);
			REQUIRE(p.transform.flipY == false);
			REQUIRE(p.transform.orientation == Model25D::TileOrientation::Deg0);
		}
		{
			auto p = variants->variantToTile(1);
			REQUIRE(p.tileIndex == 1);
			REQUIRE(p.transform.flipX == false);
			REQUIRE(p.transform.flipY == false);
			REQUIRE(p.transform.orientation == TileOrientation::Deg0);
		}
		{
			auto p = variants->variantToTile(2);
			REQUIRE(p.tileIndex == 1);
			REQUIRE(p.transform.flipX == true);
			REQUIRE(p.transform.flipY == false);
			REQUIRE(p.transform.orientation == TileOrientation::Deg0);
		}

		REQUIRE(table.shape(0) == 3);
		REQUIRE(table.shape(1) == 3);
		REQUIRE(table.shape(2) == TileEdge::Count);

		Ruleset ruleset(table);

		std::unordered_set<int> set{ 0, 1, 2 };
		TileSuperposition tileA(set); tileA.add(0);
		TileSuperposition tileB(set); tileB.add(1);
		TileSuperposition tileBflipped(set); tileBflipped.add(2);
		TileSuperposition tileAandBflipped(set); tileAandBflipped.add(0); tileAandBflipped.add(2);
		TileSuperposition none(set);
		TileSuperposition any(set); any.setToAll();

		REQUIRE(ruleset.allowedStates(tileA, GridRelation::PosX, _) == tileB);
		REQUIRE(ruleset.allowedStates(tileA, GridRelation::PosY, _) == any);
		REQUIRE(ruleset.allowedStates(tileA, GridRelation::NegX, _) == tileB);
		REQUIRE(ruleset.allowedStates(tileA, GridRelation::NegY, _) == any);

		REQUIRE(ruleset.allowedStates(tileB, GridRelation::PosX, _) == tileAandBflipped);
		REQUIRE(ruleset.allowedStates(tileB, GridRelation::PosY, _) == any);
		REQUIRE(ruleset.allowedStates(tileB, GridRelation::NegX, _) == tileAandBflipped);
		REQUIRE(ruleset.allowedStates(tileB, GridRelation::NegY, _) == any);

		REQUIRE(ruleset.allowedStates(tileBflipped, GridRelation::PosX, _) == tileB);
		REQUIRE(ruleset.allowedStates(tileBflipped, GridRelation::PosY, _) == any);
		REQUIRE(ruleset.allowedStates(tileBflipped, GridRelation::NegX, _) == tileB);
		REQUIRE(ruleset.allowedStates(tileBflipped, GridRelation::NegY, _) == any);

		GridSlotTopology slotTopology(10, 1);
		libwfc::Solver solver(slotTopology, ruleset, TileSuperposition(set));
		solver.reset();
		solver.slots()[0].maskBy(tileA);
		REQUIRE(solver.solve(false));
		//REQUIRE(solver.stats().choiceCount == 4);
	}
}

TEST_CASE("using tile rotation in solving", "[Model25D]") {
	auto model = std::make_shared<Model25D::Tileset>();
	Serialization::deserializeFrom(model, SHARE_DIR "/samples/test/tileset02.25d.json");

	for (auto& tile : model->tileTypes) {
		tile->transformPermission.flipX = false;
		tile->transformPermission.flipY = false;
		tile->transformPermission.rotation = false;
	}

	REQUIRE(model->tileTypes.size() == 2);
	REQUIRE(model->edgeTypes.size() == 3);

	SECTION("no transform") {
		LOG << "-----------------";
		auto adapter = Tileset25DController::ConvertModelToRuleset(*model);
		REQUIRE(adapter.tileVariantList->variantCount() == 2);

		Ruleset ruleset(adapter.table);
		std::unordered_set<int> set{ 0, 1 };
		GridSlotTopology slotTopology(10, 1);
		libwfc::Solver solver(slotTopology, ruleset, TileSuperposition(set));
		solver.reset();
		solver.slots()[0].maskBy(TileSuperposition(set).add(0));
		REQUIRE(!solver.solve(false));
	}

	SECTION("rotation") {
		model->tileTypes[1]->transformPermission.rotation = true;

		auto adapter = Tileset25DController::ConvertModelToRuleset(*model);
		const auto& variants = adapter.tileVariantList;
		const auto& table = adapter.table;
		REQUIRE(variants->variantCount() == 5);

		std::map<std::shared_ptr<Model25D::EdgeType>, int> edgeTypeToIndex;
		for (int i = 0; i < model->edgeTypes.size(); ++i) {
			edgeTypeToIndex[model->edgeTypes[i]] = i;
		}

		SECTION("check tile A") {
			auto transformedTile = variants->variantToTile(0);
			const auto& tileTransform = transformedTile.transform;
			REQUIRE(transformedTile.tileIndex == 0);
			REQUIRE(tileTransform.flipX == false);
			REQUIRE(tileTransform.flipY == false);
			REQUIRE(tileTransform.orientation == TileOrientation::Deg0);
			const auto& tileType = *model->tileTypes[transformedTile.tileIndex];
			auto transformedEdgePosX = Tileset25DController::GetEdgeTypeInDirection(tileType, TileEdge::Direction::PosX, tileTransform);
			REQUIRE(edgeTypeToIndex[transformedEdgePosX.edge] == 0);
			REQUIRE(transformedEdgePosX.transform.flipped == false);
		}

		SECTION("check tile B0") {
			auto transformedTile = variants->variantToTile(1);
			const auto& tileTransform = transformedTile.transform;
			REQUIRE(transformedTile.tileIndex == 1);
			REQUIRE(tileTransform.flipX == false);
			REQUIRE(tileTransform.flipY == false);
			REQUIRE(tileTransform.orientation == TileOrientation::Deg0);
			const auto& tileType = *model->tileTypes[transformedTile.tileIndex];
			auto transformedEdgeNegY = Tileset25DController::GetEdgeTypeInDirection(tileType, TileEdge::Direction::NegY, tileTransform);
			REQUIRE(edgeTypeToIndex[transformedEdgeNegY.edge] == 2);
			REQUIRE(transformedEdgeNegY.transform.flipped == false);
		}

		SECTION("check tile B90") {
			auto transformedTile = variants->variantToTile(2);
			const auto& tileTransform = transformedTile.transform;
			REQUIRE(transformedTile.tileIndex == 1);
			REQUIRE(tileTransform.flipX == false);
			REQUIRE(tileTransform.flipY == false);
			REQUIRE(tileTransform.orientation == TileOrientation::Deg90);
			const auto& tileType = *model->tileTypes[transformedTile.tileIndex];

			auto transformedEdgePosX = Tileset25DController::GetEdgeTypeInDirection(tileType, TileEdge::Direction::PosX, tileTransform);
			REQUIRE(edgeTypeToIndex[transformedEdgePosX.edge] == 2);
			REQUIRE(transformedEdgePosX.transform.flipped == false);

			auto transformedEdgeNegX = Tileset25DController::GetEdgeTypeInDirection(tileType, TileEdge::Direction::NegX, tileTransform);
			REQUIRE(edgeTypeToIndex[transformedEdgeNegX.edge] == 0);
			REQUIRE(transformedEdgeNegX.transform.flipped == true);
		}

		SECTION("check tile A to tile B90") {
			auto transformedTileA = variants->variantToTile(0);
			const auto& tileA = *model->tileTypes[transformedTileA.tileIndex];
			auto edgeAPosX = Tileset25DController::GetEdgeTypeInDirection(tileA, TileEdge::Direction::PosX, transformedTileA.transform);
			auto edgeANegX = Tileset25DController::GetEdgeTypeInDirection(tileA, TileEdge::Direction::NegX, transformedTileA.transform);
			auto edgeAPosY = Tileset25DController::GetEdgeTypeInDirection(tileA, TileEdge::Direction::PosY, transformedTileA.transform);
			auto edgeANegY = Tileset25DController::GetEdgeTypeInDirection(tileA, TileEdge::Direction::NegY, transformedTileA.transform);

			auto transformedTileB90 = variants->variantToTile(2);
			const auto& tileB90 = *model->tileTypes[transformedTileB90.tileIndex];
			auto edgeB90PosX = Tileset25DController::GetEdgeTypeInDirection(tileB90, TileEdge::Direction::PosX, transformedTileB90.transform);
			auto edgeB90NegX = Tileset25DController::GetEdgeTypeInDirection(tileB90, TileEdge::Direction::NegX, transformedTileB90.transform);
			auto edgeB90PosY = Tileset25DController::GetEdgeTypeInDirection(tileB90, TileEdge::Direction::PosY, transformedTileB90.transform);
			auto edgeB90NegY = Tileset25DController::GetEdgeTypeInDirection(tileB90, TileEdge::Direction::NegY, transformedTileB90.transform);

			REQUIRE(Tileset25DController::AreEdgesCompatible(edgeB90NegX, edgeAPosX));
			REQUIRE(Tileset25DController::AreEdgesCompatible(edgeAPosX, edgeB90NegX));
			REQUIRE(Tileset25DController::AreEdgesCompatible(edgeANegX, edgeB90PosX));
			REQUIRE(Tileset25DController::AreEdgesCompatible(edgeB90PosX, edgeANegX));

			REQUIRE(Tileset25DController::AreEdgesCompatible(edgeB90NegY, edgeAPosY));
			REQUIRE(Tileset25DController::AreEdgesCompatible(edgeB90NegY, edgeB90PosY));
			REQUIRE(Tileset25DController::AreEdgesCompatible(edgeANegY, edgeB90PosY));
			REQUIRE(Tileset25DController::AreEdgesCompatible(edgeANegY, edgeAPosY));
		}

		REQUIRE(table.shape(0) == 5);
		REQUIRE(table.shape(1) == 5);
		REQUIRE(table.shape(2) == Model25D::TileEdge::Count);

		Ruleset ruleset(table);

		std::unordered_set<int> set{ 0, 1, 2, 3, 4 };
		
		for (int x : set) {
			auto super = TileSuperposition(set).add(x);
			for (int direction = 0; direction < 4; ++direction) {
				auto relation = static_cast<GridRelation>(direction);
				for (int y : ruleset.allowedStates(super, relation, _)) {
					REQUIRE(ruleset.allows(x, relation, y, _));
				}
			}
		}

		std::map<TileEdge::Direction, TileEdge::Direction> dualDirectionLut = {
			{ TileEdge::Direction::PosX, TileEdge::Direction::NegX },
			{ TileEdge::Direction::NegX, TileEdge::Direction::PosX },
			{ TileEdge::Direction::PosY, TileEdge::Direction::NegY },
			{ TileEdge::Direction::NegY, TileEdge::Direction::PosY },
		};
		for (int k = 0; k < 4; ++k) {
			auto direction = static_cast<TileEdge::Direction>(k);
			auto relation = static_cast<GridRelation>(k);
			auto dualDirection = dualDirectionLut[direction];

			for (int x : set) {
				auto transformedTileX = variants->variantToTile(x);
				const auto& tileX = *model->tileTypes[transformedTileX.tileIndex];
				auto edgeX = Tileset25DController::GetEdgeTypeInDirection(tileX, direction, transformedTileX.transform);

				for (int y : set) {
					
					auto transformedTileY = variants->variantToTile(y);
					const auto& tileY = *model->tileTypes[transformedTileY.tileIndex];
					auto edgeY = Tileset25DController::GetEdgeTypeInDirection(tileY, dualDirection, transformedTileY.transform);
					
					bool allowed = Tileset25DController::AreEdgesCompatible(edgeX, edgeY);

					INFO("k = " << k << ", x = " << x << ", y = " << y);
					INFO("edgeX = #" << edgeTypeToIndex[edgeX.edge] << ", flipped: " << edgeX.transform.flipped);
					INFO("edgeY = #" << edgeTypeToIndex[edgeY.edge] << ", flipped: " << edgeY.transform.flipped);

					REQUIRE(allowed == ruleset.allows(x, relation, y, _));
				}
			}
		}

		auto tileA = TileSuperposition(set).add(0);
		auto tileB0 = TileSuperposition(set).add(1);
		auto tileB90 = TileSuperposition(set).add(2);
		auto tileB180 = TileSuperposition(set).add(3);
		auto tileB270 = TileSuperposition(set).add(4);
		auto tileAandB90 = TileSuperposition(set).add(0).add(2);
		auto none = TileSuperposition(set);
		auto any = TileSuperposition(set).setToAll();

		REQUIRE(ruleset.allowedStates(tileA, GridRelation::PosX, _) == tileB90);
		REQUIRE(ruleset.allowedStates(tileA, GridRelation::PosY, _) == tileAandB90);
		REQUIRE(ruleset.allowedStates(tileA, GridRelation::NegX, _) == tileB90);
		REQUIRE(ruleset.allowedStates(tileA, GridRelation::NegY, _) == tileAandB90);

		REQUIRE(ruleset.allowedStates(tileB0, GridRelation::PosX, _) == tileB0);
		REQUIRE(ruleset.allowedStates(tileB0, GridRelation::PosY, _) == none);
		REQUIRE(ruleset.allowedStates(tileB0, GridRelation::NegX, _) == tileB0);
		REQUIRE(ruleset.allowedStates(tileB0, GridRelation::NegY, _) == none);

		REQUIRE(ruleset.allowedStates(tileB90, GridRelation::PosX, _) == tileA);
		REQUIRE(ruleset.allowedStates(tileB90, GridRelation::PosY, _) == tileAandB90);
		REQUIRE(ruleset.allowedStates(tileB90, GridRelation::NegX, _) == tileA);
		REQUIRE(ruleset.allowedStates(tileB90, GridRelation::NegY, _) == tileAandB90);

		REQUIRE(ruleset.allowedStates(tileB180, GridRelation::PosX, _) == tileB180);
		REQUIRE(ruleset.allowedStates(tileB180, GridRelation::PosY, _) == none);
		REQUIRE(ruleset.allowedStates(tileB180, GridRelation::NegX, _) == tileB180);
		REQUIRE(ruleset.allowedStates(tileB180, GridRelation::NegY, _) == none);

		REQUIRE(ruleset.allowedStates(tileB270, GridRelation::PosX, _) == none);
		REQUIRE(ruleset.allowedStates(tileB270, GridRelation::PosY, _) == tileB270);
		REQUIRE(ruleset.allowedStates(tileB270, GridRelation::NegX, _) == none);
		REQUIRE(ruleset.allowedStates(tileB270, GridRelation::NegY, _) == tileB270);

		GridSlotTopology slotTopology(10, 1);
		libwfc::Solver solver(slotTopology, ruleset, TileSuperposition(set));
		REQUIRE(solver.solve());
		//REQUIRE(solver.stats().choiceCount == 0);
	}
}
