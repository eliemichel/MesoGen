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
#include <libwfc.h>
#include <catch2/catch_test_macros.hpp>
#include <sstream>

using libwfc::ndvector;
using libwfc::GridSlotTopology;
using libwfc::Ruleset;
using libwfc::GridRelation;
using libwfc::Solver;

constexpr auto _ = GridRelation::PosX;

TEST_CASE("int nbvector in 2d is working", "[nbvector]") {
	ndvector<int, 2> vec(3, 4);

	REQUIRE(vec.stride(0) == 1);
	REQUIRE(vec.stride(1) == 3);

	REQUIRE(vec.shape(0) == 3);
	REQUIRE(vec.shape(1) == 4);

	SECTION("nbvector can be written to") {
		vec.at(1, 2) = 8;
		REQUIRE(vec.at(1, 2) == 8);
	}
}

TEST_CASE("bool nbvector in 3d is working", "[nbvector]") {
	ndvector<bool, 3> table(9, 8, 4);

	REQUIRE(table.stride(0) == 1);
	REQUIRE(table.stride(1) == 9);
	REQUIRE(table.stride(2) == 9 * 8);

	REQUIRE(table.shape(0) == 9);
	REQUIRE(table.shape(1) == 8);
	REQUIRE(table.shape(2) == 4);

	SECTION("nbvector can be written to") {
		table.set_at(true, 1, 2, 1);
		REQUIRE(table.get_at(1, 2, 1) == true);

		table.set_at(false, 1, 2, 2);
		REQUIRE(table.get_at(1, 2, 2) == false);
	}
}

TEST_CASE("NaiveTileSuperposition is consistent", "[NaiveTileSuperposition]") {
	using TileSuperposition = libwfc::NaiveTileSuperposition<int>;

	std::unordered_set<int> tileset{ 0, 1, 3 };
	TileSuperposition none(tileset);
	TileSuperposition a(tileset);
	a.add(0);
	TileSuperposition b(tileset);
	b.add(1);
	TileSuperposition c(tileset);
	c.add(2);
	TileSuperposition ab(tileset);
	ab.add(0);
	ab.add(1);
	TileSuperposition bc(tileset);
	bc.add(1);
	bc.add(2);
	TileSuperposition ca(tileset);
	ca.add(2);
	ca.add(0);
	TileSuperposition abc(tileset);
	abc.add(0);
	abc.add(1);
	abc.add(2);

	SECTION("equality and inequality") {
		REQUIRE(ab != abc);
		REQUIRE(TileSuperposition(bc) == bc);
		REQUIRE(TileSuperposition(none) == TileSuperposition(tileset));
		REQUIRE(none.isEmpty());
		REQUIRE(!a.isEmpty());
		REQUIRE(!b.isEmpty());
		REQUIRE(!c.isEmpty());
		REQUIRE(!abc.isEmpty());
	}

	SECTION("maskBy removes states from the superposition") {
		{
			TileSuperposition x(abc);
			REQUIRE(x.maskBy(ab));
			REQUIRE(x == ab);
		}

		{
			TileSuperposition x(a);
			REQUIRE(!x.maskBy(a));
			REQUIRE(x == a);
		}

		{
			TileSuperposition x(ca);
			REQUIRE(x.maskBy(ab));
			REQUIRE(x == a);
		}

		{
			TileSuperposition x(bc);
			REQUIRE(x.maskBy(a));
			REQUIRE(x == none);
		}
	}

	SECTION("display") {
		std::ostringstream ss;
		ss << ca;
		const std::string str = ss.str();
		REQUIRE((str == "NaiveTileSuperposition{0, 2}" || str == "NaiveTileSuperposition{2, 0}"));
	}
}

