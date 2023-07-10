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
#include "MesostructureMvcRenderer.h"

#include <utils/reflutils.h>

#include <glm/glm.hpp>

using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::uvec2;
using glm::uvec4;
using glm::mat2;
using glm::mat4;

//-----------------------------------------------------------------------------
#pragma region [Public Methods]

MesostructureMvcRenderer::MesostructureMvcRenderer(std::weak_ptr<MesostructureRenderer> mesostructureRenderer)
	: m_mesostructureRenderer(mesostructureRenderer)
{
	m_buildSweepVaoShader = ShaderPool::GetShader("build-sweep-vao", -1, false /* load */);
	m_buildSweepVaoShader->setType(ShaderProgram::ShaderProgramType::ComputeShader);
	m_buildSweepVaoShader->load();

	m_deformSweepVaoShader = ShaderPool::GetShader("deform-sweep-vao", -1, false /* load */);
	m_deformSweepVaoShader->setType(ShaderProgram::ShaderProgramType::ComputeShader);
	m_deformSweepVaoShader->load();

	m_buildSweepControlPointsShader = ShaderPool::GetShader("build-unit-sweep-control-points", -1, false /* load */);
	m_buildSweepControlPointsShader->setType(ShaderProgram::ShaderProgramType::ComputeShader);
	m_buildSweepControlPointsShader->load();

	m_renderShader = ShaderPool::GetShader("mesostructure-tile-mesh-mvc");

	m_wireframeRenderShader = ShaderPool::GetShader("mesostructure-tile-wireframe");
}

int MesostructureMvcRenderer::instanceCount(int tileIndex) const {
	if (tileIndex + 1 > m_sortedSlotsOffsets.size()) return 0;
	return m_sortedSlotsOffsets[tileIndex + 1] - m_sortedSlotsOffsets[tileIndex];
}

#pragma endregion
//-----------------------------------------------------------------------------
#pragma region [Public update/render outline]

void MesostructureMvcRenderer::update() {
	if (!properties().enabled || properties().deformationMode != MesostructureRenderer::DeformationMode::PostGeneration) return;
	if (properties().alwaysRecompute) {
		onModelChange();
	}
	auto mesostructure = m_mesostructureData.lock(); if (!mesostructure) return;
	if (!mesostructure->macrosurface.lock()) return;
	if (!mesostructure->macrosurface.lock()->bmesh()) return;
	const auto& model = *mesostructure->model;

	auto& tileTypes = mesostructure->model->tileTypes;

	updateLut(model);

	std::shared_ptr<TileVariantList> tileVariantList = mesostructure->tileVariantList;
	std::shared_ptr<MacrosurfaceData> macrosurface = mesostructure->macrosurface.lock();

	// 1. Upload new profiles

	updateProfileTextures(model);

	// 2.1. Pre-cache sweep geometry
	// TODO: Benchmark whether this is a bottleneck and maybe do lazy evaluation,
	// i.e. just compute the number of active path (to check for invalidation) as
	// long as it did not change.
	std::vector<std::vector<TilesetController::SweepGeometry>> sweeps;
	for (int tileTypeIndex = 0; tileTypeIndex < tileTypes.size(); ++tileTypeIndex) {
		sweeps.push_back(TilesetController::GetActivePaths2(*tileTypes[tileTypeIndex], true /* include caps */));
	}

	// 2.2. Reallocate and compute Sweep VAOs if needed
	bool sweepsCountChanged = false;
	bool sweepsChanged = false;
	updateSweepDrawCalls(sweeps, model, sweepsCountChanged, sweepsChanged);

	// 4. Load macrosurface
	bool tileCountChanged = m_sortedSlotsOffsets.size() != model.tileTypes.size() + 1;
	bool macrosurfaceChanged = m_macrosurfaceVersion.mustUpdate(macrosurface->version());
	bool slotAssignmentsChanged = m_slotAssignmentsVersion.mustUpdate(mesostructure->slotAssignmentsVersion);
	if (tileCountChanged || macrosurfaceChanged || slotAssignmentsChanged) {
		// Sort by tile type
		m_sortedSlotIndices = SortSlotIndices(*mesostructure, m_sortedSlotsOffsets);
		m_sortedSlotsOffsetSsbo.update(m_sortedSlotsOffsets);
	}
	if (macrosurfaceChanged || slotAssignmentsChanged) {
		// Upload new surface
		BuildMacrosurfaceSsbos(*mesostructure, *macrosurface->bmesh(), m_macrosurfaceCornerSsbo, m_macrosurfaceNormalSsbo, m_sortedSlotIndices, m_flippedSlot);
	}

	// Finally, update wireframe
	updateWireframeDrawCalls(model);

	// reset event flags
	m_edgeTypeFlipped = false;
}

