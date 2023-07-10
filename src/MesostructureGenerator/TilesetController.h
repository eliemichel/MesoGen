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

#include <OpenGL>
#include "Model.h"
#include "TileVariantList.h"

#include <libwfc.h>
#include <ReflectionAttributes.h>

#include <glm/glm.hpp>
#include <refl.hpp>

#include <memory>
#include <vector>
#include <array>

class MesostructureData;

/**
 * If something is doable using this TilesetController, use it;
 * otherwise directly edit the Model::Tileset object.
 */
class TilesetController {
public:
	class ShapeAccessor {
	public:
		ShapeAccessor();
		ShapeAccessor(Model::Shape& shape, const Model::TileEdge& tileEdge, const glm::mat4& transform, int tileEdgeIndex, int shapeIndex);

		const Model::Shape& shape() const;
		bool isValid() const;

		int tileEdgeIndex() const { return m_tileEdgeIndex; }
		int shapeIndex() const { return m_shapeIndex; }
		const glm::mat4& transform() const { return m_transform; };
		const Model::TileEdge& tileEdge() const { return m_tileEdge; };

	private:
		static Model::Shape invalidShape;
		static Model::TileEdge invalidTileEdge;

	private:
		bool m_isValid;
		Model::Shape& m_shape;
		const Model::TileEdge& m_tileEdge;
		const glm::mat4& m_transform;

		int m_tileEdgeIndex;
		int m_shapeIndex;
	};

public:
	void setModel(std::weak_ptr<Model::Tileset> model);
	std::weak_ptr<Model::Tileset> model() const { return m_model; }
	std::shared_ptr<Model::Tileset> lockModel() const { return m_model.lock(); }

	void setMesostructureData(std::weak_ptr<MesostructureData> mesostructureData);

	// poor man's signaling
	bool hasChangedLately() const { return m_hasChangedLately; }
	void resetChangedEvent() { m_hasChangedLately = false; }
	void setDirty(int tileIndex = -1);

	//-----------------------------------------------------------------------------
	// Create

	void addTileType();
	void addEdgeType();
	void addTileInnerPath(Model::TileType& tileType, const Model::TileInnerPath& path);
	void addTileInnerGeometry(Model::TileType& tileType);
	void copyTileType(Model::TileType& tileType);

	//-----------------------------------------------------------------------------
	// Read

	int tileTypeIndex(const Model::TileType& tileType) const;

	bool hasTileInnerPath(const Model::TileType& tileType, const Model::TileInnerPath& path);
	ShapeAccessor getShapeAccessor(const Model::TileType& tileType, int tileEdgeIndex, int shapeIndex);

	/**
	 * Get all shapes of a tile
	 */
	static std::vector<ShapeAccessor> GetTileShapes(const Model::TileType& tileType);

	/**
	 * Caps are shapes that are connected to no path in this tile
	 */
	static std::vector<ShapeAccessor> GetTileCaps(const Model::TileType& tileType);

	/**
	 * @param l0 l1 coords of start/end in local face frame
	 * @param p0, q0, q1, p1 output tile space coords of the control points
	 * @param k0, k1 indices of the start/end faces
	 */
	static void ComputeSweepControlPoints(const glm::vec3& l0, const glm::vec3& l1, glm::vec3& p0, glm::vec3& q0, glm::vec3& q1, glm::vec3& p1, int k0, int k1);

	struct SweepGeometry {
		Model::TileInnerPath model;
		glm::vec2 startPosition; // within a tile interface
		glm::vec2 endPosition;
		bool startFlip;
		bool endFlip;

		// no longer needed?
		std::vector<GLfloat> pointData;
		std::array<glm::vec3, 4> controlPoints;
	};
	static std::vector<SweepGeometry> GetPotentialPaths(const Model::TileType& tileType);
	static std::vector<SweepGeometry> GetActivePaths(const Model::TileType& tileType, bool includeCaps = false);
	static std::vector<SweepGeometry> GetActivePaths2(const Model::TileType& tileType, bool includeCaps = false);