TEST_CASE("checkerboard tileset", "[Ruleset]") {
	using TileSuperposition = libwfc::NaiveTileSuperposition<int>;

	int tileCount = 2;
	ndvector<bool, 3> table(tileCount, tileCount, 4);
	table.set_at(false, 0, 0, 0); // a -PosX-> a
	table.set_at(false, 0, 0, 1); // a -PosY-> a

	table.set_at(true, 0, 1, 0); // a -PosX-> b
	table.set_at(true, 0, 1, 1); // a -PosY-> b

	table.set_at(true, 1, 0, 0); // b -PosX-> a
	table.set_at(true, 1, 0, 1); // b -PosY-> a

	table.set_at(false, 1, 1, 0); // b -PosX-> b
	table.set_at(false, 1, 1, 1); // b -PosY-> b

	for (int a = 0; a < tileCount; ++a) {
		for (int b = 0; b < tileCount; ++b) {
			for (int r = 0; r < 2; ++r) {
				int dualr = r + 2;
				bool value = table.get_at(a, b, r);
				table.set_at(value, b, a, dualr);
			}
		}
	}

	std::unordered_set<int> tileset{ 0, 1 };
	TileSuperposition onlyA(tileset);
	onlyA.add(0);
	TileSuperposition onlyB(tileset);
	onlyB.add(1);
	TileSuperposition any(tileset);
	any.add(0);
	any.add(1);

	Ruleset<int, TileSuperposition, GridRelation> ruleset(table);

	SECTION("ruleset is consistent") {
		REQUIRE(ruleset.allows(0, GridRelation::NegX, 0, _) == false);
		REQUIRE(ruleset.allows(0, GridRelation::PosX, 1, _) == true);
		REQUIRE(ruleset.allows(0, GridRelation::NegX, 1, _) == true);
		REQUIRE(ruleset.allows(1, GridRelation::PosX, 0, _) == true);
		REQUIRE(ruleset.allows(1, GridRelation::NegY, 1, _) == false);

		REQUIRE(ruleset.allowedStates(onlyB, GridRelation::NegY, _) == onlyA);
		REQUIRE(ruleset.allowedStates(any, GridRelation::NegY, _) == any);
	}

	int width = 5;
	int height = 6;
	GridSlotTopology slotTopology(width, height);

	SECTION("slot topology is consistent") {
		{
			auto idx = slotTopology.index(2, 4);
			REQUIRE(slotTopology.neighborOf(idx, GridRelation::PosX)->first == slotTopology.index(3, 4));
			REQUIRE(slotTopology.neighborOf(idx, GridRelation::NegX)->first == slotTopology.index(1, 4));
			REQUIRE(slotTopology.neighborOf(idx, GridRelation::PosY)->first == slotTopology.index(2, 5));
			REQUIRE(slotTopology.neighborOf(idx, GridRelation::NegY)->first == slotTopology.index(2, 3));
		}

		for (int x = 0; x < width; ++x) {
			for (int y = 0; y < height; ++y) {
				if (y > 0) {
					auto idx = slotTopology.index(x, y);
					auto nidx = slotTopology.neighborOf(idx, GridRelation::NegY);
					REQUIRE(nidx.has_value());
					REQUIRE(slotTopology.neighborOf(nidx->first, GridRelation::PosY)->first == idx);
				}
				if (y < height - 1) {
					auto idx = slotTopology.index(x, y);
					auto nidx = slotTopology.neighborOf(idx, GridRelation::PosY);
					REQUIRE(nidx.has_value());
					REQUIRE(slotTopology.neighborOf(nidx->first, GridRelation::NegY)->first == idx);
				}
				if (x > 0) {
					auto idx = slotTopology.index(x, y);
					auto nidx = slotTopology.neighborOf(idx, GridRelation::NegX);
					REQUIRE(nidx.has_value());
					REQUIRE(slotTopology.neighborOf(nidx->first, GridRelation::PosX)->first == idx);
				}
				if (x < width - 1) {
					auto idx = slotTopology.index(x, y);
					auto nidx = slotTopology.neighborOf(idx, GridRelation::PosX);
					REQUIRE(nidx.has_value());
					REQUIRE(slotTopology.neighborOf(nidx->first, GridRelation::NegX)->first == idx);
				}
			}
		}
	}

	Solver solver(slotTopology, ruleset, TileSuperposition(tileset));
	solver.options().maxSteps = 20;

	SECTION("solver initial state is consistent") {
		REQUIRE(solver.slots().size() == width * height);
		for (const auto& slot : solver.slots()) {
			REQUIRE(slot.tileset() == tileset);
		}
	}

	REQUIRE(solver.solve());

	SECTION("solver result has no superposition") {
		for (const auto& slot : solver.slots()) {
			REQUIRE(slot.tileCount() == 1);
		}
	}

	SECTION("solver result respects the rules") {
		int x, y;
		for (int index = 0; index < solver.slots().size(); ++index) {
			const auto& slot = solver.slots()[index];
			slotTopology.fromIndex(index, x, y);

			if (x > 0) {
				int nindex = slotTopology.index(x - 1, y);
				REQUIRE(nindex == slotTopology.neighborOf(index, GridRelation::NegX)->first);
				const auto& neighbor = solver.slots()[nindex];
				REQUIRE(ruleset.allows(*slot.begin(), GridRelation::NegX, *neighbor.begin(), _));
			}

			if (x < width - 1) {
				int nindex = slotTopology.index(x + 1, y);
				REQUIRE(nindex == slotTopology.neighborOf(index, GridRelation::PosX)->first);
				const auto& neighbor = solver.slots()[nindex];
				REQUIRE(ruleset.allows(*slot.begin(), GridRelation::PosX, *neighbor.begin(), _));
			}

			if (y > 0) {
				int nindex = slotTopology.index(x, y - 1);
				REQUIRE(nindex == slotTopology.neighborOf(index, GridRelation::NegY)->first);
				const auto& neighbor = solver.slots()[nindex];
				REQUIRE(ruleset.allows(*slot.begin(), GridRelation::NegY, *neighbor.begin(), _));
			}

			if (y < height - 1) {
				int nindex = slotTopology.index(x, y + 1);
				REQUIRE(nindex == slotTopology.neighborOf(index, GridRelation::PosY)->first);
				const auto& neighbor = solver.slots()[nindex];
				REQUIRE(ruleset.allows(*slot.begin(), GridRelation::PosY, *neighbor.begin(), _));
			}
		}
	}

	SECTION("solver result is a checkerboard") {
		auto outputTileAt = [&solver, &slotTopology](int x, int y) { return *solver.slots()[slotTopology.index(x, y)].begin(); };
		int ref = outputTileAt(0, 0);
		WARN("Reference tile at (0, 0) is " << ref);

		for (int x = 0; x < width; ++x) {
			for (int y = 0; y < height; ++y) {
				int tile = outputTileAt(x, y);
				INFO("Tile at (" << x << ", " << y << ") is " << tile);
				bool matchRef = tile == ref;
				bool isEven = x % 2 == y % 2;
				REQUIRE(matchRef == isEven);
			}
		}
	}
}
