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
#include <OpenGL>

#include "MesostructureExporter.h"
#include "MesostructureData.h"
#include "MacrosurfaceData.h"
#include "MesostructureRenderer.h"
#include "BMesh.h"

#include <utils/reflutils.h>
#include <ShaderPool.h>
#include <Texture.h>
#include <Logger.h>
#include <glm/glm.hpp>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <bit>

using glm::vec4;
using glm::uvec2;
using glm::uvec4;

MesostructureExporter::MesostructureExporter(
	std::weak_ptr<MesostructureData> mesostructureData,
	std::weak_ptr<MesostructureRenderer> mesostructureRenderer
)
	: m_mesostructureData(mesostructureData)
	, m_mesostructureRenderer(mesostructureRenderer)
{
	m_exportShader = ShaderPool::GetShader("mesostructure-export", 0, false);
	m_exportShader->setType(ShaderProgram::ShaderProgramType::ComputeShader);
	m_exportShader->load();

	m_exportMvcShader = ShaderPool::GetShader("mesostructure-export-mvc", 0, false);
	m_exportMvcShader->setType(ShaderProgram::ShaderProgramType::ComputeShader);
	m_exportMvcShader->load();
}

bool MesostructureExporter::exportMesh() const {
	auto mesostructure = m_mesostructureData.lock(); if (!mesostructure) return false;
	auto renderer = m_mesostructureRenderer.lock(); if (!renderer) return false;
	auto macrosurface = mesostructure->macrosurface.lock(); if (!macrosurface) return false;
	auto& props = properties();

	if (renderer->properties().deformationMode == MesostructureRenderer::DeformationMode::PostGeneration) {
		return exportMeshMvc();
	}

	std::ofstream file;
	file.open(props.filename, std::ios::out);
	if (!file.is_open()) {
		WARN_LOG << "Could not open file '" << props.filename << "' for export!";
		return false;
	}
	file.close();

	const auto& shader = *m_exportShader;

	Ssbo outputPointSsbo;
	Ssbo outputNormalSsbo;
	Ssbo outputUvSsbo;
	std::vector<OutputDataBatch> outputData;

	bool wasAlwaysRecompute = renderer->properties().alwaysRecompute;
	renderer->properties().alwaysRecompute = true;
	renderer->update();
	renderer->properties().alwaysRecompute = wasAlwaysRecompute;
	const auto& unsort = renderer->sortedSlotIndices();

	autoSetUniforms(shader, properties());
	shader.setUniform("uModelMatrix", mesostructure->modelMatrix());
	shader.bindStorageBlock("SweepInfos", renderer->sweepInfoSsbo().raw);
	shader.bindStorageBlock("SweepControlPointOffset", renderer->sweepControlPointOffsetSsbo().raw);
	for (int k = 0; k < 4; ++k) {
		shader.bindStorageBlock("SweepControlPoint" + std::to_string(k), renderer->sweepControlPointSsbo()[k].raw);
	}

	for (int tileIndex = 0; tileIndex < mesostructure->model->tileTypes.size(); ++tileIndex) {
		int tileInstanceCount = renderer->instanceCount(tileIndex);
		if (tileInstanceCount == 0) continue;

		GLuint slotStartIndex = renderer->sortedSlotsOffsets()[tileIndex];
		shader.setUniform("uSlotStartIndex", slotStartIndex);

		for (GLuint sweepIndex = 0; sweepIndex < renderer->sweepDrawCalls()[tileIndex].size(); ++sweepIndex) {
			uvec2 size = renderer->computeSweepGridSize(tileIndex, sweepIndex);
			GLuint pointCount = size.x * size.y;
			if (pointCount == 0) continue;
			GLsizeiptr attributeSsboSize = static_cast<GLsizeiptr>(sizeof(vec4) * pointCount);
			GLsizeiptr outputAttributeSsboSize = tileInstanceCount * attributeSsboSize;

			auto rd = renderer->sweepDrawCalls()[tileIndex][sweepIndex];

#ifndef NDEBUG
			GLint vboSize = 0;
			glGetNamedBufferParameteriv(rd->vbo, GL_BUFFER_SIZE, &vboSize);
			assert(vboSize == 3 * attributeSsboSize);
#endif // NDEBUG

			outputPointSsbo.resize(std::max(outputPointSsbo.size, outputAttributeSsboSize));
			outputNormalSsbo.resize(std::max(outputNormalSsbo.size, outputAttributeSsboSize));
			outputUvSsbo.resize(std::max(outputUvSsbo.size, outputAttributeSsboSize));

			autoSetUniforms(shader, renderer->properties());
			shader.setUniform("uSweepIndex", sweepIndex);
			shader.setUniform("uPointCount", pointCount);
			shader.setUniform("uInstanceCount", static_cast<GLuint>(tileInstanceCount));
			shader.setUniform("uNormalBufferStartOffset", size.x * size.y);
			shader.setUniform("uUvBufferStartOffset", 2 * size.x * size.y);
			shader.bindStorageBlock("MacrosurfaceNormals", renderer->macrosurfaceNormalSsbo().raw);
			shader.bindStorageBlock("MacrosurfaceCorners", renderer->macrosurfaceCornerSsbo().raw);
			shader.bindStorageBlock("SweepVbo", rd->vbo);

			shader.bindStorageBlock("OutputPoints", outputPointSsbo.raw);
			shader.bindStorageBlock("OutputNormal", outputNormalSsbo.raw);
			shader.bindStorageBlock("OutputUv", outputUvSsbo.raw);

			shader.dispatch(uvec2(size.x * size.y, tileInstanceCount), uvec2(8));

			// Get output mesh data and write to file
			size_t outputPointCount = tileInstanceCount * pointCount;
			outputData.push_back(OutputDataBatch{});
			auto& batch = outputData.back();
			batch.pointData.resize(outputPointCount);
			batch.normalData.resize(outputPointCount);
			batch.uvData.resize(outputPointCount);
			batch.slotIndexData.resize(outputPointCount);
			glGetNamedBufferSubData(outputPointSsbo.raw, 0, sizeof(vec4) * outputPointCount, batch.pointData.data());
			glGetNamedBufferSubData(outputNormalSsbo.raw, 0, sizeof(vec4) * outputPointCount, batch.normalData.data());
			glGetNamedBufferSubData(outputUvSsbo.raw, 0, sizeof(vec4) * outputPointCount, batch.uvData.data());
			batch.elementData.resize((size.x - 1) * (size.y - 1) * tileInstanceCount);
			auto it = batch.elementData.begin();
			auto slotIndexIt = batch.slotIndexData.begin();
			for (GLuint k = 0; k < static_cast<GLuint>(tileInstanceCount); ++k) {
				int sortedSlotIndex = slotStartIndex + k;
				int slotIndex = unsort[sortedSlotIndex];
				fill(slotIndexIt, slotIndexIt + pointCount, slotIndex);
				slotIndexIt += pointCount;

				bool flipped = renderer->flippedSlot()[sortedSlotIndex];
				GLuint offset = k * size.x * size.y;
				for (GLuint j = 0; j < size.y - 1; ++j) {
					for (GLuint i = 0; i < size.x - 1; ++i) {
						if (flipped) {
							*it = offset + uvec4(
								j * size.x + i,
								(j + 1) * size.x + i,
								(j + 1) * size.x + (i + 1),
								j * size.x + (i + 1)
							);
						}
						else {
							*it = offset + uvec4(
								j * size.x + i,
								j * size.x + (i + 1),
								(j + 1) * size.x + (i + 1),
								(j + 1) * size.x + i
							);
						}

#ifndef NDEBUG
						for (int c = 0; c < 4; ++c) {
							assert((*it)[c] < outputPointCount);
						}
#endif // NDEBUG
						++it;
					}
				}
			}
			assert(it == batch.elementData.end());
		}
	}

	/*
	if (props.filename.ends_with(".obj")) {
		return WriteObj(props.filename, outputData);
	}
	else if (props.filename.ends_with(".ply")) {
	*/
		return writePly(props.filename, outputData);
	/*
	}
	else {
		return false;
	}
	*/
}