	/**
	 * Return an edge type index and its orientation
	 */
	static Model::TransformedEdge GetEdgeTypeInDirection(const Model::TileType& tileType, Model::TileEdge::Direction direction, const Model::TileTransform& tileTransform);

	/**
	 * Tell what an edge of a tile becomes after applying a tile transform
	 * It's a bit the inverse of GetEdgeTypeInDirection
	 * @param tile type (used only to get initial edge transform)
	 * @param direction Index of the edge in local tile space
	 * @param tileTransform transformation to apply
	 * @return a direction in the slot space, and an edge transform (packed in a TransformedDirection)
	 */
	static Model::TileEdge::TransformedDirection TransformEdgeDirection(const Model::TileType& tileType, Model::TileEdge::Direction direction, const Model::TileTransform& tileTransform);

	static bool AreEdgesCompatible(const Model::TransformedEdge& transformedEdgeA, const Model::TransformedEdge& transformedEdgeB);

	/**
	 * Use sparsely, O(n)
	 */
	int indexOfEdgeType(const std::weak_ptr<Model::EdgeType>& edgeTypePtr) const;

	static glm::mat2 TileOrientationAsMat2(Model::TileOrientation orientation);
	static glm::mat2 TileTransformAsMat2(const Model::TileTransform& tileTransform);
	static glm::uvec4 TileTransformAsPermutation(const Model::TileTransform& tileTransform);

	//-----------------------------------------------------------------------------
	// Update

	void setTileEdgeType(Model::TileEdge& tileEdge, std::shared_ptr<Model::EdgeType> edgeType);

	/**
	 * Insert a new shape and make sure they are still orderred correctly.
	 * If two shapes overlap, they are merged.
	 */
	void insertEdgeShape(Model::EdgeType& edgeType, const Model::Shape& shape);
	void subtractEdgeShape(Model::EdgeType& edgeType, const Model::Shape& shape);
	void removeEdgeShape(Model::EdgeType& edgeType, const Model::Shape& shape);

	//-----------------------------------------------------------------------------
	// Delete

	void removeTileType(int tileTypeIndex);
	void removeEdgeType(int edgeTypeIndex);

	/**
	 * shapeIndex MUST be valid
	 */
	void removeEdgeShape(Model::EdgeType& edgeType, int shapeIndex);

	void removeTileInnerPath(Model::TileType& tileType, const Model::TileInnerPath& path);

	void removeTileInnerGeometry(Model::TileType& tileType, int index);

	//-----------------------------------------------------------------------------
	// Convert

	struct RulesetAdapter {
		libwfc::ndvector<bool, 3> table;
		std::shared_ptr<TileVariantList> tileVariantList;
	};
	static RulesetAdapter ConvertModelToRuleset(const Model::Tileset& model);

	struct SignedWangRulesetAdapter {
		libwfc::ndvector<int, 2> table = { 0, 0 };
		std::shared_ptr<TileVariantList> tileVariantList;
		std::unordered_set<int> borderLabels;
		std::unordered_set<int> borderOnlyLabels;
	};
	static SignedWangRulesetAdapter ConvertModelToSignedWangRuleset(const Model::Tileset& model);

public:
	struct Properties {
		std::string tilesetFilename = "tileset.json";
	};
	const Properties& properties() const { return m_properties; }
	Properties& properties() { return m_properties; }

private:
	int guessAssignmentOffset(Model::TileType& tileType, const Model::TileInnerPath& path);

	/**
	 * Remove all edge types that are not used by any tile type unless they are
	 * marked as explicit.
	 */
	void garbageCollectEdgeTypes();

	/**
	 * Remove all sweeps that use profiles that have been removed.
	 */
	void garbageCollectSweeps();

private:
	Properties m_properties;
	std::weak_ptr<Model::Tileset> m_model;
	std::weak_ptr<MesostructureData> m_mesostructureData;

	bool m_hasChangedLately = false;

private:
	static const std::vector<glm::vec3> DefaultEdgeColors;
};

#define _ ReflectionAttributes::
REFL_TYPE(TilesetController::Properties)
REFL_FIELD(tilesetFilename, _ MaximumSize(512))
REFL_END
#undef _
