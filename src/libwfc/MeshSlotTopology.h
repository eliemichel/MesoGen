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

#include "SlotTopology.h"

#include <optional>
#include <vector>
#include <cassert>

namespace libwfc {

enum class MeshRelation {
    Neighbor0,
    Neighbor1,
    Neighbor2,
    Neighbor3,
};

/**
 * A slot topology derived from a 3D mesh, where each face is a slot.
 */
class MeshSlotTopology : public AbstractSlotTopology<int, MeshRelation> {
public:
    using SlotIndex = int;
    using RelationEnum = MeshRelation;

    struct Face {
        int index;
        std::vector<int> neighbors;
        std::vector<RelationEnum> neighborRelations;
    };

public:
    MeshSlotTopology(const std::vector<Face>& faces)
        : m_faces(faces)
    {}

    std::optional<std::pair<SlotIndex, RelationEnum>> neighborOf(const SlotIndex& slotIndex, RelationEnum relation) const override {
        assert(slotIndex >= 0);
        assert(slotIndex < slotCount());

        const auto& face = m_faces[slotIndex];
        int relationIndex = static_cast<int>(relation);
        if (relationIndex >= face.neighbors.size()) {
            return {};
        }

        int neighborIndex = face.neighbors[relationIndex];

        if (neighborIndex == -1) {
            return {};
        }

        return std::make_pair(neighborIndex, face.neighborRelations[relationIndex]);
    }

    int slotCount() const override { return static_cast<int>(m_faces.size()); }

    // only for testing!
    const std::vector<Face>& faces() const { return m_faces; }

private:
    std::vector<Face> m_faces;
};

}
