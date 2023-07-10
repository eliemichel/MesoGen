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

#include "Serialization.h"
#include "Model.h"

namespace Serialization {

// Per type implementation:

struct TilesetSerializationContext {
	std::map<Model::EdgeTypePtr, int> edgeLut;
};

struct TilesetDeserializationContext {
	std::vector<Model::EdgeTypePtr>& edgeTypes;
};

// Csg2D::Disc

template <>
void serialize(const Model::Csg2D::Disc& disc, Document& document, Value& json);

template <>
void deserialize(Model::Csg2D::Disc& disc, const Document& document, const Value& json);

// Csg2D::Union

template <>
void serialize(const Model::Csg2D::Union& unionOp, Document& document, Value& json);

template <>
void deserialize(Model::Csg2D::Union& unionOp, const Document& document, const Value& json);

// Csg2D::Difference

template <>
void serialize(const Model::Csg2D::Difference& differenceOp, Document& document, Value& json);

template <>
void deserialize(Model::Csg2D::Difference& differenceOp, const Document& document, const Value& json);

// Csg2D

template <>
void serialize(const Model::Csg2D::Csg2D& csg, Document& document, Value& json);

template <>
void deserialize(Model::Csg2D::Csg2D& csg, const Document& document, const Value& json);

// Shape

template <>
void serialize(const Model::Shape& shape, Document& document, Value& json);

template <>
void deserialize(Model::Shape& shape, const Document& document, const Value& json);

// TileInnerPath

template <>
void serialize(const Model::TileInnerPath& path, Document& document, Value& json);

template <>
void deserialize(Model::TileInnerPath& path, const Document& document, const Value& json);

// TileInnerGeometry

template <>
void serialize(const Model::TileInnerGeometry& geo, Document& document, Value& json);

template <>
void deserialize(Model::TileInnerGeometry& geo, const Document& document, const Value& json);

// EdgeType

template <>
void serialize(const Model::EdgeType& edgeType, Document& document, Value& json);

template <>
void deserialize(Model::EdgeType& edgeType, const Document& document, const Value& json);

// EdgeTransform

template <>
void serialize(const Model::EdgeTransform& edgeTransform, Document& document, Value& json);

template <>
void deserialize(Model::EdgeTransform& edgeTransform, const Document& document, const Value& json);

// TileEdge

template <>
void serialize(const Model::TileEdge& tileEdge, Document& document, Value& json, TilesetSerializationContext& context);

template <>
void deserialize(Model::TileEdge& tileEdge, const Document& document, const Value& json, TilesetDeserializationContext& context);

// TileTransformPermission

template <>
void serialize(const Model::TileTransformPermission& permission, Document& document, Value& json);

template <>
void deserialize(Model::TileTransformPermission& permission, const Document& document, const Value& json);

// TileType

template <>
void serialize(const Model::TileType& tileType, Document& document, Value& json, TilesetSerializationContext& context);

template <>
void deserialize(Model::TileType& tileType, const Document& document, const Value& json, TilesetDeserializationContext& context);

// Tileset

template <>
void serialize(const Model::Tileset& tileset, Document& document, Value& json);

template <>
void deserialize(Model::Tileset& tileset, const Document& document, const Value& json);

}