void MesostructureMvcRenderer::render(const Camera& camera, int startTileIndex, int tileCount) const {
	if (!properties().enabled || properties().deformationMode != MesostructureRenderer::DeformationMode::PostGeneration) return;
	auto mesostructure = m_mesostructureData.lock(); if (!mesostructure) return;
	const auto& shader = *m_renderShader;

	int endTileIndex = static_cast<int>(m_sweepDrawCalls.size());
	if (tileCount > 0) {
		endTileIndex = std::min(endTileIndex, startTileIndex + tileCount);
	}

	// 1. Draw sweeps

	glEnable(GL_DEPTH_TEST);

	autoSetUniforms(shader, properties());
	shader.setUniform("uModelMatrix", mesostructure->modelMatrix());
	shader.bindStorageBlock("SweepInfos", m_sweepInfoSsbo.raw);
	for (int k = 0; k < 4; ++k) {
		shader.bindStorageBlock("SweepControlPoint" + std::to_string(k), m_sweepControlPointSsbo[k].raw);
	}

	for (int tileIndex = startTileIndex; tileIndex < endTileIndex; ++tileIndex) {
		int tileInstanceCount = instanceCount(tileIndex);
		if (tileInstanceCount == 0) continue;
		shader.setUniform("uSlotStartIndex", m_sortedSlotsOffsets[tileIndex]);

		for (GLuint sweepIndex = 0; sweepIndex < m_sweepDrawCalls[tileIndex].size(); ++sweepIndex) {
			shader.setUniform("uSweepIndex", sweepIndex);
			shader.bindStorageBlock("MacrosurfaceCorners", m_macrosurfaceCornerSsbo.raw);
			shader.bindStorageBlock("MacrosurfaceNormals", m_macrosurfaceNormalSsbo.raw);

			auto rd = m_sweepDrawCalls[tileIndex][sweepIndex];
			rd->render(camera, 0, 0, tileInstanceCount);
		}
	}

	// 2. Draw wireframe

	if (properties().wireframe) {
		glLineWidth(1);

		m_wireframeRenderShader.use_count();
		autoSetUniforms(*m_wireframeRenderShader, properties());
		m_wireframeRenderShader->setUniform("uUseInstancing", true);
		m_wireframeRenderShader->setUniform("uModelMatrix", mesostructure->modelMatrix());
		m_wireframeRenderShader->bindStorageBlock("MacrosurfaceNormals", m_macrosurfaceNormalSsbo.raw);
		m_wireframeRenderShader->bindStorageBlock("MacrosurfaceCorners", m_macrosurfaceCornerSsbo.raw);

		for (int tileIndex = startTileIndex; tileIndex < endTileIndex; ++tileIndex) {
			int tileInstanceCount = instanceCount(tileIndex);
			if (tileInstanceCount == 0) continue;

			m_wireframeRenderShader->setUniform("uSlotStartIndex", m_sortedSlotsOffsets[tileIndex]);

			auto rd = m_wireframeDrawCalls[tileIndex];
			rd->render(camera, 0, 0, tileInstanceCount);
		}
	}
}

#pragma endregion
//-----------------------------------------------------------------------------
#pragma region [Public Callbacks]

void MesostructureMvcRenderer::onModelChange() {
	// TODO: force update slot assignemnt data
	onTileTypeAddedOrRemoved();
	onEdgeTypeAddedOrRemoved();
}

void MesostructureMvcRenderer::onProfileChange(int edgeTypeIndex, int profileIndex) {
	if (!m_profileTextures.empty() && !m_profileTextures[edgeTypeIndex].empty()) {
		m_profileTextures[edgeTypeIndex][profileIndex].reset();
	}
}

void MesostructureMvcRenderer::onProfileAddedOrRemoved(int edgeTypeIndex) {
	if (!m_profileTextures.empty()) {
		m_profileTextures[edgeTypeIndex].clear();
	}
}

void MesostructureMvcRenderer::onEdgeTypeAddedOrRemoved() {
	m_profileTextures.clear();
}

void MesostructureMvcRenderer::onProfilePrecisionChanged() {
	m_profileTextures.clear();
}

void MesostructureMvcRenderer::onSweepOffsetsChanged(int tileTypeIndex, int sweepIndex) {
	if (!m_sweepDrawCalls.empty() && !m_sweepDrawCalls[tileTypeIndex].empty()) {
		m_sweepDrawCalls[tileTypeIndex][sweepIndex].reset();
	}
}

void MesostructureMvcRenderer::onSweepAddedOrRemoved(int tileTypeIndex) {
	if (!m_sweepDrawCalls.empty()) {
		m_sweepDrawCalls[tileTypeIndex].clear();
		m_sweepDrawCallIsDeformed[tileTypeIndex].clear();
		m_sweepResolutions[tileTypeIndex].clear();
	}
}

void MesostructureMvcRenderer::onTileTypeAddedOrRemoved() {
	m_sweepDrawCalls.clear();
	m_sweepDrawCallIsDeformed.clear();
	m_sweepResolutions.clear();

	m_wireframeDrawCalls.clear();
}

void MesostructureMvcRenderer::onSweepDivisionsChanged() {
	m_sweepDrawCalls.clear();
	m_sweepDrawCallIsDeformed.clear();
	m_sweepResolutions.clear();
}

