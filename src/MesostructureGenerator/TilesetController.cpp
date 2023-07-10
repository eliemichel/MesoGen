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
#include "TilesetController.h"
#include "Bezier.h"
#include "CsgEngine.h"
#include "MesostructureData.h"
#include "CsgCompiler.h"

#include <cassert>
#include <set>
#include <map>
#include <variant>

using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat2;
using glm::mat3;
using glm::mat4;

using Model::Shape;
using Model::TileEdge;
using Model::Tileset;
using Model::EdgeType;
using Model::TileType;
using Model::TileInnerPath;
using Model::TransformedEdge;
using Model::TileTransform;
using Model::TileOrientation;

const std::vector<vec3> TilesetController::DefaultEdgeColors = {
	vec3(0.9764705882352941, 0.2549019607843137, 0.26666666666666666),
	vec3(0.9725490196078431, 0.5882352941176471, 0.11764705882352941),
	vec3(0.9764705882352941, 0.7803921568627451, 0.30980392156862746),
	vec3(0.5647058823529412, 0.7450980392156863, 0.42745098039215684),
	vec3(0.2627450980392157, 0.6666666666666666, 0.5450980392156862),
	vec3(0.3411764705882353, 0.4588235294117647, 0.5647058823529412),
	vec3(0.4666666666666667, 0.3254901960784314, 0.6352941176470588),
};

void TilesetController::setModel(std::weak_ptr<Model::Tileset> model) {
	m_model = model;
}

void TilesetController::setMesostructureData(std::weak_ptr<MesostructureData> mesostructureData) {
	m_mesostructureData = mesostructureData;
}

void TilesetController::setDirty(int tileIndex) {
	m_hasChangedLately = true;
	if (auto mesostructureData = m_mesostructureData.lock()) {
		if (tileIndex < 0) {
			mesostructureData->tileListVersion.increment();
			int tileCount = 0;
			if (auto model = m_model.lock()) tileCount = static_cast<int>(model->tileTypes.size());
			mesostructureData->tileVersions.resize(tileCount);
			for (auto & v : mesostructureData->tileVersions) {
				v.increment();
			}
			mesostructureData->slotAssignmentsVersion.increment();
		}
		else {
			mesostructureData->tileVersions[tileIndex].increment();
		}
	}
}

//-----------------------------------------------------------------------------
// ShapeAccessor

Shape TilesetController::ShapeAccessor::invalidShape;
TileEdge TilesetController::ShapeAccessor::invalidTileEdge;

TilesetController::ShapeAccessor::ShapeAccessor()
	: m_isValid(true)
	, m_shape(invalidShape)
	, m_tileEdge(invalidTileEdge)
	, m_transform(TileEdge::Transforms[0])
	, m_tileEdgeIndex(0)
	, m_shapeIndex(0)
{}

TilesetController::ShapeAccessor::ShapeAccessor(Shape& shape, const TileEdge& tileEdge, const mat4& transform, int tileEdgeIndex, int shapeIndex)
	: m_isValid(true)
	, m_shape(shape)
	, m_tileEdge(tileEdge)
	, m_transform(transform)
	, m_tileEdgeIndex(tileEdgeIndex)
	, m_shapeIndex(shapeIndex)
{}

const Shape& TilesetController::ShapeAccessor::shape() const {
	assert(isValid());
	return m_shape;
}

bool TilesetController::ShapeAccessor::isValid() const {
	return m_isValid;
}

//-----------------------------------------------------------------------------
// Create

void TilesetController::addTileType() {
	if (auto model = m_model.lock())
	{
		while (model->edgeTypes.size() < 2) {
			addEdgeType();
		}

		auto tileType = std::make_shared<Model::TileType>();
		for (int k = 0; k < Model::TileEdge::Count; ++k) {
			if (k < 4) {
				tileType->edges[k].type = model->edgeTypes[k % 2];
			}
			tileType->edges[k].transform.flipped = k >= 2;
		}
		tileType->transformPermission = model->defaultTileTransformPermission;
		model->tileTypes.push_back(tileType);

		setDirty();
	}
}

void TilesetController::addEdgeType() {
	if (auto model = m_model.lock())
	{
		auto type = std::make_shared<Model::EdgeType>();
		type->isExplicit = true;
		size_t colorIndex = model->edgeTypes.size() % DefaultEdgeColors.size();
		type->color = DefaultEdgeColors[colorIndex];
		model->edgeTypes.push_back(type);

		setDirty();
	}
}

void TilesetController::addTileInnerPath(TileType& tileType, const TileInnerPath& path) {
	if (hasTileInnerPath(tileType, path)) return;

	tileType.innerPaths.push_back(path);
	tileType.innerPaths.back().assignmentOffset = guessAssignmentOffset(tileType, path);

	setDirty(tileTypeIndex(tileType));
}