bool MesostructureExporter::exportMeshMvc() const {
	auto mesostructure = m_mesostructureData.lock(); if (!mesostructure) return false;
	auto renderer = m_mesostructureRenderer.lock(); if (!renderer) return false;
	auto macrosurface = mesostructure->macrosurface.lock(); if (!macrosurface) return false;
	auto& props = properties();

	std::ofstream file;
	file.open(props.filename, std::ios::out);
	if (!file.is_open()) {
		WARN_LOG << "Could not open file '" << props.filename << "' for export!";
		return false;
	}
	file.close();

	const auto& shader = *m_exportMvcShader;

	Ssbo outputPointSsbo;
	Ssbo outputNormalSsbo;
	Ssbo outputUvSsbo;
	std::vector<OutputDataBatch> outputData;

	bool wasAlwaysRecompute = renderer->properties().alwaysRecompute;
	renderer->properties().alwaysRecompute = true;
	renderer->update();
	renderer->properties().alwaysRecompute = wasAlwaysRecompute;

	autoSetUniforms(shader, properties());
	shader.setUniform("uModelMatrix", mesostructure->modelMatrix());
	shader.bindStorageBlock("SweepInfos", renderer->sweepInfoSsbo().raw);
	for (int k = 0; k < 4; ++k) {
		shader.bindStorageBlock("SweepControlPoint" + std::to_string(k), renderer->sweepControlPointSsbo()[k].raw);
	}

	for (int tileIndex = 0; tileIndex < mesostructure->model->tileTypes.size(); ++tileIndex) {
		int tileInstanceCount = renderer->instanceCount(tileIndex);
		if (tileInstanceCount == 0) continue;

		GLuint slotStartIndex = renderer->sortedSlotsOffsets()[tileIndex];
		shader.setUniform("uSlotStartIndex", slotStartIndex);

		for (GLuint sweepIndex = 0; sweepIndex < renderer->sweepDrawCalls()[tileIndex].size(); ++sweepIndex) {
			uvec2 size = renderer->computeSweepGridSizeMvc(tileIndex, sweepIndex);
			GLuint pointCount = size.x * size.y;
			if (pointCount == 0) continue;
			GLsizeiptr attributeSsboSize = static_cast<GLsizeiptr>(sizeof(vec4) * pointCount);
			GLsizeiptr outputAttributeSsboSize = tileInstanceCount * attributeSsboSize;

			auto rd = renderer->sweepDrawCalls()[tileIndex][sweepIndex];

#ifndef NDEBUG
			GLint vboSize = 0;
			glGetNamedBufferParameteriv(rd->vbo, GL_BUFFER_SIZE, &vboSize);
			assert(vboSize == 3 * attributeSsboSize);
#endif // NDEBUG

			outputPointSsbo.resize(std::max(outputPointSsbo.size, outputAttributeSsboSize));
			outputNormalSsbo.resize(std::max(outputNormalSsbo.size, outputAttributeSsboSize));
			outputUvSsbo.resize(std::max(outputUvSsbo.size, outputAttributeSsboSize));

			autoSetUniforms(shader, renderer->properties());
			shader.setUniform("uSweepIndex", sweepIndex);
			shader.setUniform("uPointCount", pointCount);
			shader.setUniform("uInstanceCount", static_cast<GLuint>(tileInstanceCount));
			shader.setUniform("uNormalBufferStartOffset", size.x * size.y);
			shader.setUniform("uUvBufferStartOffset", 2 * size.x * size.y);
			shader.bindStorageBlock("MacrosurfaceCorners", renderer->macrosurfaceCornerSsbo().raw);
			shader.bindStorageBlock("MacrosurfaceNormals", renderer->macrosurfaceNormalSsbo().raw);
			shader.bindStorageBlock("SweepVbo", rd->vbo);

			shader.bindStorageBlock("OutputPoints", outputPointSsbo.raw);
			shader.bindStorageBlock("OutputNormal", outputNormalSsbo.raw);
			shader.bindStorageBlock("OutputUv", outputUvSsbo.raw);

			shader.dispatch(uvec2(size.x * size.y, tileInstanceCount), uvec2(8));

			// Get output mesh data and write to file
			size_t outputPointCount = tileInstanceCount * pointCount;
			outputData.push_back(OutputDataBatch{});
			auto& batch = outputData.back();
			batch.pointData.resize(outputPointCount);
			batch.normalData.resize(outputPointCount);
			batch.uvData.resize(outputPointCount);
			glGetNamedBufferSubData(outputPointSsbo.raw, 0, sizeof(vec4) * outputPointCount, batch.pointData.data());
			glGetNamedBufferSubData(outputNormalSsbo.raw, 0, sizeof(vec4) * outputPointCount, batch.normalData.data());
			glGetNamedBufferSubData(outputUvSsbo.raw, 0, sizeof(vec4) * outputPointCount, batch.uvData.data());
			batch.elementData.resize((size.x - 1) * (size.y - 1) * tileInstanceCount);
			auto it = batch.elementData.begin();
			for (GLuint k = 0; k < static_cast<GLuint>(tileInstanceCount); ++k) {
				bool flipped = renderer->flippedSlot()[slotStartIndex + k];
				GLuint offset = k * size.x * size.y;
				for (GLuint j = 0; j < size.y - 1; ++j) {
					for (GLuint i = 0; i < size.x - 1; ++i) {
						if (flipped) {
							*it = offset + uvec4(
								j * size.x + i,
								(j + 1) * size.x + i,
								(j + 1) * size.x + (i + 1),
								j * size.x + (i + 1)
							);
						}
						else {
							*it = offset + uvec4(
								j * size.x + i,
								j * size.x + (i + 1),
								(j + 1) * size.x + (i + 1),
								(j + 1) * size.x + i
							);
						}

#ifndef NDEBUG
						for (int c = 0; c < 4; ++c) {
							assert((*it)[c] < outputPointCount);
						}
#endif // NDEBUG
						++it;
					}
				}
			}
			assert(it == batch.elementData.end());
		}
	}

	return writePly(props.filename, outputData);
}

