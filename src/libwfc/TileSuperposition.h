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

#include <Logger.h>

#include <unordered_set>
#include <random>

namespace libwfc {

#if 0 // not used so far, templates are enough
template <typename Tile, typename TileIterable>
class AbstractTileSuperposition {
public:
    using tile_type = Tile;
    using const_iterator = TileIterable::const_iterator;

    virtual void add(const Tile& tile) = 0;

    virtual const TileIterable& tileset() const = 0;

    virtual const_iterator begin() const = 0;

    virtual const_iterator end() const = 0;
};
#endif

/**
 * An example of TileSuperposition, to be used mostly for testing and reference
 * since it is not optimized at all.
 */
template <typename Tile>
class NaiveTileSuperposition {
public:
    using tile_type = Tile;
    using const_iterator = typename std::unordered_set<Tile>::const_iterator;

    NaiveTileSuperposition(const std::unordered_set<Tile>& tileset)
        : m_tileset(tileset)
    {}

    static NaiveTileSuperposition FromPrototype(const NaiveTileSuperposition& proto) {
        return NaiveTileSuperposition(proto.tileset());
    }

    NaiveTileSuperposition& add(const Tile& tile) {
        m_elements.insert(tile);
        return *this;
    }

    const std::unordered_set<Tile>& tileset() const { return m_tileset; }

    const_iterator begin() const {
        return m_elements.cbegin();
    }

    const_iterator end() const {
        return m_elements.cend();
    }

    NaiveTileSuperposition& setToAll() {
        m_elements.clear();
        for (const auto& t : tileset()) {
            add(t);
        }
        return *this;
    }

    NaiveTileSuperposition& setToNone() {
        m_elements.clear();
        return *this;
    }

    /**
     * Keep in this superposition only the ones that are also contained in `other`
     * Return true if this superposition changed
     */
    bool maskBy(const NaiveTileSuperposition& other) {
        std::unordered_set<Tile> old_elements(m_elements);
        m_elements.clear();
        bool changed = false;
        for (const auto& t : old_elements) {
            if (other.m_elements.find(t) != other.m_elements.end()) {
                add(t);
            }
            else {
                changed = true;
            }
        }
        return changed;
    }

    bool isEmpty() const {
        return m_elements.empty();
    }

    float measureEntropy() {
        return std::max(0.0f, static_cast<float>(m_elements.size()) - 1);
    }
    
    int tileCount() const {
        return static_cast<int>(m_elements.size());
    }

    template<class Generator>
    void observe(Generator& rand_engine) {
        //DEBUG_LOG << "Observing a superposition of " << m_elements.size() << " items: " << *this;
        std::uniform_int_distribution dist(0, static_cast<int>(m_elements.size()) - 1);

        int index = dist(rand_engine);
        //DEBUG_LOG << "Random index is " << index;

        auto it = m_elements.begin();
        std::advance(it, index);
        Tile tile = *it;

        m_elements.clear();
        m_elements.insert(tile);
    }

    bool operator==(const NaiveTileSuperposition<Tile>& other) const {
        return other.m_elements == m_elements;
    }

    bool operator!=(const NaiveTileSuperposition<Tile>& other) const {
        return !(other == *this);
    }

    friend std::ostream& operator<<(std::ostream& os, const NaiveTileSuperposition<Tile>& x) {
        os << "NaiveTileSuperposition{";
        bool first = true;
        for (const Tile& t : x) {
            if (!first) os << ", ";
            os << t;
            first = false;
        }
        os << "}";
        return os;
    }

    bool checkIntegrity() const {
        return true;
    }

private:
    std::unordered_set<Tile> m_elements;
    std::unordered_set<Tile> m_tileset;
};

} // namespace libwfc