void MesostructureMvcRenderer::onEdgeTypeFlipped(int tileTypeIndex) {
	m_edgeTypeFlipped = true;
	if (!m_wireframeDrawCalls.empty()) {
		m_wireframeDrawCalls[tileTypeIndex].reset();
	}
}

#pragma endregion
//-----------------------------------------------------------------------------
#pragma region [Static update Steps]

mat4 MesostructureMvcRenderer::Permutate(const mat4& m, const uvec4& perm) {
	return mat4(
		m[perm[0]],
		m[perm[1]],
		m[perm[2]],
		m[perm[3]]
	);
}

std::shared_ptr<Texture> MesostructureMvcRenderer::CreateProfileTexture(const Model::Shape& profile, const CsgCompiler::Options& opts) {
	std::vector<glm::vec2> profileData = CsgCompiler().compileGlm(profile.csg, opts);
	GLsizei size = static_cast<GLsizei>(profileData.size());
	auto profileTexture = std::make_shared<Texture>(GL_TEXTURE_1D);
	profileTexture->storage(1, GL_RG32F, size);
	profileTexture->subImage(0, 0, size, GL_RG, GL_FLOAT, profileData.data());
	profileTexture->setWrapMode(GL_REPEAT);
	return profileTexture;
}

uvec2 MesostructureMvcRenderer::computeSweepGridSize(int tileTypeIndex, int sweepIndex) {
	auto mesostructure = m_mesostructureData.lock(); if (!mesostructure) return uvec2(0);
	const auto& model = *mesostructure->model;
	updateLut(model);
	const auto& tileType = *model.tileTypes[tileTypeIndex];
	if (sweepIndex >= tileType.innerPaths.size()) return uvec2(0);
	const auto& sweep = tileType.innerPaths[sweepIndex];
	return ComputeSweepGridSize(sweep, tileType, m_profileTextures, m_lut, properties().sweepDivisions);
}

uvec2 MesostructureMvcRenderer::ComputeSweepGridSize(
	const Model::TileInnerPath& sweep,
	const Model::TileType& tileType,
	std::vector<std::vector<std::shared_ptr<Texture>>>& profileTextures,
	const std::unordered_map<std::shared_ptr<Model::EdgeType>, int>& lut,
	int sweepDivisions
) {
	bool isCap = sweep.tileEdgeTo == -1;
	if (isCap) {
		int edgeTypeFrom = lut.at(tileType.edges[sweep.tileEdgeFrom].type.lock());

		GLsizei width0 = profileTextures[edgeTypeFrom][sweep.shapeFrom]->width();
		return uvec2(width0, sweepDivisions / 4);
	}
	else {
		int edgeTypeFrom = lut.at(tileType.edges[sweep.tileEdgeFrom].type.lock());
		int edgeTypeTo = lut.at(tileType.edges[sweep.tileEdgeTo].type.lock());

		GLsizei width0 = profileTextures[edgeTypeFrom][sweep.shapeFrom]->width();
		GLsizei width1 = profileTextures[edgeTypeTo][sweep.shapeTo]->width();
		return uvec2(std::max(width0, width1), sweepDivisions);
	}
}

std::vector<std::pair<int, int>> MesostructureMvcRenderer::SweepsDependingOnProfile(
	int edgeTypeIndex,
	int profileIndex,
	const std::vector<std::shared_ptr<Model::TileType>>& tileTypes,
	const std::unordered_map<std::shared_ptr<Model::EdgeType>, int>& lut
) {
	std::vector<std::pair<int, int>> res;
	for (int tileTypeIndex = 0; tileTypeIndex < tileTypes.size(); ++tileTypeIndex) {
		const auto& sweeps = tileTypes[tileTypeIndex]->innerPaths;
		for (int sweepIndex = 0; sweepIndex < sweeps.size(); ++sweepIndex) {
			const auto& sw = sweeps[sweepIndex];

			bool depends = false;
			int edgeTypeFrom = lut.at(tileTypes[tileTypeIndex]->edges[sw.tileEdgeFrom].type.lock());
			if (edgeTypeFrom == edgeTypeIndex && sw.shapeFrom == profileIndex) {
				depends = true;
			}

			if (sw.tileEdgeTo != -1) { // if not a cap
				int edgeTypeTo = lut.at(tileTypes[tileTypeIndex]->edges[sw.tileEdgeTo].type.lock());
				if (edgeTypeTo == edgeTypeIndex && sw.shapeTo == profileIndex) {
					depends = true;
				}
			}

			if (depends) {
				res.push_back(std::make_pair(tileTypeIndex, sweepIndex));
			}
		}
	}
	return res;
}