void TilesetController::addTileInnerGeometry(Model::TileType& tileType) {
	tileType.innerGeometry.push_back({
		"",
		transpose(glm::mat4(
			1.0, 0.0, 0.0, 0.5,
			0.0, 1.0, 0.0, 0.5,
			0.0, 0.0, 1.0, 0.0,
			0.0, 0.0, 0.0, 1.0
		)),
		glm::vec4(1.0),
		1.0,
		0.0
	});
	setDirty(tileTypeIndex(tileType));
}

void TilesetController::copyTileType(TileType& tileType) {
	if (auto model = m_model.lock()) {
		auto newTileType = std::make_shared<Model::TileType>();
		for (int k = 0; k < Model::TileEdge::Count; ++k) {
			newTileType->edges[k] = tileType.edges[k];
		}
		newTileType->innerPaths = tileType.innerPaths;
		model->tileTypes.push_back(newTileType);

		setDirty();
	}
}

//-----------------------------------------------------------------------------
// Read

int TilesetController::tileTypeIndex(const TileType& tileType) const {
	if (auto model = m_model.lock()) {
		for (int i = 0; i < model->tileTypes.size(); ++i) {
			if (model->tileTypes[i].get() == &tileType) {
				return i;
			}
		}
	}
	return -1;
}

bool TilesetController::hasTileInnerPath(const TileType& tileType, const TileInnerPath& path) {
	for (const auto& otherPath : tileType.innerPaths) {
		bool isSame = (
			otherPath.shapeFrom == path.shapeFrom
			&& otherPath.shapeTo == path.shapeTo
			&& otherPath.tileEdgeFrom == path.tileEdgeFrom
			&& otherPath.tileEdgeTo == path.tileEdgeTo
		);
		bool isOpposite = (
			otherPath.shapeFrom == path.shapeTo
			&& otherPath.shapeTo == path.shapeFrom
			&& otherPath.tileEdgeFrom == path.tileEdgeTo
			&& otherPath.tileEdgeTo == path.tileEdgeFrom
		);
		if (isSame || isOpposite) return true;
	}
	return false;
}

std::vector<TilesetController::ShapeAccessor> TilesetController::GetTileShapes(const Model::TileType& tileType) {
	std::vector<ShapeAccessor> shapeAccessors;
	for (int tileEdgeIndex = 0; tileEdgeIndex < Model::TileEdge::Count; ++tileEdgeIndex) {
		auto& tileEdge = tileType.edges[tileEdgeIndex];
		auto edgeType = tileEdge.type.lock();
		if (!edgeType) continue;
		const mat4& tr = TileEdge::Transforms[tileEdgeIndex];
		for (int shapeIndex = 0; shapeIndex < edgeType->shapes.size(); ++shapeIndex) {
			shapeAccessors.push_back(ShapeAccessor(edgeType->shapes[shapeIndex], tileEdge, tr, tileEdgeIndex, shapeIndex));
		}
	}
	return shapeAccessors;
}

TilesetController::ShapeAccessor TilesetController::getShapeAccessor(
	const TileType& tileType,
	int tileEdgeIndex,
	int shapeIndex
) {
	const auto& tileEdge = tileType.edges[tileEdgeIndex];
	auto edgeType = tileEdge.type.lock();
	const auto& tr = TileEdge::Transforms[tileEdgeIndex];

	return
		edgeType && shapeIndex >= 0 && shapeIndex < edgeType->shapes.size()
		? ShapeAccessor(edgeType->shapes[shapeIndex], tileEdge, tr, tileEdgeIndex, shapeIndex)
		: ShapeAccessor();
}

std::vector<TilesetController::ShapeAccessor> TilesetController::GetTileCaps(const TileType& tileType) {

	std::vector<TilesetController::ShapeAccessor> caps;
	std::set<std::pair<int, int>> usedPairs;

	for (const auto& path : tileType.innerPaths) {
		usedPairs.insert(std::make_pair(path.tileEdgeFrom, path.shapeFrom));
		usedPairs.insert(std::make_pair(path.tileEdgeTo, path.shapeTo));
	}

	for (int tileEdgeIndex = 0; tileEdgeIndex < Model::TileEdge::Count; ++tileEdgeIndex) {
		const auto& tileEdge = tileType.edges[tileEdgeIndex];
		const auto& tr = Model::TileEdge::Transforms[tileEdgeIndex];
		if (auto edgeType = tileEdge.type.lock()) {
			for (int shapeIndex = 0; shapeIndex < edgeType->shapes.size(); ++shapeIndex) {
				auto pair = std::make_pair(tileEdgeIndex, shapeIndex);
				if (usedPairs.find(pair) == usedPairs.end()) {
					auto& shape = edgeType->shapes[shapeIndex];
					caps.push_back(ShapeAccessor(shape, tileEdge, tr, tileEdgeIndex, shapeIndex));
				}
			}
		}
	}

	return caps;
}

