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

#include "Ssbo.h"
#include "Model.h"
#include "MesostructureData.h"
#include "MacrosurfaceData.h"
#include "TileVariantList.h"
#include "CsgCompiler.h"
#include "TilesetController.h"
#include "BMesh.h"
#include "MesostructureRenderer.h"

#include <DrawCall.h>
#include <Camera.h>
#include <RuntimeObject.h>
#include <NonCopyable.h>
#include <ShaderProgram.h>
#include <ShaderPool.h>
#include <Texture.h>
#include <ReflectionAttributes.h>

#include <glm/glm.hpp>
#include <refl.hpp>

#include <memory>
#include <vector>
#include <unordered_map>

/**
 * New mesostructure renderer, entierly mesh based and running in compute shaders.
 * This tries to recompute only what is needed.
 * This replaces both old MesostructureRenderData and MesostructureRenderer.
 * 
 * There is a lot in this class, which is why it was splitted initially, but in
 * the end it is more flexible this way.
 * 
 * This variant does not support extra geometry (that does not touch interfaces)
 */
class MesostructureMvcRenderer : public RuntimeObject, public NonCopyable {
public:
	enum class DeformationMode {
		PreGeneration,
		PostGeneration,
	};

public:
	MesostructureMvcRenderer(std::weak_ptr<MesostructureRenderer> mesostructureRenderer);

	void update() override;
	void render(const Camera& camera) const override { render(camera, 0, 0); }
	void render(const Camera& camera, int startTileIndex, int tileCount = 1) const;

	void setMesostructureData(std::weak_ptr<MesostructureData> mesostructureData) { m_mesostructureData = mesostructureData; }

	/**
	 * Number of instances of a given tile type in the current slot assignment.
	 */
	int instanceCount(int tileIndex) const;

	// temporary accessors used by the exporter
	const std::vector<GLuint>& sortedSlotsOffsets() const { return m_sortedSlotsOffsets; }
	const std::vector<std::vector<std::shared_ptr<DrawCall>>>& sweepDrawCalls() const { return m_sweepDrawCalls; }
	const Ssbo& macrosurfaceNormalSsbo() const { return m_macrosurfaceNormalSsbo; }
	const Ssbo& macrosurfaceCornerSsbo() const { return m_macrosurfaceCornerSsbo; }
	const Ssbo& sweepInfoSsbo() const { return m_sweepInfoSsbo; }
	const std::array<Ssbo, 4>& sweepControlPointSsbo() const { return m_sweepControlPointSsbo; }
	const std::vector<bool>& flippedSlot() const { return m_flippedSlot; }
	const std::vector<int>& sortedSlotIndices() const { return m_sortedSlotIndices; }

	// wrapper around static ComputeSweepGridSize
	glm::uvec2 computeSweepGridSize(int tileTypeIndex, int sweepIndex);

public:
	// Callbacks to be called when there are changes in the model
	// NB: Changes to the macrosurface and tile assignments are automatically
	// detected by version counters.
	// TODO: Build a controller that holds version counters for everything and
	// triggers all these callbacks in an efficient way.

	/**
	 * The most basic way to update this renderer's precomputed data is to call
	 * onModelChange for any kind of change. Alternatively, you can use all other
	 * callbacks when appropriate.
	 */
	void onModelChange();

	void onProfileChange(int edgeTypeIndex, int profileIndex);
	void onProfileAddedOrRemoved(int edgeTypeIndex);
	void onEdgeTypeAddedOrRemoved();
	void onProfilePrecisionChanged();
	void onSweepOffsetsChanged(int tileTypeIndex, int sweepIndex);
	void onSweepAddedOrRemoved(int tileTypeIndex);
	void onTileTypeAddedOrRemoved();
	void onSweepDivisionsChanged();
	void onEdgeTypeFlipped(int tileTypeIndex);

public:
	const MesostructureRenderer::Properties& properties() const { return m_mesostructureRenderer.lock()->properties(); }
	MesostructureRenderer::Properties& properties() { return m_mesostructureRenderer.lock()->properties(); }

private:
	/**
	 * Data provided for each sweep *type* when building the sweep control
	 * points.
	 */
	struct SweepBuildingInfo {
		GLuint startTileEdge;
		GLuint startProfile;
		GLuint endTileEdge;
		GLuint endProfile;

		glm::vec2 startCenter;
		glm::vec2 endCenter;