void MesostructureMvcRenderer::EnsureCorrectVectorSizes(
	std::vector<std::vector<std::shared_ptr<Texture>>>& profileTextures,
	const std::vector<std::shared_ptr<Model::EdgeType>>& edgeTypes
) {
	if (profileTextures.empty()) {
		profileTextures.resize(edgeTypes.size());
	}

	assert(profileTextures.size() == edgeTypes.size());

	for (int edgeTypeIndex = 0; edgeTypeIndex < edgeTypes.size(); ++edgeTypeIndex) {

		auto& profileTexturesPerEdge = profileTextures[edgeTypeIndex];
		auto& sweeps = edgeTypes[edgeTypeIndex]->shapes;

		if (profileTexturesPerEdge.empty()) {
			profileTexturesPerEdge.resize(sweeps.size());
		}
		assert(profileTexturesPerEdge.size() == sweeps.size());
	}
}

bool MesostructureMvcRenderer::EnsureCorrectVectorSizes(
	std::vector<std::vector<std::shared_ptr<DrawCall>>>& sweepDrawCalls,
	const std::vector<std::vector<TilesetController::SweepGeometry>>& sweeps
) {
	bool changed = false;
	if (sweepDrawCalls.empty()) {
		sweepDrawCalls.resize(sweeps.size());
		changed = true;
	}

	assert(sweepDrawCalls.size() == sweeps.size());

	for (int tileTypeIndex = 0; tileTypeIndex < sweeps.size(); ++tileTypeIndex) {

		auto& drawCallsPerTile = sweepDrawCalls[tileTypeIndex];
		auto& sweepsPerTile = sweeps[tileTypeIndex];

		if (drawCallsPerTile.empty()) {
			drawCallsPerTile.resize(sweepsPerTile.size());
			changed = true;
		}
		assert(drawCallsPerTile.size() == sweepsPerTile.size());
	}
	return changed;
}

void MesostructureMvcRenderer::EnsureCorrectVectorSizes(
	std::vector<std::vector<bool>>& sweepDrawCallIsDeformed,
	const std::vector<std::vector<TilesetController::SweepGeometry>>& sweeps
) {
	if (sweepDrawCallIsDeformed.empty()) {
		sweepDrawCallIsDeformed.resize(sweeps.size());
		for (int tileTypeIndex = 0; tileTypeIndex < sweeps.size(); ++tileTypeIndex) {
			auto& flags = sweepDrawCallIsDeformed[tileTypeIndex];
			if (flags.empty()) {
				flags.resize(sweeps[tileTypeIndex].size());
				fill(flags.begin(), flags.end(), false);
			}
		}
	}
}

void MesostructureMvcRenderer::EnsureCorrectVectorSizes(
	std::vector<std::vector<glm::uvec2>>& sweepResolutions,
	const std::vector<std::vector<TilesetController::SweepGeometry>>& sweeps
) {
	if (sweepResolutions.empty()) {
		sweepResolutions.resize(sweeps.size());
		for (int tileTypeIndex = 0; tileTypeIndex < sweeps.size(); ++tileTypeIndex) {
			auto& res = sweepResolutions[tileTypeIndex];
			if (res.empty()) {
				res.resize(sweeps[tileTypeIndex].size());
			}
		}
	}
}

bool MesostructureMvcRenderer::BuildMacrosurfaceSsbos(
	const MesostructureData& mesostructure,
	bmesh::BMesh& bmesh, Ssbo& macrosurfaceCornerSsbo,
	Ssbo& macrosurfaceNormalSsbo, const std::vector<int>& indices,
	std::vector<bool>& flippedSlot
) {
	std::vector<mat4> positions;
	std::vector<mat4> normals;

	size_t slotCount = bmesh.faces.size();
	bool useAssignments = mesostructure.slotAssignments.size() == slotCount;

	positions.reserve(slotCount);
	normals.reserve(slotCount);
	flippedSlot.clear();
	flippedSlot.reserve(slotCount);

	mat4 p, n;
	for (int u = 0; u < slotCount; ++u) {
		int i = indices[u];
		auto face = bmesh.faces[i];

		auto loops = bmesh.NeighborLoops(face);
		assert(loops.size() == 4);

		for (int j = 0; j < 4; ++j) {
			p[j] = vec4(loops[j]->vertex->position, 1);
			n[j] = vec4(loops[j]->normal, 0);
		}

		bool flipped = false;
		if (useAssignments) {
			int variant = mesostructure.slotAssignments[i];
			if (variant < 0) continue;
			auto transformedTile = mesostructure.tileVariantList->variantToTile(variant);
			if (transformedTile.tileIndex < 0 || transformedTile.tileIndex >= mesostructure.tileCount()) continue;
			uvec4 permutation = TilesetController::TileTransformAsPermutation(transformedTile.transform);
			p = Permutate(p, permutation);
			n = Permutate(n, permutation);
			flipped = transformedTile.transform.flipX != transformedTile.transform.flipY;
		}

		positions.push_back(p);
		normals.push_back(n);
		flippedSlot.push_back(flipped);
	}

	macrosurfaceCornerSsbo.update(positions);
	macrosurfaceNormalSsbo.update(normals);

	return true;
}