void TilesetController::ComputeSweepControlPoints(const vec3& l0, const vec3& l1, vec3 &p0, vec3 &q0, vec3 &q1, vec3 &p1, int k0, int k1) {
	const mat4& tr0 = TileEdge::Transforms[k0];
	const mat4& tr1 = TileEdge::Transforms[k1];
	
	p0 = vec3(tr0 * vec4(l0, 1));
	p1 = vec3(tr1 * vec4(l1, 1));

	float distance = glm::distance(p0, p1);
	constexpr float SQRT2 = 1.4142135623730951f;

	float circleRadius = k0 == k1 ? distance : distance * SQRT2 / 2;
	float handleOffsetBudget = circleRadius * 8 * (SQRT2 - 1) / 3;

	float alpha0 = acos(dot(normalize(p1 - p0), mat3(tr0) * vec3(0, 0, 1)));
	float alpha1 = acos(dot(normalize(p0 - p1), mat3(tr1) * vec3(0, 0, 1)));
	if (abs(alpha0 + alpha1) < 1e-4) {
		alpha0 = alpha1 = 1;
	}

	q0 = vec3(tr0 * vec4(l0 - vec3(0, 0, handleOffsetBudget * alpha0 / (alpha0 + alpha1)), 1));
	q1 = vec3(tr1 * vec4(l1 - vec3(0, 0, handleOffsetBudget * alpha1 / (alpha0 + alpha1)), 1));
}

std::vector<TilesetController::SweepGeometry> TilesetController::GetPotentialPaths(const TileType& tileType) {
	std::vector<SweepGeometry> potentialPaths;

	vec3 p0, q0, q1, p1;
	for (int k0 = 0; k0 < TileEdge::Count; ++k0) {
		auto tileEdge0 = tileType.edges[k0];
		auto edgeType0 = tileEdge0.type.lock();
		if (!edgeType0) continue;

		for (int i0 = 0; i0 < edgeType0->shapes.size(); ++i0) {
			auto& shape0 = edgeType0->shapes[i0];
			vec2 center0 = center(shape0.csg);
			vec4 l0(center0.x - 0.5f, center0.y * 0.5f, 0.5f, 1.0f);
			if (tileEdge0.transform.flipped) {
				l0.x = -l0.x;
			}

			for (int k1 = 0; k1 < TileEdge::Count; ++k1) {
				auto tileEdge1 = tileType.edges[k1];
				auto edgeType1 = tileEdge1.type.lock();
				if (!edgeType1) continue;
				
				for (int i1 = 0; i1 < edgeType1->shapes.size(); ++i1) {
					if (k0 > k1 || (k0 == k1 && i0 >= i1)) continue;
					auto& shape1 = edgeType1->shapes[i1];
					vec2 center1 = center(shape1.csg);
					vec4 l1(center1.x - 0.5f, center1.y * 0.5f, 0.5f, 1.0f);
					if (tileEdge1.transform.flipped) {
						l1.x = -l1.x;
					}
					
					ComputeSweepControlPoints(l0, l1, p0, q0, q1, p1, k0, k1);

					SweepGeometry geo;
					geo.pointData;
					std::vector<GLuint> elementData;
					constexpr GLuint restartIndex = std::numeric_limits<GLuint>::max();
					Bezier::InsertBezier(p0, q0, q1, p1, geo.pointData, elementData, restartIndex);

					geo.model.tileEdgeFrom = k0;
					geo.model.shapeFrom = i0;
					geo.model.tileEdgeTo = k1;
					geo.model.shapeTo = i1;

					geo.controlPoints[0] = p0;
					geo.controlPoints[1] = q0;
					geo.controlPoints[2] = q1;
					geo.controlPoints[3] = p1;

					potentialPaths.push_back(geo);
				}
			}
		}
	}

	return potentialPaths;
}

