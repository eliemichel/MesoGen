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

#include "VersionCounter.h"

#include <glm/glm.hpp>
#include <refl.hpp>
#include <ReflectionAttributes.h>
#include <NonCopyable.h>

#include <string>
#include <memory>

namespace bmesh {
struct BMesh;
}
namespace libwfc {
class MeshSlotTopology;
}
struct DrawCall;

class MacrosurfaceData : public NonCopyable {
public:
	enum class SlotTopology {
		Grid,
		Mesh,
	};

public:
	MacrosurfaceData();

	// Use either one of these:
	bool loadMesh();
	void loadBMesh(std::shared_ptr<bmesh::BMesh> bm);
	void generateGrid();
	void disableDrawCall(); // for headless tests

	bool ready() const;

	// Accessors
	const VersionCounter& version() const { return m_version; }
	std::shared_ptr<bmesh::BMesh> bmesh() const { return m_bmesh; }
	std::shared_ptr<DrawCall> meshDrawCall() const { return m_meshDrawCall; }
	std::shared_ptr<libwfc::MeshSlotTopology> slotTopology() const { return m_slotTopology; }
	const glm::mat4 & modelMatrix() const { return m_modelMatrix; }

	void setModelMatrix(const glm::mat4&& modelMatrix) { m_modelMatrix = modelMatrix; }
	void setModelMatrix(const glm::mat4& modelMatrix) { m_modelMatrix = modelMatrix; }

public:
	struct Properties {
		SlotTopology slotTopology = SlotTopology::Mesh;
		glm::ivec2 dimensions = glm::ivec2(4, 3);
		std::string modelFilename = "basket.obj";
		glm::vec2 viewportPosition = glm::vec2(1.0f, -2.0f);
		float viewportScale = 1.0f;
	};
	const Properties& properties() const { return m_properties; }
	Properties& properties() { return m_properties; }

private:
	Properties m_properties;

	/**
	 * Signaling system for the mesostructure and render data to get updated
	 */
	VersionCounter m_version;

	/**
	 * The root source of information about the macrosurface (mesh)
	 */
	std::shared_ptr<bmesh::BMesh> m_bmesh;

	/**
	 * A draw call that is set to draw the mesh onto which the mesostructure
	 * has been generated.
	 */
	std::shared_ptr<DrawCall> m_meshDrawCall;

	/**
	 * Slot topology derived from the mesh
	 */
	std::shared_ptr<libwfc::MeshSlotTopology> m_slotTopology;

	/**
	 * A global transform matrix to move/rotate/scale the whole object
	 */
	glm::mat4 m_modelMatrix;
};

#define _ ReflectionAttributes::
REFL_TYPE(MacrosurfaceData::Properties)
REFL_FIELD(slotTopology)
REFL_FIELD(dimensions, _ Range(0, 100)) //, _ HideInDialog())
REFL_FIELD(modelFilename, _ MaximumSize(256)) //, _ HideInDialog())
REFL_FIELD(viewportPosition, _ Range(-5, 5));
REFL_FIELD(viewportScale, _ Range(0.01f, 5));
REFL_END
#undef _