std::vector<int> MesostructureMvcRenderer::SortSlotIndices(const MesostructureData& mesostructure, std::vector<GLuint>& offsets) {
	size_t slotCount = mesostructure.macrosurface.lock()->bmesh()->faces.size();
	size_t tileTypeCount = mesostructure.model->tileTypes.size();

	std::vector<std::vector<int>> splitIndices(tileTypeCount + 1); // one extra for unassigned
	std::vector<int> indices;

	// When there is a mismatch, don't sort
	if (mesostructure.slotAssignments.size() != slotCount) {
		offsets.resize(tileTypeCount + 1);
		for (int i = 0; i < tileTypeCount + 1; ++i) {
			offsets[i] = 0;
		}
		indices.resize(slotCount);
		for (int i = 0; i < slotCount; ++i) {
			indices[i] = i;
		}
		return indices;
	}

	for (int i = 0; i < slotCount; ++i) {
		int variant = mesostructure.slotAssignments[i];

		int tileIndex = static_cast<int>(tileTypeCount);
		if (variant >= 0) {
			auto transformedTile = mesostructure.tileVariantList->variantToTile(variant);
			if (transformedTile.tileIndex >= 0) {
				tileIndex = transformedTile.tileIndex;
			}
		}

		if (tileIndex < splitIndices.size()) {
			splitIndices[tileIndex].push_back(i);
		}
	}

	indices.reserve(slotCount);
	offsets.resize(tileTypeCount + 2);
	int c = 0;
	offsets[c] = 0;
	for (const auto& chunk : splitIndices) {
		indices.insert(
			indices.end(),
			std::make_move_iterator(chunk.begin()),
			std::make_move_iterator(chunk.end())
		);
		offsets[c + 1] = offsets[c] + static_cast<GLuint>(chunk.size());
		++c;
	}
	return indices;
}

std::shared_ptr<DrawCall> MesostructureMvcRenderer::CreateWireframeDrawCall(const Model::TileType& tileType, bool includeDiagonals) {
	std::array<bool, 4> flip = { false, false, false, false };
	for (int i = 0; i < 4; ++i) {
		flip[i] = tileType.edges[i].transform.flipped;
	}

	std::vector<GLuint> elementData(2 * 5 * 4);
	for (int side = 0; side < 4; ++side) {
		GLuint offset = 4 * (side + 1);
		int c = 2 * 5 * side;
		elementData[c++] = offset + 0;
		elementData[c++] = offset + 1;

		elementData[c++] = offset + 1;
		elementData[c++] = offset + 2;

		elementData[c++] = offset + 2;
		elementData[c++] = offset + 3;

		elementData[c++] = offset + 3;
		elementData[c++] = offset + 0;

		if (includeDiagonals) {
			if (flip[side]) {
				elementData[c++] = offset + 3;
				elementData[c++] = offset + 1;
			}
			else {
				elementData[c++] = offset + 0;
				elementData[c++] = offset + 2;
			}
		}
	}

	static const std::vector<GLfloat> pointData = {
		+0.5f, -0.5f, 0,
		+0.5f, +0.5f, 0,
		-0.5f, +0.5f, 0,
		-0.5f, -0.5f, 0,

		+0.5f, -0.5f, 0,
		+0.5f, +0.5f, 0,
		+0.5f, +0.5f, +0.5f,
		+0.5f, -0.5f, +0.5f,

		+0.5f, +0.5f, 0,
		-0.5f, +0.5f, 0,
		-0.5f, +0.5f, +0.5f,
		+0.5f, +0.5f, +0.5f,

		-0.5f, +0.5f, 0,
		-0.5f, -0.5f, 0,
		-0.5f, -0.5f, +0.5f,
		-0.5f, +0.5f, +0.5f,

		-0.5f, -0.5f, 0,
		+0.5f, -0.5f, 0,
		+0.5f, -0.5f, +0.5f,
		-0.5f, -0.5f, +0.5f,

		+0.5f, -0.5f, +0.5f,
		+0.5f, +0.5f, +0.5f,
		-0.5f, +0.5f, +0.5f,
		-0.5f, -0.5f, +0.5f,
	};

	auto drawCall = std::make_shared<DrawCall>("mesostructure-tile-wireframe", GL_LINES, true /* indexed */);
	drawCall->loadFromVectors(pointData, elementData);
	return drawCall;
}

#pragma endregion
//-----------------------------------------------------------------------------
#pragma region [Non static update steps]