std::vector<TilesetController::SweepGeometry> TilesetController::GetActivePaths(const TileType& tileType, bool includeCaps) {
	std::vector<SweepGeometry> activePaths;

	vec3 p0, q0, q1, p1;
	for (const auto& path : tileType.innerPaths) {
		int k0 = path.tileEdgeFrom;
		int k1 = path.tileEdgeTo;
		int i0 = path.shapeFrom;
		int i1 = path.shapeTo;

		auto tileEdge0 = tileType.edges[k0];
		auto edgeType0 = tileEdge0.type.lock();
		if (!edgeType0) continue;

		auto& shape0 = edgeType0->shapes[i0];
		vec2 center0 = center(shape0.csg);
		vec4 l0(center0.x - 0.5f, center0.y * 0.5f, 0.5f, 1.0f);
		if (tileEdge0.transform.flipped) {
			l0.x = -l0.x;
		}

		auto tileEdge1 = tileType.edges[k1];
		auto edgeType1 = tileEdge1.type.lock();
		if (!edgeType1) continue;

		if (k0 > k1 || (k0 == k1 && i0 >= i1)) continue;
		auto& shape1 = edgeType1->shapes[i1];
		vec2 center1 = center(shape1.csg);
		vec4 l1(center1.x - 0.5f, center1.y * 0.5f, 0.5f, 1.0f);
		if (tileEdge1.transform.flipped) {
			l1.x = -l1.x;
		}
		
		ComputeSweepControlPoints(l0, l1, p0, q0, q1, p1, k0, k1);

		SweepGeometry geo;
		geo.pointData;
		std::vector<GLuint> elementData;
		constexpr GLuint restartIndex = std::numeric_limits<GLuint>::max();
		Bezier::InsertBezier(p0, q0, q1, p1, geo.pointData, elementData, restartIndex);

		geo.model = path;

		geo.controlPoints[0] = p0;
		geo.controlPoints[1] = q0;
		geo.controlPoints[2] = q1;
		geo.controlPoints[3] = p1;

		geo.startPosition = center0;
		geo.endPosition = center1;

		activePaths.push_back(geo);
	}

	if (includeCaps) {
		for (const auto& accessor : GetTileCaps(tileType)) {
			int k0 = accessor.tileEdgeIndex();
			int i0 = accessor.shapeIndex();
			const auto& tileEdge0 = tileType.edges[k0];

			const auto& shape0 = accessor.shape();
			vec2 center0 = center(shape0.csg);
			vec4 l0(center0.x - 0.5f, center0.y * 0.5f, 0.5f, 1.0f);
			if (tileEdge0.transform.flipped) {
				l0.x = -l0.x;
			}

			ComputeSweepControlPoints(l0, l0 * vec4(-1, 1, 1, 1), p0, q0, q1, p1, k0, (k0 + 2) % 4);

			SweepGeometry geo;
			geo.model.tileEdgeFrom = k0;
			geo.model.shapeFrom = i0;
			geo.model.tileEdgeTo = -1;
			geo.model.shapeTo = -1;
			geo.controlPoints[0] = p0;
			geo.controlPoints[1] = q0;
			geo.startPosition = center0;
			activePaths.push_back(geo);
		}
	}

	return activePaths;
}

std::vector<TilesetController::SweepGeometry> TilesetController::GetActivePaths2(const TileType& tileType, bool includeCaps) {
	std::vector<SweepGeometry> activePaths;

	for (const auto& path : tileType.innerPaths) {
		int k0 = path.tileEdgeFrom;
		int k1 = path.tileEdgeTo;
		int i0 = path.shapeFrom;
		int i1 = path.shapeTo;

		auto tileEdge0 = tileType.edges[k0];
		auto edgeType0 = tileEdge0.type.lock();
		if (!edgeType0) continue;

		auto& shape0 = edgeType0->shapes[i0];
		vec2 center0 = center(shape0.csg);

		auto tileEdge1 = tileType.edges[k1];
		auto edgeType1 = tileEdge1.type.lock();
		if (!edgeType1) continue;

		if (k0 > k1 || (k0 == k1 && i0 >= i1)) continue;
		auto& shape1 = edgeType1->shapes[i1];
		vec2 center1 = center(shape1.csg);

		SweepGeometry geo;
		geo.model = path;

		geo.startPosition = center0;
		geo.endPosition = center1;
		geo.startFlip = tileEdge0.transform.flipped;
		geo.endFlip = tileEdge1.transform.flipped;

		activePaths.push_back(geo);
	}

	if (includeCaps) {
		for (const auto& accessor : GetTileCaps(tileType)) {
			int k0 = accessor.tileEdgeIndex();
			int i0 = accessor.shapeIndex();
			const auto& tileEdge0 = tileType.edges[k0];

			const auto& shape0 = accessor.shape();
			vec2 center0 = center(shape0.csg);

			SweepGeometry geo;
			geo.model.tileEdgeFrom = k0;
			geo.model.shapeFrom = i0;
			geo.model.tileEdgeTo = -1;
			geo.model.shapeTo = -1;
			geo.startPosition = center0;
			geo.startFlip = tileEdge0.transform.flipped;
			activePaths.push_back(geo);
		}
	}

	return activePaths;
}

