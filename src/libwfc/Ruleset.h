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

#include "ndvector.h"

#include <magic_enum.hpp>

#include <unordered_map>
#include <memory>

namespace libwfc {

/**
 * Tile is typically just 'int'
 *
 * SuperposedState represents a set of Tile types. Could be just std::set<Tile>
 * but it is more advised to use some bitfield based representation.
 * It must implement a `add(Tile tile)` class to add a tile to the superposition
 * This type must also include a `tileset()` returning an iterable over the set
 * of tiles that could be in the superposition, and takes this tileset in its
 * constructor.
 *
 * RelationEnum lists all the possible relations between two tiles,
 * e.g. North, South, East, West
 */
template <typename Tile, typename TileSuperposition, typename RelationEnum>
class AbstractRuleset {
public:
    /**
     * Return the probability that two slots X and Y are respectively
     * in states `x` and `y` knowing that going from `x` through `relationX` leads to `y`
     * and going from `y` through `relationY` leads to `x`.
     */
    virtual bool allows(const Tile& x, RelationEnum relationX, const Tile& y, RelationEnum relationY) const = 0;

    /**
     * Return a superposition of all states that are possible for `Y` if `X` is
     * in superposed state `x` and `X` and `Y` are bound by a connection of type
     * `relationX` from `x` to `y` and `relationY` from `y` to `x`.
     *
     * Implementing this is optional, a default implementation is provided
     * if you derive from AbstractRuleset defined bellow, but you should
     * choose to override it for optimization and use the default
     * implementation to test against, as the behavior must be the same.
     */
    virtual TileSuperposition allowedStates(const TileSuperposition& x, RelationEnum relationX, RelationEnum relationY) const {
        TileSuperposition y = TileSuperposition::FromPrototype(x);
        for (const auto& yy : x.tileset()) {
            bool allowed = false;
            for (const auto& xx : x) {
                if (allows(xx, relationX, yy, relationY)) {
                    assert(allows(yy, relationY, xx, relationX));
                    allowed = true;
                    break;
                }
            }
            if (allowed) {
                y.add(yy);
            }
        }
        return y;
    }
};

inline int tile_index(int x) { return x; }

/**
 * A simple rule set based on a (tile count, tile count, relation count) array of booleans
 * A static Tile::tile_index(Tile) or ::tile_index(Tile) must exist, converting a tile into a size_t
 */
template <typename Tile, typename TileSuperposition, typename RelationEnum>
class Ruleset : public AbstractRuleset<Tile, TileSuperposition, RelationEnum> {
public:
    Ruleset(const ndvector<bool, 3>& table)
        : m_table(table)
    {
        assert(table.shape(0) == table.shape(1));
        assert(table.shape(2) == magic_enum::enum_count<RelationEnum>());
    }

    bool allows(const Tile& x, RelationEnum relationX, const Tile& y, RelationEnum relationY) const override {
        (void)relationY; // not used by this version
        int relation_index = static_cast<int>(relationX);
        return m_table.get_at(tile_index(x), tile_index(y), relation_index);
    }

private:
    const ndvector<bool, 3>& m_table;
};

/**
 * A ruleset based on Wang tiles, where tiles are associated labels on
 * their edges, that are used to validate connections.
 * On top of having teh same "color" (absolute value), edge labels must have opposite
 * signs to be allowed to be connected.
 */
template <typename Tile, typename TileSuperposition, typename RelationEnum>
class SignedWangRuleset : public AbstractRuleset<Tile, TileSuperposition, RelationEnum> {
public:
    SignedWangRuleset(const ndvector<int, 2>& table)
        : m_table(table)
    {
        assert(table.shape(1) == magic_enum::enum_count<RelationEnum>());
    }

    bool allows(const Tile& x, RelationEnum relationX, const Tile& y, RelationEnum relationY) const override {
        int labelX = m_table.get_at(x, static_cast<int>(relationX));
        int labelY = m_table.get_at(y, static_cast<int>(relationY));
        return labelX == -labelY;
    }

    const ndvector<int, 2>& table() const { return m_table; }

private:
    const ndvector<int, 2>& m_table;
};

/**
 * A version of SignedWangRuleset specialized for FastTileSuperposition.
 */
template <typename RelationEnum>
class FastSignedWangRuleset : public AbstractRuleset<int, FastTileSuperposition<>, RelationEnum> {
public:
    using Tile = int;
    using TileSuperposition = FastTileSuperposition<>;

    FastSignedWangRuleset(const ndvector<int, 2>& table, int maxTileCount)
        : m_table(table)
    {
        assert(table.shape(1) == magic_enum::enum_count<RelationEnum>());

        m_maxLabel = 0;
        for (int i = 0; i < m_table.shape(0); ++i) {
            for (int j = 0; j < m_table.shape(1); ++j) {
                int label = m_table.get_at(i, j);
                m_maxLabel = std::max(m_maxLabel, abs(label));
            }
        }

        int labelCount = 2 * m_maxLabel + 1;
        int relCount = m_table.shape(1);
        m_memoized.resize(labelCount * relCount);

        for (int label = -m_maxLabel; label <= m_maxLabel; ++label) {
            for (int rel = 0; rel < relCount; ++rel) {

                auto y = std::make_unique<TileSuperposition>(maxTileCount);
                y->setToNone();
                for (int yy = 0; yy < maxTileCount; ++yy) {
                    int labelY = m_table.get_at(yy, rel);
                    if (labelY == -label) {
                        y->add(yy);
                    }
                }

                m_memoized[(m_maxLabel + label) * relCount + rel] = std::move(y);
            }
        }
    }

    bool allows(const Tile& x, RelationEnum relationX, const Tile& y, RelationEnum relationY) const override {
        //int labelX = m_table.get_at(x, static_cast<int>(relationX));
        //int labelY = m_table.get_at(y, static_cast<int>(relationY));
        //return labelX == -labelY;

        (void)x;
        (void)relationX;
        (void)y;
        (void)relationY;
        assert(false);
        return false;
    }

    TileSuperposition allowedStates(const TileSuperposition& x, RelationEnum relationX, RelationEnum relationY) const override {
        TileSuperposition y = TileSuperposition::FromPrototype(x);
        int relCount = m_table.shape(1);

        std::vector<bool> useMemoized(m_memoized.size(), false);
        for (int xx : x) {
            int labelX = m_table.get_at(xx, static_cast<int>(relationX));
            useMemoized[(m_maxLabel + labelX) * relCount + static_cast<int>(relationY)] = true;
        }

        auto memoizedIt = m_memoized.cbegin();
        auto useMemoizedIt = useMemoized.cbegin();
        auto memoizedEnd = m_memoized.cend();
        for (; memoizedIt < memoizedEnd; ++memoizedIt, ++useMemoizedIt) {
            if (*useMemoizedIt) {
                y.add(**memoizedIt);
            }
        }

        return y;
    }

    const ndvector<int, 2>& table() const { return m_table; }

private:
    const ndvector<int, 2>& m_table;
    int m_maxLabel;
    std::vector<std::unique_ptr<TileSuperposition>> m_memoized;
};

} // namespace libwfc