void MesostructureMvcRenderer::updateProfileTextures(const Model::Tileset& model) {
	CsgCompiler::Options opts;
	opts.maxSegmentLength = properties().profilePrecision();

	EnsureCorrectVectorSizes(m_profileTextures, model.edgeTypes);

	// For each profile in the model
	for (int edgeTypeIndex = 0; edgeTypeIndex < model.edgeTypes.size(); ++edgeTypeIndex) {
		auto& profiles = model.edgeTypes[edgeTypeIndex]->shapes;
		for (int profileIndex = 0; profileIndex < profiles.size(); ++profileIndex) {

			// If there is no associated texture (potentially because it's been invalidated), allocate and upload it
			std::shared_ptr<Texture>& texture = m_profileTextures[edgeTypeIndex][profileIndex];
			if (!texture) {
				texture = CreateProfileTexture(profiles[profileIndex], opts);

				// for each sweep using this profile
				for (auto pair : SweepsDependingOnProfile(edgeTypeIndex, profileIndex, model.tileTypes, m_lut)) {
					if (!m_sweepDrawCalls.empty() && !m_sweepDrawCalls[pair.first].empty()) {
						m_sweepDrawCalls[pair.first][pair.second].reset();
					}
				}
			}
		}
	}
}

bool MesostructureMvcRenderer::updateSweepDrawCalls(
	const std::vector<std::vector<TilesetController::SweepGeometry>>& sweeps,
	const Model::Tileset& model,
	bool& sweepsCountChanged,
	bool& sweepsChanged
) {
	// If tangent multiplier changed, force recomputing sweep draw calls
	if (m_cachedProperties.tangentMultiplier != properties().tangentMultiplier
		|| m_cachedProperties.profileScale != properties().profileScale)
	{
		m_cachedProperties.tangentMultiplier = properties().tangentMultiplier;
		m_cachedProperties.profileScale = properties().profileScale;

		m_sweepDrawCalls.clear();
		m_sweepDrawCallIsDeformed.clear();
		m_sweepResolutions.clear();
	}

	sweepsCountChanged = EnsureCorrectVectorSizes(m_sweepDrawCalls, sweeps);
	EnsureCorrectVectorSizes(m_sweepDrawCallIsDeformed, sweeps);
	EnsureCorrectVectorSizes(m_sweepResolutions, sweeps);

	// For each sweep object
	sweepsChanged = false;
	bool areSweepPathsCached = false;
	for (int tileTypeIndex = 0; tileTypeIndex < model.tileTypes.size(); ++tileTypeIndex) {
		for (int sweepIndex = 0; sweepIndex < sweeps[tileTypeIndex].size(); ++sweepIndex) {

			// If there is no associated draw call (potentially because it's been invalidated),
			// allocate vao and synthesize procedural data.
			std::shared_ptr<DrawCall>& drawCall = m_sweepDrawCalls[tileTypeIndex][sweepIndex];
			if (!drawCall) {
				if (!areSweepPathsCached) {
					allocateSweepPaths(sweeps, model); // TODO: call this only when a sweep or tile is added/removed
					updateSweepPaths();
					areSweepPathsCached = true;
				}

				drawCall = createSweepDrawCall(tileTypeIndex, sweepIndex, sweeps[tileTypeIndex][sweepIndex], model);
				deformSweepDrawCall(tileTypeIndex, sweepIndex);
				m_sweepDrawCallIsDeformed[tileTypeIndex][sweepIndex] = false;
				sweepsChanged = true;
			}
		}
	}

	return sweepsCountChanged;
}

void MesostructureMvcRenderer::updateLut(const Model::Tileset& model) {
	m_lut.clear();
	m_lut[nullptr] = -1;
	for (int edgeTypeIndex = 0; edgeTypeIndex < model.edgeTypes.size(); ++edgeTypeIndex) {
		m_lut[model.edgeTypes[edgeTypeIndex]] = edgeTypeIndex;
	}
}