Model::TransformedEdge TilesetController::GetEdgeTypeInDirection(const Model::TileType& tileType, Model::TileEdge::Direction direction, const Model::TileTransform& tileTransform) {
	int tileEdgeIndex = static_cast<int>(direction);
	Model::EdgeTransform edgeTransform;
	edgeTransform.flipped = false;

	tileEdgeIndex = (tileEdgeIndex + 4 - static_cast<int>(tileTransform.orientation)) % 4;

	if (tileTransform.flipY) {
		if (tileEdgeIndex % 2 == 1) {
			tileEdgeIndex = (tileEdgeIndex + 2) % 4;
		}
		edgeTransform.flipped = !edgeTransform.flipped;
	}

	if (tileTransform.flipX) {
		if (tileEdgeIndex % 2 == 0) {
			tileEdgeIndex = (tileEdgeIndex + 2) % 4;
		}
		edgeTransform.flipped = !edgeTransform.flipped;
	}

	const TileEdge& tileEdge = tileType.edges[tileEdgeIndex];
	if (tileEdge.transform.flipped) {
		edgeTransform.flipped = !edgeTransform.flipped;
	}

	return TransformedEdge{ tileEdge.type.lock(), edgeTransform };
}

TileEdge::TransformedDirection TilesetController::TransformEdgeDirection(const Model::TileType& tileType, TileEdge::Direction direction, const TileTransform& tileTransform) {
	int orientationIndex = static_cast<int>(tileTransform.orientation);
	int directionIndex = static_cast<int>(direction);
	int transformedDirectionIndex = directionIndex;
	
	//transformedDirectionIndex = (transformedDirectionIndex + 4 - orientationIndex) % 4;
	if (tileTransform.flipX && transformedDirectionIndex % 2 == 0) transformedDirectionIndex = (transformedDirectionIndex + 2) % 4;
	if (tileTransform.flipY && transformedDirectionIndex % 2 == 1) transformedDirectionIndex = (transformedDirectionIndex + 2) % 4;
	transformedDirectionIndex = (transformedDirectionIndex + orientationIndex) % 4;

	bool flip = false;
	if (tileTransform.flipX) flip = !flip;
	if (tileTransform.flipY) flip = !flip;
	if (tileType.edges[directionIndex].transform.flipped) flip = !flip;

	return TileEdge::TransformedDirection{
		static_cast<TileEdge::Direction>(transformedDirectionIndex),
		flip
	};
}

bool TilesetController::AreEdgesCompatible(const TransformedEdge& transformedEdgeA, const TransformedEdge& transformedEdgeB) {
	return
		transformedEdgeA.edge == transformedEdgeB.edge &&
		transformedEdgeA.transform.flipped != transformedEdgeB.transform.flipped;
}

int TilesetController::indexOfEdgeType(const std::weak_ptr<EdgeType>& edgeTypePtr) const {
	if (auto model = m_model.lock()) {
		if (auto edgeType = edgeTypePtr.lock()) {
			auto it = find(model->edgeTypes.begin(), model->edgeTypes.end(), edgeType);
			if (it != model->edgeTypes.end()) {
				return static_cast<int>(distance(model->edgeTypes.begin(), it));
			}
		}
	}
	return -1;
}

glm::mat2 TilesetController::TileOrientationAsMat2(TileOrientation orientation) {
	switch (orientation) {
	case TileOrientation::Deg0:
		return glm::mat2(
			1, 0,
			0, 1
		);
	case TileOrientation::Deg90:
		return glm::mat2(
			0, 1,
			-1, 0
		);
	case TileOrientation::Deg180:
		return glm::mat2(
			-1, 0,
			0, -1
		);
	case TileOrientation::Deg270:
		return glm::mat2(
			0, -1,
			1, 0
		);
	default:
		assert(false);
		return mat2(0);
	}
}

glm::mat2 TilesetController::TileTransformAsMat2(const Model::TileTransform& tileTransform) {
	glm::mat2 flipMatrix(
		tileTransform.flipX ? -1 : 1, 0,
		0, tileTransform.flipY ? -1 : 1
	);
	return TileOrientationAsMat2(tileTransform.orientation) * flipMatrix;
}

glm::uvec4 TilesetController::TileTransformAsPermutation(const Model::TileTransform& tileTransform) {
	// corner #i of the cage is sent to corner #perm[i] by this deformation
	glm::uvec4 perm(0u, 1u, 2u, 3u);
	if (tileTransform.flipX) {
		std::swap(perm[0], perm[3]);
		std::swap(perm[1], perm[2]);
	}
	if (tileTransform.flipY) {
		std::swap(perm[0], perm[1]);
		std::swap(perm[2], perm[3]);
	}
	int offset = static_cast<GLuint>(tileTransform.orientation);
	for (int k = 0; k < 4; ++k) {
		perm[k] = (perm[k] + offset) % 4;
	}
	return perm;
}

//-----------------------------------------------------------------------------
// Update

void TilesetController::setTileEdgeType(TileEdge& tileEdge, std::shared_ptr<EdgeType> edgeType) {
	tileEdge.type = edgeType;
	garbageCollectEdgeTypes();
	garbageCollectSweeps();

	setDirty();
}