		GLuint flags; // IS_CAP, FLIP_START_PROFILE, FLIP_END_PROFILE
		GLuint _pad0;
		GLuint _pad1;
		GLuint _pad2;
	};

	/**
	 * Data provided for each sweep *instance* when building the sweep control
	 * points, providing indices to the other buffers. This must remain small as
	 * there are potentially many sweep instances.
	 */
	struct SweepIndexInfo {
		GLuint slotIndex;
		GLuint tileTypeIndex;
		GLuint sweepIndex;
		GLuint _pad0;
	};

private:
	// Update steps. These all assume that pointers are valid

	/**
	 * Utility function to permutate the columns of a matrix
	 * This is used for matrices representing the 4 corners of a slot
	 */
	static glm::mat4 Permutate(const glm::mat4& m, const glm::uvec4& perm);

	/**
	 * A profile drawn on a tile interface is stored on the GPU as a 1D texture.
	 */
	static std::shared_ptr<Texture> CreateProfileTexture(const Model::Shape& profile, const CsgCompiler::Options& opts);

	/**
	 * A sweep object is mostly a deformed grid.
	 * This function computes the most appropriate resolution for this grid,
	 * profided the geometry of the start/end profile and whether it is a cap or not.
	 */
	static glm::uvec2 ComputeSweepGridSize(
		const Model::TileInnerPath& sweep,
		const Model::TileType& tileType,
		std::vector<std::vector<std::shared_ptr<Texture>>>& profileTextures,
		const std::unordered_map<std::shared_ptr<Model::EdgeType>, int>& lut,
		int sweepDivisions
	);

	/**
	 * Return pairs (tileTypeIndex, sweepIndex) listing all sweep objects that
	 * depend on a given profile, so that we know which sweeps to update whenever
	 * a profile changes.
	 */
	static std::vector<std::pair<int, int>> SweepsDependingOnProfile(
		int edgeTypeIndex,
		int profileIndex,
		const std::vector<std::shared_ptr<Model::TileType>>& tileTypes,
		const std::unordered_map<std::shared_ptr<Model::EdgeType>, int>& lut
	);

	/**
	 * Vectors are empty when they have been invalidated by a change in the model.
	 * After this function, profileTextures size and the size of its components
	 * matches the content of edgeTypes, and newly added entries are set to null.
	 *
	 * Only the case of empty vectors is handled, not other size mismatches,
	 * because the caller is responsible from emptying the vectors when
	 * invalidating either a given edge type or the whole profile pool.
	 */
	static void EnsureCorrectVectorSizes(
		std::vector<std::vector<std::shared_ptr<Texture>>>& profileTextures,
		const std::vector<std::shared_ptr<Model::EdgeType>>& edgeTypes
	);
	static bool EnsureCorrectVectorSizes(
		std::vector<std::vector<std::shared_ptr<DrawCall>>>& sweepDrawCalls,
		const std::vector<std::vector<TilesetController::SweepGeometry>>& sweeps
	);
	static void EnsureCorrectVectorSizes(
		std::vector<std::vector<bool>>& sweepDrawCallIsDeformed,
		const std::vector<std::vector<TilesetController::SweepGeometry>>& sweeps
	);
	static void EnsureCorrectVectorSizes(
		std::vector<std::vector<glm::uvec2>>& sweepResolutions,
		const std::vector<std::vector<TilesetController::SweepGeometry>>& sweeps
	);

	/**
	 * The macrosurface is stored as a buffer of mat4 for corner positions, and
	 * another one for normals.
	 * Corners are permutted depending on the tile transform of the tiles
	 * associated to the faces, so these buffer depend both from the original mesh
	 * and from the thing.
	 */
	static bool BuildMacrosurfaceSsbos(
		const MesostructureData& mesostructure,
		bmesh::BMesh& bmesh,
		Ssbo& macrosurfaceCornerSsbo,
		Ssbo& macrosurfaceNormalSsbo,
		const std::vector<int>& indices,
		std::vector<bool>& flippedSlot
	);

	static std::vector<int> SortSlotIndices(
		const MesostructureData& mesostructure,
		std::vector<GLuint>& offsets
	);

	std::shared_ptr<DrawCall> CreateWireframeDrawCall(
		const Model::TileType& tileType,
		bool includeDiagonals
	);

public:
	// Non static update steps

	/**
	 * Compute the temporary look up table that maps edgeType pointers to
	 * edgeType indices.
	 */
	void updateLut(const Model::Tileset& model);

