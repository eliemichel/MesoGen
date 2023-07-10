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
#include "Serialization.h"

namespace Serialization {

// Csg2D::Disc

template <>
void serialize(const Model::Csg2D::Disc& disc, Document& document, Value& json) {
	json.SetObject();
	auto& allocator = document.GetAllocator();

	Value centerJson;
	serialize(disc.center, document, centerJson);
	json.AddMember("center", centerJson, allocator);

	Value sizeJson;
	serialize(disc.size, document, sizeJson);
	json.AddMember("size", sizeJson, allocator);

	json.AddMember("angle", disc.angle, allocator);
}

template <>
void deserialize(Model::Csg2D::Disc& disc, const Document& document, const Value& json) {
	DESER_ASSERT(json, Object);

	if (json.HasMember("angle")) {
		disc.angle = json["angle"].GetFloat();
	} else {
		disc.angle = 0;
	}

	DESER_ASSERT_HAS_MEMBER(json, "center");
	deserialize(disc.center, document, json["center"]);

	if (json.HasMember("size")) {
		deserialize(disc.size, document, json["size"]);
	}
	else if (json.HasMember("radius")) {
		float radius = json["radius"].GetFloat();
		disc.size = glm::vec2(radius, radius);
	}
}

// Csg2D::Box

template <>
void serialize(const Model::Csg2D::Box& box, Document& document, Value& json) {
	json.SetObject();
	auto& allocator = document.GetAllocator();

	Value centerJson;
	serialize(box.center, document, centerJson);
	json.AddMember("center", centerJson, allocator);

	Value sizeJson;
	serialize(box.size, document, sizeJson);
	json.AddMember("size", sizeJson, allocator);

	json.AddMember("angle", box.angle, allocator);
}

template <>
void deserialize(Model::Csg2D::Box& box, const Document& document, const Value& json) {
	DESER_ASSERT(json, Object);

	if (json.HasMember("angle")) {
		DESER_ASSERT(json["angle"], Float);
		box.angle = json["angle"].GetFloat();
	}
	else {
		box.angle = 0;
	}

	DESER_ASSERT_HAS_MEMBER(json, "center");
	deserialize(box.center, document, json["center"]);

	DESER_ASSERT_HAS_MEMBER(json, "size");
	deserialize(box.size, document, json["size"]);
}

// Csg2D::Union

template <>
void serialize(const Model::Csg2D::Union& unionOp, Document& document, Value& json) {
	json.SetObject();
	auto& allocator = document.GetAllocator();

	Value lhsJson, rhsJson;
	serialize(unionOp.lhs, document, lhsJson);
	json.AddMember("lhs", lhsJson, allocator);
	serialize(unionOp.rhs, document, rhsJson);
	json.AddMember("rhs", rhsJson, allocator);
}

template <>
void deserialize(Model::Csg2D::Union& unionOp, const Document& document, const Value& json) {
	DESER_ASSERT(json, Object);

	DESER_ASSERT_HAS_MEMBER(json, "lhs");
	deserialize(unionOp.lhs, document, json["lhs"]);

	DESER_ASSERT_HAS_MEMBER(json, "rhs");
	deserialize(unionOp.rhs, document, json["rhs"]);
}

// Csg2D::Difference

template <>
void serialize(const Model::Csg2D::Difference& differenceOp, Document& document, Value& json) {
	json.SetObject();
	auto& allocator = document.GetAllocator();

	Value lhsJson, rhsJson;
	serialize(differenceOp.lhs, document, lhsJson);
	json.AddMember("lhs", lhsJson, allocator);
	serialize(differenceOp.rhs, document, rhsJson);
	json.AddMember("rhs", rhsJson, allocator);
}

template <>
void deserialize(Model::Csg2D::Difference& differenceOp, const Document& document, const Value& json) {
	DESER_ASSERT(json, Object);

	DESER_ASSERT_HAS_MEMBER(json, "lhs");
	deserialize(differenceOp.lhs, document, json["lhs"]);

	DESER_ASSERT_HAS_MEMBER(json, "rhs");
	deserialize(differenceOp.rhs, document, json["rhs"]);
}

// Csg2D

template <>
void serialize(const Model::Csg2D::Csg2D& csg, Document& document, Value& json) {
	json.SetObject();
	auto& allocator = document.GetAllocator();

	if (auto* disc = std::get_if<Model::Csg2D::Disc>(&csg)) {
		serialize(*disc, document, json);
	}
	else if (auto* box = std::get_if<Model::Csg2D::Box>(&csg)) {
		serialize(*box, document, json);
	}
	else if (auto* unionOp = std::get_if<Model::Csg2D::Union>(&csg)) {
		serialize(*unionOp, document, json);
	}
	else if (auto* differenceOp = std::get_if<Model::Csg2D::Difference>(&csg)) {
		serialize(*differenceOp, document, json);
	}
	else {
		throw SerializationError("Variant not supported for Csg2D: " + std::to_string(csg.index()));
	}
	json.AddMember("node-type", csg.index(), allocator);
}

template <>
void deserialize(Model::Csg2D::Csg2D& csg, const Document& document, const Value& json) {
	DESER_ASSERT(json, Object);
	DESER_ASSERT_HAS_MEMBER(json, "node-type");
	DESER_ASSERT(json["node-type"], Number);

	int index = json["node-type"].GetInt();
	switch (index) {
	case 0:
	{
		Model::Csg2D::Disc disc;
		deserialize(disc, document, json);
		csg = disc;
		break;
	}
	case 3:
	{
		Model::Csg2D::Box box;
		deserialize(box, document, json);
		csg = box;
		break;
	}
	case 1:
	{
		Model::Csg2D::Union unionOp;
		deserialize(unionOp, document, json);
		csg = unionOp;
		break;
	}
	case 2:
	{
		Model::Csg2D::Difference differenceOp;
		deserialize(differenceOp, document, json);
		csg = differenceOp;
		break;
	}
	default:
		throw SerializationError("Invalid node type for Csg2D: " + std::to_string(index));
	}
}

// Shape

template <>
void serialize(const Model::Shape& shape, Document& document, Value& json) {
	json.SetObject();
	auto& allocator = document.GetAllocator();

	Value csgJson;
	serialize(shape.csg, document, csgJson);
	json.AddMember("csg", csgJson, allocator);

	Value colorJson;
	serialize(shape.color, document, colorJson);
	json.AddMember("color", colorJson, allocator);

	json.AddMember("roughness", shape.roughness, allocator);
	json.AddMember("metallic", shape.metallic, allocator);
}

template <>
void deserialize(Model::Shape& shape, const Document& document, const Value& json) {
	DESER_ASSERT(json, Object);
	DESER_ASSERT_HAS_MEMBER(json, "csg");
	deserialize(shape.csg, document, json["csg"]);

	if (json.HasMember("color")) {
		deserialize(shape.color, document, json["color"]);
	}
	else {
		shape.color = glm::vec4(1.0f);
	}

	shape.roughness = json.HasMember("roughness") ? json["roughness"].GetFloat() : 1.0f;
	shape.metallic = json.HasMember("metallic") ? json["metallic"].GetFloat() : 0.0f;
}

// TileInnerPath

template <>
void serialize(const Model::TileInnerPath& path, Document& document, Value& json) {
	json.SetObject();
	auto& allocator = document.GetAllocator();

	json.AddMember("shapeFrom", path.shapeFrom, allocator);
	json.AddMember("shapeTo", path.shapeTo, allocator);
	json.AddMember("tileEdgeFrom", path.tileEdgeFrom, allocator);
	json.AddMember("tileEdgeTo", path.tileEdgeTo, allocator);

	json.AddMember("addHalfTurn", path.addHalfTurn, allocator);
	json.AddMember("assignmentOffset", path.assignmentOffset, allocator);
	json.AddMember("flipEnd", path.flipEnd, allocator);
}

template <>
void deserialize(Model::TileInnerPath& path, const Document& document, const Value& json) {
	DESER_ASSERT(json, Object);
	DESER_ASSERT_HAS_MEMBER(json, "shapeFrom");
	DESER_ASSERT_HAS_MEMBER(json, "shapeTo");
	DESER_ASSERT_HAS_MEMBER(json, "tileEdgeFrom");
	DESER_ASSERT_HAS_MEMBER(json, "tileEdgeTo");

	DESER_ASSERT(json["shapeFrom"], Int);
	DESER_ASSERT(json["shapeTo"], Int);
	DESER_ASSERT(json["tileEdgeFrom"], Int);
	DESER_ASSERT(json["tileEdgeTo"], Int);

	path.shapeFrom = json["shapeFrom"].GetInt();
	path.shapeTo = json["shapeTo"].GetInt();
	path.tileEdgeFrom = json["tileEdgeFrom"].GetInt();
	path.tileEdgeTo = json["tileEdgeTo"].GetInt();

	if (json.HasMember("addHalfTurn")) {
		DESER_ASSERT(json["addHalfTurn"], Bool);
		path.addHalfTurn = json["addHalfTurn"].GetBool();
	}
	if (json.HasMember("assignmentOffset")) {
		DESER_ASSERT(json["assignmentOffset"], Int);
		path.assignmentOffset = json["assignmentOffset"].GetInt();
	}
	if (json.HasMember("flipEnd")) {
		DESER_ASSERT(json["flipEnd"], Bool);
		path.flipEnd = json["flipEnd"].GetBool();
	}
}

// TileInnerGeometry

template <>
void serialize(const Model::TileInnerGeometry& geo, Document& document, Value& json) {
	json.SetObject();
	auto& allocator = document.GetAllocator();
	
	json.AddMember("objFilename", geo.objFilename, allocator);
	AddMember("transform", geo.transform, document, json);

	Value colorJson;
	serialize(geo.color, document, colorJson);
	json.AddMember("color", colorJson, allocator);

	json.AddMember("roughness", geo.roughness, allocator);
	json.AddMember("metallic", geo.metallic, allocator);
}

template <>
void deserialize(Model::TileInnerGeometry& geo, const Document& document, const Value& json) {
	DESER_ASSERT(json, Object);
	DESER_ASSERT_HAS_MEMBER(json, "objFilename");
	DESER_ASSERT_HAS_MEMBER(json, "transform");

	DESER_ASSERT(json["objFilename"], String);
	DESER_ASSERT(json["transform"], Array);

	geo.objFilename = json["objFilename"].GetString();
	deserialize(geo.transform, document, json["transform"]);

	if (json.HasMember("color")) {
		deserialize(geo.color, document, json["color"]);
	}
	else {
		geo.color = glm::vec4(1.0f);
	}

	geo.roughness = json.HasMember("roughness") ? json["roughness"].GetFloat() : 1.0f;
	geo.metallic = json.HasMember("metallic") ? json["metallic"].GetFloat() : 0.0f;
}

// EdgeType

template <>
void serialize(const Model::EdgeType& edgeType, Document& document, Value& json) {
	json.SetObject();
	auto& allocator = document.GetAllocator();

	AddArrayMember("shapes", edgeType.shapes, document, json);
	json.AddMember("isExplicit", edgeType.isExplicit, allocator);
	json.AddMember("borderEdge", edgeType.borderEdge, allocator);
	json.AddMember("borderOnly", edgeType.borderOnly, allocator);
	AddMember("color", edgeType.color, document, json);
}

template <>
void deserialize(Model::EdgeType& edgeType, const Document& document, const Value& json) {
	DESER_ASSERT(json, Object);
	
	DESER_ASSERT_HAS_MEMBER(json, "isExplicit");
	DESER_ASSERT(json["isExplicit"], Bool);
	edgeType.isExplicit = json["isExplicit"].GetBool();

	if (json.HasMember("borderEdge")) {
		DESER_ASSERT(json["borderEdge"], Bool);
		edgeType.borderEdge = json["borderEdge"].GetBool();
	} else {
		edgeType.borderEdge = true;
	}

	if (json.HasMember("borderOnly")) {
		DESER_ASSERT(json["borderOnly"], Bool);
		edgeType.borderOnly = json["borderOnly"].GetBool();
	}
	else {
		edgeType.borderOnly = false;
	}

	GetArrayMember("shapes", edgeType.shapes, document, json);

	DESER_ASSERT_HAS_MEMBER(json, "color");
	deserialize(edgeType.color, document, json["color"]);
}

// EdgeTransform

template <>
void serialize(const Model::EdgeTransform& edgeTransform, Document& document, Value& json) {
	json.SetObject();
	auto& allocator = document.GetAllocator();
	json.AddMember("flipped", edgeTransform.flipped, allocator);
}

template <>
void deserialize(Model::EdgeTransform& edgeTransform, const Document& document, const Value& json) {
	DESER_ASSERT(json, Object);
	DESER_ASSERT_HAS_MEMBER(json, "flipped");
	DESER_ASSERT(json["flipped"], Bool);

	edgeTransform.flipped = json["flipped"].GetBool();
}

// TileEdge

template <>
void serialize(const Model::TileEdge& tileEdge, Document& document, Value& json, TilesetSerializationContext& context) {
	json.SetObject();
	AddMember("transform", tileEdge.transform, document, json);

	int tileEdgeIndex = -1;
	if (auto edgeType = tileEdge.type.lock()) {
		tileEdgeIndex = context.edgeLut[edgeType];
	}

	json.AddMember("tileEdgeIndex", tileEdgeIndex, document.GetAllocator());
}

template <>
void deserialize(Model::TileEdge& tileEdge, const Document& document, const Value& json, TilesetDeserializationContext& context) {
	DESER_ASSERT(json, Object);
	DESER_ASSERT_HAS_MEMBER(json, "transform");
	DESER_ASSERT_HAS_MEMBER(json, "tileEdgeIndex");

	deserialize(tileEdge.transform, document, json["transform"]);

	DESER_ASSERT(json["tileEdgeIndex"], Int);
	int tileEdgeIndex = json["tileEdgeIndex"].GetInt();
	if (tileEdgeIndex >= 0 && tileEdgeIndex < context.edgeTypes.size()) {
		tileEdge.type = context.edgeTypes[tileEdgeIndex];
	}
	else {
		tileEdge.type.reset();
	}
}

// TileTransformPermission

template <>
void serialize(const Model::TileTransformPermission& permission, Document& document, Value& json) {
	auto& allocator = document.GetAllocator();
	json.SetObject();
	json.AddMember("rotation", permission.rotation, allocator);
	json.AddMember("flipX", permission.flipX, allocator);
	json.AddMember("flipY", permission.flipY, allocator);
}

template <>
void deserialize(Model::TileTransformPermission& permission, const Document& document, const Value& json) {
	DESER_ASSERT(json, Object);

	DESER_ASSERT_HAS_MEMBER(json, "rotation");
	DESER_ASSERT(json["rotation"], Bool);
	permission.rotation = json["rotation"].GetBool();

	DESER_ASSERT_HAS_MEMBER(json, "flipX");
	DESER_ASSERT(json["flipX"], Bool);
	permission.flipX = json["flipX"].GetBool();

	DESER_ASSERT_HAS_MEMBER(json, "flipY");
	DESER_ASSERT(json["flipY"], Bool);
	permission.flipY = json["flipY"].GetBool();
}

// TileType

template <>
void serialize(const Model::TileType& tileType, Document& document, Value& json, TilesetSerializationContext& context) {
	auto& allocator = document.GetAllocator();

	json.SetObject();
	AddArrayMember("innerPaths", tileType.innerPaths, document, json);
	AddArrayMember("innerGeometry", tileType.innerGeometry, document, json);
	AddArrayMember("edges", tileType.edges, document, json, context);

	Value transformPermission;
	serialize(tileType.transformPermission, document, transformPermission);
	json.AddMember("transformPermission", transformPermission, allocator);

	json.AddMember("ignore", tileType.ignore, allocator);
}

template <>
void deserialize(Model::TileType& tileType, const Document& document, const Value& json, TilesetDeserializationContext& context) {
	DESER_ASSERT(json, Object);
	GetArrayMember("innerPaths", tileType.innerPaths, document, json);
	if (json.HasMember("innerGeometry")) {
		GetArrayMember("innerGeometry", tileType.innerGeometry, document, json, context);
	}
	GetArrayMember("edges", tileType.edges, document, json, context, true);

	if (json.HasMember("transformPermission")) {
		deserialize(tileType.transformPermission, document, json["transformPermission"]);
	}
	else {
		WARN_LOG << "Key not found: 'transformPermission'";
	}

	if (json.HasMember("ignore")) {
		DESER_ASSERT(json["ignore"], Bool);
		tileType.ignore = json["ignore"].GetBool();
	}
	else {
		tileType.ignore = false;
	}
}

// Tileset

template <>
void serialize(const Model::Tileset& tileset, Document& document, Value& json) {
	auto& allocator = document.GetAllocator();

	json.SetObject();

	TilesetSerializationContext context;
	int i = 0;
	for (const auto& edgeType : tileset.edgeTypes) {
		context.edgeLut[edgeType] = i++;
	}

	AddArrayMember("tileTypes", tileset.tileTypes, document, json, context);
	AddArrayMember("edgeTypes", tileset.edgeTypes, document, json);

	Value v;
	serialize(tileset.defaultTileTransformPermission, document, v);
	json.AddMember("defaultTileTransformPermission", v, allocator);
}

template <>
void deserialize(Model::Tileset& tileset, const Document& document, const Value& json) {
	DESER_ASSERT(json, Object);

	TilesetDeserializationContext context{ tileset.edgeTypes };

	GetArrayMember("edgeTypes", tileset.edgeTypes, document, json);
	GetArrayMember("tileTypes", tileset.tileTypes, document, json, context);

	if (json.HasMember("defaultTileTransformPermission")) {
		deserialize(tileset.defaultTileTransformPermission, document, json["defaultTileTransformPermission"]);
	}
	else {
		WARN_LOG << "Key not found: 'defaultTileTransformPermission'";
	}
}

} // namespace Serialization