void TilesetController::insertEdgeShape(EdgeType& edgeType, const Shape& shape) {
	std::vector<Model::Shape> newShapes;
	newShapes.reserve(edgeType.shapes.size() + 1);

	// When connected to an existing shape, it is merged into it, and so are all
	// the other shapes that are connected to it.
	Shape mergedShape = shape;

	for (const Shape& otherShape : edgeType.shapes) {
		bool intersect = distance(otherShape.csg, shape.csg) <= 1e-4;

		if (!intersect) {
			newShapes.push_back(otherShape);
		}
		else {
			mergedShape.csg = Model::Csg2D::Union(mergedShape.csg, otherShape.csg);
		}
	}

	newShapes.push_back(mergedShape);
	
	edgeType.shapes = newShapes;

	setDirty();
}

void TilesetController::subtractEdgeShape(EdgeType& edgeType, const Shape& shape) {
	// When connected to an existing shape, it is subtracted from it, and may
	// split it into multiple shapes. TODO
	
	for (Shape& otherShape : edgeType.shapes) {
		bool intersect = distance(otherShape.csg, shape.csg) <= 1e-4;
		
		if (intersect) {
			otherShape.csg = Model::Csg2D::Difference(otherShape.csg, shape.csg);
			setDirty();
			break;
		}
	}
}

void TilesetController::removeEdgeShape(EdgeType& edgeType, const Shape& shape) {
	std::vector<Model::Shape> newShapes;
	newShapes.reserve(edgeType.shapes.size() + 1);

	// When connected to an existing shape, it is subtracted from it, and may
	// split it into multiple shapes. TODO

	bool changed = false;
	for (Shape& otherShape : edgeType.shapes) {
		bool intersect = distance(otherShape.csg, shape.csg) <= 1e-4;

		if (intersect) {
			changed = true;
		}
		else {
			newShapes.push_back(otherShape);
		}
	}

	edgeType.shapes = newShapes;

	if (changed) {
		garbageCollectSweeps();
		setDirty();
	}
}

//-----------------------------------------------------------------------------
// Delete

void TilesetController::removeTileType(int tileTypeIndex) {
	if (auto model = m_model.lock()) {
		assert(tileTypeIndex >= 0);
		assert(tileTypeIndex < model->tileTypes.size());
		model->tileTypes.erase(model->tileTypes.begin() + tileTypeIndex);
		garbageCollectEdgeTypes();
		setDirty();
	}
}

void TilesetController::removeEdgeType(int edgeTypeIndex) {
	if (auto model = m_model.lock()) {
		assert(edgeTypeIndex >= 0);
		assert(edgeTypeIndex < model->edgeTypes.size());

		// Update paths and remove the ones that were using this shape
		// TODO: replace with GarbageCollectSweeps
		auto edgeType = model->edgeTypes[edgeTypeIndex];
		for (auto& tileType : model->tileTypes) {
			int innerPathsIndex = -1;
			std::vector<int> toRemove;
			for (auto& path : tileType->innerPaths) {
				++innerPathsIndex;
				if (tileType->edges[path.tileEdgeFrom].type.lock() == edgeType) {
					toRemove.push_back(innerPathsIndex);
				}
				else if (tileType->edges[path.tileEdgeTo].type.lock() == edgeType) {
					toRemove.push_back(innerPathsIndex);
				}
			}
			int offset = 0;
			for (int idx : toRemove) {
				tileType->innerPaths.erase(tileType->innerPaths.begin() + (idx - offset));
				++offset;
			}
		}

		// Effectively remove the edge
		model->edgeTypes.erase(model->edgeTypes.begin() + edgeTypeIndex);

		setDirty();
	}
}

void TilesetController::removeEdgeShape(EdgeType& edgeType, int shapeIndex) {
	if (auto model = m_model.lock()) {
		assert(shapeIndex >= 0);
		assert(shapeIndex < edgeType.shapes.size());
		edgeType.shapes.erase(edgeType.shapes.begin() + shapeIndex);

		// Update paths and remove the ones that were using this shape
		for (auto& tileType : model->tileTypes) {
			int innerPathsIndex = -1;
			std::vector<int> toRemove;
			for (auto& path : tileType->innerPaths) {
				++innerPathsIndex;
				if (tileType->edges[path.tileEdgeFrom].type.lock().get() == &edgeType) {
					if (path.shapeFrom == shapeIndex) {
						toRemove.push_back(innerPathsIndex);
					}
					if (path.shapeFrom > shapeIndex) {
						--path.shapeFrom;
					}
				}
				if (tileType->edges[path.tileEdgeTo].type.lock().get() == &edgeType) {
					if (path.shapeTo == shapeIndex && (toRemove.empty() || toRemove.back() != innerPathsIndex)) {
						toRemove.push_back(innerPathsIndex);
					}
					if (path.shapeTo > shapeIndex) {
						--path.shapeTo;
					}
				}
			}
			int offset = 0;
			for (int idx : toRemove) {
				tileType->innerPaths.erase(tileType->innerPaths.begin() + (idx - offset));
				++offset;
			}
		}

		setDirty();
	}
}