bool MesostructureExporter::WriteObj(const std::string& filename, const std::vector<OutputDataBatch>& outputData) {
	std::ofstream file;
	file.open(filename, std::ios::out);
	if (!file.is_open()) {
		WARN_LOG << "Could not open file '" << filename << "' for export!";
		return false;
	}

	GLuint totalOutputPointCount = 0;
	for (auto& batch : outputData) {
		for (auto& p : batch.pointData) {
			file << "v " << p.x << " " << p.y << " " << p.z << "\n";
		}
		totalOutputPointCount += static_cast<GLuint>(batch.pointData.size());
	}
	file << "# ---------------\n\n";
	for (auto& batch : outputData) {
		for (auto& n : batch.normalData) {
			file << "vn " << n.x << " " << n.y << " " << n.z << "\n";
		}
	}
	file << "\n# ---------------\n\n";
	for (auto& batch : outputData) {
		for (auto& t : batch.uvData) {
			file << "vt " << t.x << " " << t.y << "\n";
		}
	}
	file << "\n# ---------------\n\n";
	GLuint offset = 1u;
	for (auto& batch : outputData) {
		for (auto& q : batch.elementData) {
			uvec4 quad = q + offset;
#ifndef NDEBUG
			for (int c = 0; c < 4; ++c) {
				assert(quad[c] <= totalOutputPointCount);
			}
#endif // NDEBUG
			file
				<< "f " << quad.x << "/" << quad.x << "/" << quad.x
				<< " "  << quad.y << "/" << quad.y << "/" << quad.y
				<< " "  << quad.z << "/" << quad.z << "/" << quad.z
				<< " "  << quad.w << "/" << quad.w << "/" << quad.w
				<< "\n";
		}
		offset += static_cast<GLuint>(batch.pointData.size());
	}
	file.close();

	return true;
}