	/**
	 * Update all profile textures that are missing, and ensures that
	 * this->profileTextures has correct sizes
	 */
	void updateProfileTextures(const Model::Tileset& model);

	/**
	 * Allocate/Update VAO for each sweep type, if needed, i.e. if profile
	 * changed, or model changed etc. In practice, update each element of
	 * sweepDrawCalls that is a nullptr.
	 * Return true iff the number of sweep draw calls changed
	 */
	bool updateSweepDrawCalls(
		const std::vector<std::vector<TilesetController::SweepGeometry>>& sweeps,
		const Model::Tileset& model,
		bool& sweepsCountChanged,
		bool& sweepsChanged
	);

	/**
	 * Deform the sweep into the unit tile space
	 * @param sweepControlPointOffset if not provided, computed automatically
	 */
	void deformSweepDrawCall(int tileTypeIndex, int sweepIndex, GLuint sweepControlPointOffset = -1);

	/**
	 * For wireframe vizualisation, we flip diagonal edges to match tile edge flipping
	 */
	void updateWireframeDrawCalls(const Model::Tileset& model);

	/**
	 * Each sweep type is stored in a different draw call (sweep type, not sweep
	 * instance, so one per sweep in tile *type*, not for each slot).
	 */
	std::shared_ptr<DrawCall> createSweepDrawCall(
		int tileTypeIndex,
		int sweepIndex,
		const TilesetController::SweepGeometry& sweepGeometry,
		const Model::Tileset& model
	);

	/**
	 * Allocate data to store sweep path for each sweep *instance*, and upload
	 * SSBOs containing data needed to procedurally generate paths.
	 */
	void allocateSweepPaths(
		const std::vector<std::vector<TilesetController::SweepGeometry>>& sweeps,
		const Model::Tileset& model
	);

	/**
	 * Run the sweep path generation compute shader. It comes either after
	 * allocateSweepPaths() or when procedural parameters like offset/thickness
	 * changed.
	 */
	void updateSweepPaths();

private:
	std::weak_ptr<MesostructureRenderer> m_mesostructureRenderer;

	// profiles[edgeTypeIndex][profileIndex]
	std::vector<std::vector<std::shared_ptr<Texture>>> m_profileTextures;

	// sweepDrawCalls[tileTypeIndex][sweepIndex]
	std::vector<std::vector<std::shared_ptr<DrawCall>>> m_sweepDrawCalls;

	// sweepDrawCallIsDeformed[tileTypeIndex][sweepIndex]
	std::vector<std::vector<bool>> m_sweepDrawCallIsDeformed;
	std::vector<std::vector<glm::uvec2>> m_sweepResolutions;

	// wireframeDrawCalls[tileTypeIndex]
	std::vector<std::shared_ptr<DrawCall>> m_wireframeDrawCalls;

	// Total amount of sweep *instances*
	GLuint m_sweepInstanceCount;
	std::array<Ssbo, 4> m_sweepControlPointSsbo;

	Ssbo m_macrosurfaceCornerSsbo;
	Ssbo m_macrosurfaceNormalSsbo;
	std::vector<GLuint> m_sortedSlotsOffsets;
	Ssbo m_sortedSlotsOffsetSsbo;
	Ssbo m_sweepInfoSsbo;

	// SSBOs used to compute the path of individual sweep instances
	Ssbo m_sweepBuildingInfoSsbo;
	Ssbo m_sweepIndexInfoSsbo;

	std::vector<bool> m_flippedSlot;
	std::vector<int> m_sortedSlotIndices;

	std::shared_ptr<ShaderProgram> m_buildSweepVaoShader;
	std::shared_ptr<ShaderProgram> m_deformSweepVaoShader;
	std::shared_ptr<ShaderProgram> m_buildSweepControlPointsShader;
	std::shared_ptr<ShaderProgram> m_renderShader;
	std::shared_ptr<ShaderProgram> m_wireframeRenderShader;

	std::weak_ptr<MesostructureData> m_mesostructureData;
	VersionCounter m_slotAssignmentsVersion;
	VersionCounter m_macrosurfaceVersion;
	bool m_edgeTypeFlipped;

	// temporary object built during the update() function to map edgeType pointers to edgeTypeIndices;
	std::unordered_map<std::shared_ptr<Model::EdgeType>, int> m_lut;

	// Values of the properties offset and thickness last time we've precomputed
	// sweep control points, to detect changes.
	MesostructureRenderer::Properties m_cachedProperties;
};