void TilesetController::removeTileInnerPath(TileType& tileType, const TileInnerPath& path) {
	auto it = tileType.innerPaths.begin();
	auto end = tileType.innerPaths.end();
	for (; it != end; ++it) {
		const auto& otherPath = *it;
		bool isSame = (
			otherPath.shapeFrom == path.shapeFrom
			&& otherPath.shapeTo == path.shapeTo
			&& otherPath.tileEdgeFrom == path.tileEdgeFrom
			&& otherPath.tileEdgeTo == path.tileEdgeTo
			);
		bool isOpposite = (
			otherPath.shapeFrom == path.shapeTo
			&& otherPath.shapeTo == path.shapeFrom
			&& otherPath.tileEdgeFrom == path.tileEdgeTo
			&& otherPath.tileEdgeTo == path.tileEdgeFrom
			);
		if (isSame || isOpposite) {
			tileType.innerPaths.erase(it);
			setDirty(tileTypeIndex(tileType));
			return;
		}
	}
}

void TilesetController::removeTileInnerGeometry(Model::TileType& tileType, int index) {
	tileType.innerGeometry.erase(tileType.innerGeometry.begin() + index);
	setDirty(tileTypeIndex(tileType));
}

//-----------------------------------------------------------------------------
// Convert

TilesetController::RulesetAdapter TilesetController::ConvertModelToRuleset(const Model::Tileset& model) {
	auto tileVariantList = std::make_shared<TileVariantList>(model);

	int variantCount = tileVariantList->variantCount();

	libwfc::ndvector<bool, 3> table(variantCount, variantCount, Model::TileEdge::Count);

	for (int direction = 0; direction < 2; ++direction) {

		TileEdge::Direction directionA = Model::TileEdge::Direction::PosX;
		TileEdge::Direction directionB = Model::TileEdge::Direction::PosX;
		if (direction == 0) {
			directionA = TileEdge::Direction::PosX;
			directionB = TileEdge::Direction::NegX;
		}
		else if (direction == 1) {
			directionA = TileEdge::Direction::PosY;
			directionB = TileEdge::Direction::NegY;
		}

		for (int labelA = 0; labelA < variantCount; ++labelA) {
			auto transformedTileA = tileVariantList->variantToTile(labelA);
			const auto& tileA = *model.tileTypes[transformedTileA.tileIndex];
			auto transformedEdgeA = GetEdgeTypeInDirection(tileA, directionA, transformedTileA.transform);

			for (int labelB = 0; labelB < variantCount; ++labelB) {
				auto transformedTileB = tileVariantList->variantToTile(labelB);
				const auto& tileB = *model.tileTypes[transformedTileB.tileIndex];
				auto transformedEdgeB = GetEdgeTypeInDirection(tileB, directionB, transformedTileB.transform);

				bool allowed = AreEdgesCompatible(transformedEdgeA, transformedEdgeB);
				table.set_at(allowed, labelA, labelB, static_cast<int>(directionA));
				table.set_at(allowed, labelB, labelA, static_cast<int>(directionB));
			}
		}
	}

	return RulesetAdapter{ std::move(table), std::move(tileVariantList) };
}

TilesetController::SignedWangRulesetAdapter TilesetController::ConvertModelToSignedWangRuleset(const Model::Tileset& model) {
	auto tileVariantList = std::make_shared<TileVariantList>(model);

	int variantCount = tileVariantList->variantCount();

	libwfc::ndvector<int, 2> table(variantCount, TileEdge::SolveCount);

	// Labels allowed to be used on borders (left unconnected)
	std::unordered_set<int> borderLabels;

	// Labels allowed to be used only on borders
	std::unordered_set<int> borderOnlyLabels;

	// Label 0 is reserved because it has no opposite
	std::map<std::shared_ptr<EdgeType>, int> lut;
	for (int i = 0; i < model.edgeTypes.size(); ++i) {
		lut[model.edgeTypes[i]] = i + 1;
		if (model.edgeTypes[i]->borderEdge) {
			borderLabels.insert(i + 1);
			borderLabels.insert(-(i + 1));
		}
		if (model.edgeTypes[i]->borderOnly) {
			borderOnlyLabels.insert(i + 1);
			borderOnlyLabels.insert(-(i + 1));
		}
	}

	for (int k = 0; k < 4; ++k) {

		auto direction = static_cast<TileEdge::Direction>(k);

		for (int var = 0; var < variantCount; ++var) {
			auto transformedTile = tileVariantList->variantToTile(var);
			const auto& tile = *model.tileTypes[transformedTile.tileIndex];
			auto transformedEdge = GetEdgeTypeInDirection(tile, direction, transformedTile.transform);
			int edgeIndex = lut[transformedEdge.edge];
			if (transformedEdge.transform.flipped) edgeIndex = -edgeIndex;

			//DEBUG_LOG << "table.set_at(" << edgeIndex << ", " << var << ", " << magic_enum::enum_name(direction) << ")";
			table.set_at(edgeIndex, var, static_cast<int>(direction));
		}
	}

	return SignedWangRulesetAdapter{ std::move(table), std::move(tileVariantList), borderLabels, borderOnlyLabels };
}

