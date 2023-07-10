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
#include <glm/glm.hpp>

#include <vector>
#include <array>
#include <memory>
#include <variant>
#include <string>

class Texture;
struct DrawCall;

/**
 * Structures meant to be serialized, and which do not contain any redundancy
 * nor runtime related fields.
 * 
 * In all structs, isValid() is meant to be called at runtime because the
 * struct may intentionaly represent an invalid object, whereas check() is only
 * meant for debug tiem assertions, and must always be true if objects are used
 * correctly.
 */
namespace Model {

namespace Csg2D {

struct Disc;
struct Box;
struct Union;
struct Difference;

using Csg2D = std::variant<Disc, Union, Difference, Box>;

struct Disc {
	glm::vec2 center;
	glm::vec2 size;
	float angle;
};

struct Box {
	glm::vec2 center;
	glm::vec2 size;
	float angle;
};

struct Union {
	std::shared_ptr<Csg2D> lhs;
	std::shared_ptr<Csg2D> rhs;

	Union(const Csg2D& a, const Csg2D& b)
		: lhs(std::make_shared<Csg2D>(a))
		, rhs(std::make_shared<Csg2D>(b))
	{}

	Union()
		: lhs(nullptr)
		, rhs(nullptr)
	{}
};

struct Difference {
	std::shared_ptr<Csg2D> lhs;
	std::shared_ptr<Csg2D> rhs;

	Difference(const Csg2D& a, const Csg2D& b)
		: lhs(std::make_shared<Csg2D>(a))
		, rhs(std::make_shared<Csg2D>(b))
	{}

	Difference()
		: lhs(nullptr)
		, rhs(nullptr)
	{}
};

} // namespace Csg2D

/**
 * A shape lies on an edge type. It is described by a 2D CSG tree defined over [0,1]².
 * This is also known as a 'Profile'
 */
struct Shape {
	Csg2D::Csg2D csg;
	glm::vec4 color;
	float roughness;
	float metallic;
};

/**
 * An edge type can be shared by multiple tiles.
 * It contains non overlapping shapes, sorted by increasing start.
 */
struct EdgeType {
	/// NB: faces are sorted by their 'start' field
	std::vector<Shape> shapes;

	/**
	 * An edge type is explicit when it has been manually created by the user.
	 * Implicit edges are created when adding a new tile type
	 * Only explicit edges are shown in color.
	 */
	bool isExplicit = false;

	/**
	 * If true, allow this edge to be used at a border and left unconnected.
	 */
	bool borderEdge = true;

	/**
	 * If true, prevents this edge from being used anywhere but at borders.
	 */
	bool borderOnly = false;

	glm::vec3 color = glm::vec3(0.0f);
};
typedef std::shared_ptr<EdgeType> EdgeTypePtr;
typedef std::weak_ptr<EdgeType> EdgeTypeWPtr;

/**
 * An edge type can be associated to an edge of a tile type either
 * in a standard way or mirrored.
 */
struct EdgeTransform {
	bool flipped = false;

	glm::vec3 transformed(const glm::vec3& vec) const {
		return flipped ? glm::vec3(-vec.x, vec.y, vec.z) : vec;
	}
};

struct TransformedEdge {
	EdgeTypePtr edge;
	EdgeTransform transform;
};

/**
 * Describes the connection between a tile type's edge and an edge type
 */
struct TileEdge {
	/**
	 * A scopped enum-like but that can be used as index w/o casting
	 */
	constexpr static int PosX = 0;
	constexpr static int PosY = 1;
	constexpr static int NegX = 2;
	constexpr static int NegY = 3;
	constexpr static int PosZ = 4;
	constexpr static int NegZ = 5;
	constexpr static int Count = 6;
	constexpr static int SolveCount = 4; // Edges PosZ and NegZ are not used for solving

	enum class Direction {
		PosX = 0,
		PosY = 1,
		NegX = 2,
		NegY = 3,
		PosZ = 4,
		NegZ = 5,
	};
	struct TransformedDirection {
		Direction direction;
		EdgeTransform transform;
	};

	static const std::array<glm::mat4, Count> Transforms;

	EdgeTypeWPtr type;
	EdgeTransform transform;

	bool isValid() const { return type.lock() != nullptr; }
};

/**
 * An inner path connects two shapes of adjacent edges
 */
struct TileInnerPath {
	int tileEdgeFrom;
	int shapeFrom;
	int tileEdgeTo;
	int shapeTo;

	bool addHalfTurn = false;
	int assignmentOffset = 0;
	bool flipEnd = false;
};

struct TileTransformPermission {
	bool rotation = true;
	bool flipX = true;
	bool flipY = true;
};

enum class TileOrientation {
	Deg0,
	Deg90,
	Deg180,
	Deg270,
};

struct TileTransform {
	bool flipX = false;
	bool flipY = false;
	TileOrientation orientation = TileOrientation::Deg0;
};

struct TransformedTile {
	int tileIndex;
	TileTransform transform;
};

struct TileInnerGeometry {
	std::string objFilename;
	glm::mat4 transform = glm::mat4(1.0);
	glm::vec4 color;
	float roughness;
	float metallic;
};

struct TileType {
	std::array<TileEdge, TileEdge::Count> edges;

	// The geometry of the tile's content is split between the part that touches
	// interfaces (innerPaths) and the one that does not touch any (innerGeometry)
	std::vector<TileInnerPath> innerPaths;
	std::vector<TileInnerGeometry> innerGeometry;

	// Allow the tile to be transformed when used by the solver
	TileTransformPermission transformPermission;

	// Do not use for solving (this is used to prevent a tile from being suggested by the solver)
	bool ignore;

	// Not Serialized
	std::shared_ptr<Texture> render;
};
typedef std::shared_ptr<TileType> TileTypePtr;
typedef std::weak_ptr<TileType> TileTypeWPtr;

/**
 * The root struct
 */
struct Tileset {
	std::vector<EdgeTypePtr> edgeTypes;
	std::vector<TileTypePtr> tileTypes;

	TileTransformPermission defaultTileTransformPermission;
};
typedef std::shared_ptr<Tileset> TilesetPtr;
typedef std::weak_ptr<Tileset> TilesetWPtr;

} // namespace Model