template <typename T>
static void writeFile(std::ofstream& file, const T& value) {
	file.write(reinterpret_cast<const char*>(&value), sizeof(T));
}
template <>
static void writeFile<std::string>(std::ofstream& file, const std::string& value) {
	file.write(value.c_str(), value.size());
}
static void writeFile(std::ofstream& file, const char* value) {
	file.write(value, strlen(value));
}

bool MesostructureExporter::writePly(
	const std::string& filename,
	const std::vector<OutputDataBatch>& outputData
) const {
	std::ofstream file;
	file.open(filename, std::ios::out | std::ios::binary);
	if (!file.is_open()) {
		WARN_LOG << "Could not open file '" << filename << "' for export!";
		return false;
	}
	const Properties& props = properties();

	// Global header
	writeFile(file, "ply\n");
	if constexpr (std::endian::native == std::endian::little)
		writeFile(file, "format binary_little_endian 1.0\n");
	else
		writeFile(file, "format binary_big_endian 1.0\n");
	writeFile(file, "comment Exported from Elie Michel's MesostructureGenerator\n");

	// Vertices header
	size_t totalOutputPointCount = 0;
	size_t totalOutputFaceCount = 0;
	for (auto& batch : outputData) {
		totalOutputPointCount += batch.pointData.size();
		totalOutputFaceCount += batch.elementData.size();
	}
	writeFile(file, "element vertex " + std::to_string(totalOutputPointCount) + "\n");
	writeFile(file, "property float x\n");
	writeFile(file, "property float y\n");
	writeFile(file, "property float z\n");
	writeFile(file, "property float nx\n");
	writeFile(file, "property float ny\n");
	writeFile(file, "property float nz\n");
	writeFile(file, "property float u\n");
	writeFile(file, "property float v\n");

	if (props.exportSlotIndex) {
		writeFile(file, "property int slotIndex\n");
	}

	// Face header
	writeFile(file, "element face " + std::to_string(totalOutputFaceCount) + "\n");
	writeFile(file, "property list uchar uint vertex_index\n");

	writeFile(file, "end_header\n");

	// Vertex data
	for (auto& batch : outputData) {
		auto positionIt = batch.pointData.cbegin();
		auto normalIt = batch.normalData.cbegin();
		auto uvIt = batch.uvData.cbegin();
		auto positionEnd = batch.pointData.cend();
		auto slotIndexIt = batch.slotIndexData.begin();
		for (; positionIt != positionEnd; ++positionIt, ++normalIt, ++uvIt) {
			writeFile(file, positionIt->x);
			writeFile(file, positionIt->y);
			writeFile(file, positionIt->z);
			writeFile(file, normalIt->x);
			writeFile(file, normalIt->y);
			writeFile(file, normalIt->z);
			writeFile(file, uvIt->x);
			writeFile(file, uvIt->y);

			if (props.exportSlotIndex) {
				writeFile(file, *slotIndexIt);
				++slotIndexIt;
			}
		}
		totalOutputPointCount += static_cast<GLuint>(batch.pointData.size());
	}

	// Face data
	GLuint offset = 0u;
	for (auto& batch : outputData) {
		for (const auto& quad : batch.elementData) {
			constexpr unsigned char s = 4;
			writeFile(file, s);
			writeFile(file, quad.x + offset);
			writeFile(file, quad.y + offset);
			writeFile(file, quad.z + offset);
			writeFile(file, quad.w + offset);
		}
		offset += static_cast<GLuint>(batch.pointData.size());
	}
	
	file.close();

	return true;
}