//-----------------------------------------------------------------------------
// private

int TilesetController::guessAssignmentOffset(Model::TileType& tileType, const TileInnerPath& path) {
	CsgCompiler::Options opts;
	opts.maxSegmentLength = 1e-2f;

	if (auto model = lockModel()) {
		auto tileEdgeFrom = tileType.edges[path.tileEdgeFrom];
		auto interfaceFrom = tileEdgeFrom.type.lock();
		auto tileEdgeTo = tileType.edges[path.tileEdgeTo];
		auto interfaceTo = tileEdgeTo.type.lock();
		if (!interfaceFrom || !interfaceTo) return 0;

		const auto& profileFrom = interfaceFrom->shapes[path.shapeFrom];
		std::vector<glm::vec2> profileDataFrom = CsgCompiler().compileGlm(profileFrom.csg, opts);
		const auto& profileTo = interfaceTo->shapes[path.shapeTo];
		std::vector<glm::vec2> profileDataTo = CsgCompiler().compileGlm(profileTo.csg, opts);

		auto sampleProfile = [](const std::vector<glm::vec2>& profileData, float u) {
			while (u < 0) u += 1;
			int n = (int)profileData.size();
			glm::vec2 p0 = profileData[(int)floor(u * n) % n];
			glm::vec2 p1 = profileData[((int)floor(u * n) + 1) % n];
			return mix(p0, p1, glm::fract(u * n));
		};

		bool flipFrom = tileEdgeFrom.transform.flipped;
		bool flipTo = tileEdgeTo.transform.flipped;

		vec2 startFlip = vec2(flipFrom ? -1 : 1, 1);
		vec2 endFlip = vec2(flipTo ? 1 : -1, 1);

		// Poor man's optimal transport
		constexpr int sampleCount = 20;
		int bestOffset = 0;
		float minCost = -1.0f;
		for (int k = 0; k < 100; ++k) {
			float offset = k / (float)100;
			float cost = 0;
			for (int i = 0; i < sampleCount; ++i) {
				float startRa = i / (float)sampleCount;
				float endRa = startRa;

				// Matches behavior of build-sweep-vao.comp.glsl
				if (flipFrom) startRa = 1 - startRa;
				if (!flipTo) endRa = 1 - endRa;
				if (flipFrom == flipTo) endRa += 0.5f;
				endRa += offset;

				glm::vec2 ptFrom = sampleProfile(profileDataFrom, startRa) * startFlip;
				glm::vec2 ptTo = sampleProfile(profileDataTo, endRa) * endFlip;
				float d = distance(ptFrom, ptTo);
				cost += d;
			}
			if (minCost < 0 || cost <= minCost) {
				bestOffset = k;
				minCost = cost;
			}
		}
		return bestOffset;
	}
	return 0;
}

void TilesetController::garbageCollectEdgeTypes() {
	if (auto model = m_model.lock()) {
		std::set<std::shared_ptr<EdgeType>> used;
		for (auto& tileType : model->tileTypes) {
			for (auto& tileEdge : tileType->edges) {
				if (auto edgeType = tileEdge.type.lock()) {
					used.insert(edgeType);
				}
			}
		}

		auto& edgeTypes = model->edgeTypes;
		for (int i = 0; i < edgeTypes.size(); ++i) {
			if (!edgeTypes[i]->isExplicit && !used.count(edgeTypes[i])) {
				edgeTypes.erase(edgeTypes.cbegin() + i);
				--i;
			}
		}
	}
}

void TilesetController::garbageCollectSweeps() {
	if (auto model = m_model.lock()) {
		for (auto& tileType : model->tileTypes) {
			std::vector<Model::TileInnerPath> newSweeps;
			for (auto& sweep : tileType->innerPaths) {
				auto edgeTypeFrom = tileType->edges[sweep.tileEdgeFrom].type.lock();
				if (!edgeTypeFrom) continue;
				if (sweep.shapeFrom >= edgeTypeFrom->shapes.size()) continue;
				auto edgeTypeTo = tileType->edges[sweep.tileEdgeTo].type.lock();
				if (!edgeTypeTo) continue;
				if (sweep.shapeTo >= edgeTypeTo->shapes.size()) continue;
				newSweeps.push_back(sweep);
			}
			tileType->innerPaths = newSweeps;
		}
	}
}
