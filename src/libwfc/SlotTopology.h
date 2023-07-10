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

namespace libwfc {

/**
 * Defines the way slots are related to each others
 */
template <typename SlotIndex, typename RelationEnum>
class AbstractSlotTopology {
public:
    /**
     * Return the slot index that is in relation `relation` with the given `slotIndex`.
     * Tis slot index is itself in relation `return->second` with `slotIndex`.
     * It is assumed that there is at most one such slot.
     */
    virtual std::optional<std::pair<SlotIndex, RelationEnum>> neighborOf(const SlotIndex& slotIndex, RelationEnum relation) const = 0;

    /**
     * Total number of slots
     */
    virtual int slotCount() const = 0;
};

enum class GridRelation {
    PosX,
    PosY,
    NegX,
    NegY,
};

/**
 * The simplest topology for connecting slots is a regular grid
 */
template <typename SlotIndex = int>
class GridSlotTopology : public AbstractSlotTopology<SlotIndex, GridRelation> {
public:
    GridSlotTopology(int width, int height)
        : m_width(width)
        , m_height(height)
    {
        static_assert(magic_enum::enum_count<GridRelation>() == 4, "There must be 4 types of slot relations when using a 2D grid topology");
        assert(m_width > 0);
        assert(m_height > 0);
    }

    SlotIndex index(int x, int y) const {
        return x + y * m_width;
    }

    void fromIndex(SlotIndex slotIndex, int& x, int& y) const {
        x = slotIndex % m_width;
        y = slotIndex / m_width;
    }

    std::optional<std::pair<SlotIndex, GridRelation>> neighborOf(const SlotIndex& slotIndex, GridRelation grelation) const override {
        int x, y;
        fromIndex(slotIndex, x, y);
        GridRelation dualRelation;

        switch (grelation) {
        case GridRelation::PosX:
            dualRelation = GridRelation::NegX;
            ++x;
            if (x >= m_width) return {};
            break;
        case GridRelation::PosY:
            dualRelation = GridRelation::NegY;
            ++y;
            if (y >= m_height) return {};
            break;
        case GridRelation::NegX:
            dualRelation = GridRelation::PosX;
            --x;
            if (x < 0) return {};
            break;
        case GridRelation::NegY:
            dualRelation = GridRelation::PosY;
            --y;
            if (y < 0) return {};
            break;
        }

        return std::make_pair(index(x, y), dualRelation);
    }

    int slotCount() const override {
        return m_width * m_height;
    }

private:
    int m_width;
    int m_height;
};

} // namespace libwfc
