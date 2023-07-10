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

namespace libwfc {

/**
 * A tile superposition that is more optimized than NaiveTileSuperposition, but
 * specialized to int tiles.
 */
template <typename block_t = uint32_t, int block_size = 8 * sizeof(block_t)>
class FastTileSuperposition {
private:
    static const block_t block_ones = block_t(~block_t(0));
public:
    using tile_type = int;
    class const_iterator {
    public:
        const_iterator(const const_iterator& other)
            : m_superposition(other.m_superposition)
            , m_index(other.m_index)
        {}
        int operator*() {
            return m_index;
        }
        void operator++() {
            if (m_index == m_superposition.maxTileCount()) return;
            bool found = false;
            while (!found) {
                ++m_index;
                if (m_index == m_superposition.maxTileCount()) return;
                int indexBase = m_index / block_size;
                int indexFine = m_index - block_size * indexBase;
                found = (m_superposition.m_elements[indexBase] & (1 << indexFine)) != 0;
            }
        }
        bool operator==(const const_iterator& other) const {
            return m_index == other.m_index;
        }

    private:
        const_iterator(const FastTileSuperposition& superposition, bool end = false)
            : m_superposition(superposition)
            , m_index(end ? superposition.m_maxTileCount : 0)
        {
            if (!end && ((m_superposition.m_elements[0] & (1 << 0)) == 0)) {
                ++(*this);
            }
        }

    private:
        const FastTileSuperposition& m_superposition;
        int m_index;

        friend class FastTileSuperposition;
    };

    FastTileSuperposition(int maxTileCount)
        : m_maxTileCount(maxTileCount)
        , m_elements(static_cast<size_t>(ceil(static_cast<float>(maxTileCount) / block_size)))
    {
        setToAll();
    }

    static FastTileSuperposition FromPrototype(const FastTileSuperposition& proto) {
        return FastTileSuperposition(proto.maxTileCount()).setToNone();
    }

    // Do not use with Fast superposition
    std::unordered_set<int> tileset() const {
        std::unordered_set<int> s;
        for (int x = 0; x < maxTileCount(); ++x) {
            s.insert(x);
        }
        return s;
    }

    const_iterator begin() const {
        return const_iterator(*this);
    }

    const_iterator end() const {
        return const_iterator(*this, true);
    }

    FastTileSuperposition& setToAll() {
        fill(m_elements.begin(), m_elements.end(), block_ones);
        block_t& last = m_elements[m_elements.size() - 1];

        int unusedBits = static_cast<int>(m_elements.size() * block_size) - maxTileCount();
        for (int i = 0; i < unusedBits; ++i) {
            last = last & ~(1 << (block_size - 1 - i));
        }

        m_tileCount = maxTileCount();
        m_tileCountReady = true;
        return *this;
    }

    FastTileSuperposition& setToNone() {
        fill(m_elements.begin(), m_elements.end(), 0);
        m_tileCount = 0;
        m_tileCountReady = true;
        return *this;
    }

    bool contains(int tileIndex) const
    {
        int block_index = tileIndex / block_size;
        int bit_index = tileIndex % block_size;
        block_t block_mask = 1 << bit_index;
        return (m_elements[block_index] & block_mask) != 0;
    }

    /**
     * Keep in this superposition only the ones that are also contained in `other`
     * Return true if this superposition changed
     */
    bool maskBy(const FastTileSuperposition& other) {
        auto it = m_elements.begin();
        auto end = m_elements.end();
        auto otherIt = other.m_elements.begin();
        bool changed = false;
        for (; it != end; ++it, ++otherIt) {
            block_t block = *it;
            *it = block & (*otherIt);
            changed = changed || (block != *it);
        }
        if (changed) m_tileCountReady = false;
        return changed;
    }

    bool isEmpty() const {
        return tileCount() == 0;
    }

    float measureEntropy() {
        return std::max(0.0f, static_cast<float>(tileCount()) - 1);
    }

    int tileCount() const {
        if (!m_tileCountReady) {
            m_tileCount = 0;
            for (auto block : m_elements) {
                // Should be in <bit> header in C++20 but my msvc is not fully C++20 compliant
                m_tileCount += static_cast<int>(__popcnt(block));
                //m_tileCount += static_cast<int>(std::popcount(block));
            }
            m_tileCountReady = true;
        }
        return m_tileCount;
    }

    int maxTileCount() const {
        return m_maxTileCount;
    }

    template<class Generator>
    void observe(Generator& rand_engine) {
        //DEBUG_LOG << "Observing a superposition of " << tileCount() << " items: " << *this;
        std::uniform_int_distribution dist(0, tileCount() - 1);

        int index = dist(rand_engine);
        //DEBUG_LOG << "Random index is " << index;

        auto it = begin();
        for (int i = 0; i < index; ++i) ++it; // eq of std::advance(it, index);
        int tile = *it;

        setToNone();
        add(tile);
    }

    void add(int tileIndex) {
        int tileIndexBase = tileIndex / block_size;
        int tileIndexFine = tileIndex - block_size * tileIndexBase;
        auto& block = m_elements[tileIndexBase];
        if ((block & (1 << tileIndexFine)) == 0) ++m_tileCount;
        block = block | (1 << tileIndexFine);
    }

    bool add(const FastTileSuperposition& other) {
        auto it = m_elements.begin();
        auto end = m_elements.end();
        auto otherIt = other.m_elements.begin();
        bool changed = false;
        for (; it != end; ++it, ++otherIt) {
            block_t block = *it;
            *it = block | (*otherIt);
            changed = changed || (block != *it);
        }
        if (changed) m_tileCountReady = false;
        return changed;
    }

    bool operator==(const FastTileSuperposition& other) const {
        return other.m_elements == m_elements;
    }

    bool operator!=(const FastTileSuperposition& other) const {
        return !(other == *this);
    }

    friend std::ostream& operator<<(std::ostream& os, const FastTileSuperposition& x) {
        os << "FastTileSuperposition{";
        bool first = true;
        for (int t : x) {
            if (!first) os << ", ";
            os << t;
            first = false;
        }
        os << "}";
        return os;
    }

private:
    int m_maxTileCount;
    std::vector<block_t> m_elements;
    mutable int m_tileCount = 0; // this is redundant with m_elements
    mutable bool m_tileCountReady = false;

    friend class const_iterator;
};

} // namespace libwfc