std::shared_ptr<DrawCall> MesostructureMvcRenderer::createSweepDrawCall(
	int tileTypeIndex,
	int sweepIndex,
	const TilesetController::SweepGeometry& sweepGeometry,
	const Model::Tileset& model
) {
	// sweepModel comes directly to the raw model data, the remainer of
	// sweepGeometry was computed dynamically by the controller
	const auto& sweepModel = sweepGeometry.model;

	bool isCap = sweepModel.tileEdgeTo == -1;
	glm::uvec2 size = ComputeSweepGridSize(sweepModel, *model.tileTypes[tileTypeIndex], m_profileTextures, m_lut, properties().sweepDivisions);
	m_sweepResolutions[tileTypeIndex][sweepIndex] = size;

	auto drawCall = std::make_shared<DrawCall>("mesostructure-tile-mesh-mvc", GL_TRIANGLE_STRIP, true /* indexed */);

	GLsizeiptr vboAttributeSize = 4 * sizeof(GLfloat) * size.x * size.y;

	drawCall->elementCount = (2 * size.x + 1) * (size.y - 1);
	drawCall->restart = true;
	drawCall->restartIndex = std::numeric_limits<GLuint>::max();

	drawCall->indexed = true;
	drawCall->elementCount = drawCall->elementCount;

	glCreateBuffers(1, &drawCall->vbo);
	glNamedBufferData(drawCall->vbo, 3 * vboAttributeSize, nullptr, GL_DYNAMIC_DRAW);

	glCreateBuffers(1, &drawCall->ebo);
	glNamedBufferData(drawCall->ebo, sizeof(GLuint) * drawCall->elementCount, nullptr, GL_DYNAMIC_DRAW);

	glCreateVertexArrays(1, &drawCall->vao);

	glVertexArrayAttribFormat(drawCall->vao, 0, 4, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(drawCall->vao, 0, 0);
	glVertexArrayVertexBuffer(drawCall->vao, 0, drawCall->vbo, 0, 4 * sizeof(GLfloat));
	glEnableVertexArrayAttrib(drawCall->vao, 0);

	glVertexArrayAttribFormat(drawCall->vao, 1, 4, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(drawCall->vao, 1, 1);
	glVertexArrayVertexBuffer(drawCall->vao, 1, drawCall->vbo, vboAttributeSize, 4 * sizeof(GLfloat));
	glEnableVertexArrayAttrib(drawCall->vao, 1);

	glVertexArrayAttribFormat(drawCall->vao, 2, 4, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(drawCall->vao, 2, 2);
	glVertexArrayVertexBuffer(drawCall->vao, 2, drawCall->vbo, 2 * vboAttributeSize, 4 * sizeof(GLfloat));
	glEnableVertexArrayAttrib(drawCall->vao, 2);

	glVertexArrayElementBuffer(drawCall->vao, drawCall->ebo);

	// Dispatch undeformed sweep compute
	m_buildSweepVaoShader->use();
	m_buildSweepVaoShader->setUniform("uSize", size);
	m_buildSweepVaoShader->setUniform("uIsCap", isCap);
	m_buildSweepVaoShader->setUniform("uStartFlip", sweepGeometry.startFlip);
	m_buildSweepVaoShader->setUniform("uEndFlip", sweepGeometry.endFlip);
	m_buildSweepVaoShader->setUniform("uStartPosition", sweepGeometry.startPosition);
	m_buildSweepVaoShader->setUniform("uEndPosition", sweepGeometry.endPosition);
	m_buildSweepVaoShader->setUniform("uAddHalfTurn", sweepModel.addHalfTurn);
	m_buildSweepVaoShader->setUniform("uAssignmentOffset", sweepModel.assignmentOffset);
	m_buildSweepVaoShader->setUniform("uFlipEndProfile", sweepModel.flipEnd);

	m_buildSweepVaoShader->setUniform("uNormalBufferStartOffset", size.x * size.y);
	m_buildSweepVaoShader->setUniform("uUvBufferStartOffset", 2 * size.x * size.y);
	m_buildSweepVaoShader->setUniform("uRestartIndex", std::numeric_limits<GLuint>::max());

	int startEdgeTypeIndex = m_lut[model.tileTypes[tileTypeIndex]->edges[sweepModel.tileEdgeFrom].type.lock()];
	m_buildSweepVaoShader->setUniform("uStartProfile", 0);
	m_profileTextures[startEdgeTypeIndex][sweepModel.shapeFrom]->bind(0);
	if (!isCap) {
		m_buildSweepVaoShader->setUniform("uEndProfile", 1);
		int endEdgeTypeIndex = m_lut[model.tileTypes[tileTypeIndex]->edges[sweepModel.tileEdgeTo].type.lock()];
		m_profileTextures[endEdgeTypeIndex][sweepModel.shapeTo]->bind(1);
	}

	m_buildSweepVaoShader->bindStorageBlock("SweepVbo", drawCall->vbo);
	m_buildSweepVaoShader->bindStorageBlock("SweepEbo", drawCall->ebo);
	m_buildSweepVaoShader->dispatch(size, uvec2(8));

#ifndef NDEBUG
	assert(drawCall->checkIntegrity(size.x * size.y));
#endif // NDEBUG

	return drawCall;
}

void MesostructureMvcRenderer::allocateSweepPaths(
	const std::vector<std::vector<TilesetController::SweepGeometry>>& sweeps,
	const Model::Tileset& model
) {
	// full info needed to build the control points, per sweep type
	std::vector<SweepBuildingInfo> sweepBuildingInfos;
	// full info needed to build the control points, per sweep instance (in this deformation mode, one instance per type)
	std::vector<SweepIndexInfo> sweepIndexInfos;
	// lighter info used at render time
	std::vector<GLuint> sweepInfos;

	m_sweepInstanceCount = 0;

	GLuint slotIndex = 0;
	GLuint globalSweepIndex = 0;
	for (GLuint tileTypeIndex = 0; tileTypeIndex < model.tileTypes.size(); ++tileTypeIndex) {
		GLuint sweepCount = static_cast<GLuint>(sweeps[tileTypeIndex].size());

		for (GLuint sweepIndex = 0; sweepIndex < sweepCount; ++sweepIndex) {
			auto& sweepGeometry = sweeps[tileTypeIndex][sweepIndex];
			bool isCap = sweepGeometry.model.tileEdgeTo == -1;
			GLuint flags = 0;
			if (isCap) flags = flags | 0x1;
			if (sweepGeometry.startFlip) flags = flags | 0x2;
			if (sweepGeometry.endFlip) flags = flags | 0x4;
			sweepBuildingInfos.push_back(SweepBuildingInfo{
				static_cast<GLuint>(sweepGeometry.model.tileEdgeFrom),
				static_cast<GLuint>(sweepGeometry.model.shapeFrom),
				static_cast<GLuint>(sweepGeometry.model.tileEdgeTo),
				static_cast<GLuint>(sweepGeometry.model.shapeTo),

				sweepGeometry.startPosition,
				sweepGeometry.endPosition,

				flags,
				0, 0, 0,
				});
		}

		for (GLuint sweepIndex = 0; sweepIndex < sweepCount; ++sweepIndex) {
			auto& sweepGeometry = sweeps[tileTypeIndex][sweepIndex];
			bool isCap = sweepGeometry.model.tileEdgeTo == -1;
			GLuint flags = 0;
			if (isCap) flags = flags | 0x1;

			sweepIndexInfos.push_back(SweepIndexInfo{
				slotIndex,
				tileTypeIndex,
				globalSweepIndex + sweepIndex,
				false
				});

			sweepInfos.push_back(flags);
		}

		++slotIndex;

		m_sweepInstanceCount += sweepCount;
		globalSweepIndex += sweepCount;
	}

	for (auto& ssbo : m_sweepControlPointSsbo) {
		ssbo.resize(4 * sizeof(GLfloat) * m_sweepInstanceCount);
	}

	m_sweepBuildingInfoSsbo.update(sweepBuildingInfos);
	m_sweepIndexInfoSsbo.update(sweepIndexInfos);
	m_sweepInfoSsbo.update(sweepInfos);

}

void MesostructureMvcRenderer::updateSweepPaths() {
	if (m_sweepInstanceCount == 0) return;
	// Dispatch control point compute
	const auto& shader = *m_buildSweepControlPointsShader;
	shader.use();
	autoSetUniforms(shader, properties());
	shader.setUniform("uSweepCount", m_sweepInstanceCount);
	shader.bindStorageBlock("SweepBuildingInfos", m_sweepBuildingInfoSsbo.raw);
	shader.bindStorageBlock("SweepIndexInfos", m_sweepIndexInfoSsbo.raw);
	for (int k = 0; k < 4; ++k) {
		shader.bindStorageBlock("SweepControlPoint" + std::to_string(k), m_sweepControlPointSsbo[k].raw);
	}
	shader.dispatch(m_sweepInstanceCount, 8);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void MesostructureMvcRenderer::deformSweepDrawCall(int tileTypeIndex, int sweepIndex, GLuint sweepControlPointOffset) {
	//if (m_sweepDrawCallIsDeformed[tileTypeIndex][sweepIndex]) return;
	std::shared_ptr<DrawCall>& drawCall = m_sweepDrawCalls[tileTypeIndex][sweepIndex];
	assert(drawCall != nullptr);

	if (sweepControlPointOffset == -1) {
		sweepControlPointOffset = 0;
		for (int i = 0; i < tileTypeIndex; ++i) {
			sweepControlPointOffset += static_cast<GLuint>(m_sweepDrawCalls[tileTypeIndex].size());
		}
		sweepControlPointOffset += sweepIndex;
	}

	uvec2 size = m_sweepResolutions[tileTypeIndex][sweepIndex];

	const auto& shader = *m_deformSweepVaoShader;
	shader.use();
	autoSetUniforms(shader, properties());
	shader.setUniform("uSize", size);
	shader.setUniform("uSweepControlPointOffset", sweepControlPointOffset);
	shader.setUniform("uNormalBufferStartOffset", size.x * size.y);
	shader.bindStorageBlock("SweepVbo", drawCall->vbo);
	shader.bindStorageBlock("SweepInfos", m_sweepInfoSsbo.raw);
	for (int k = 0; k < 4; ++k) {
		shader.bindStorageBlock("SweepControlPoint" + std::to_string(k), m_sweepControlPointSsbo[k].raw);
	}
	shader.dispatch(size, uvec2(8));

	m_sweepDrawCallIsDeformed[tileTypeIndex][sweepIndex] = true;
}

void MesostructureMvcRenderer::updateWireframeDrawCalls(const Model::Tileset& model) {
	if (m_cachedProperties.wireframeDiagonal != properties().wireframeDiagonal) {
		m_wireframeDrawCalls.clear();
		m_cachedProperties.wireframeDiagonal = properties().wireframeDiagonal;
	}

	if (m_wireframeDrawCalls.empty()) {
		m_wireframeDrawCalls.resize(model.tileTypes.size());
	}

	assert(m_wireframeDrawCalls.size() == model.tileTypes.size());

	for (int i = 0; i < m_wireframeDrawCalls.size(); ++i) {
		auto& drawCall = m_wireframeDrawCalls[i];
		if (!drawCall) {
			drawCall = CreateWireframeDrawCall(*model.tileTypes[i], properties().wireframeDiagonal);
		}
	}
}

#pragma endregion
//-----------------------------------------------------------------------------
