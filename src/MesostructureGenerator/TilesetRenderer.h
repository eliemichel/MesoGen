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

#include "RuntimeObject.h"
#include "QuadObject.h"
#include "Model.h"
#include "Ssbo.h"

#include <NonCopyable.h>
#include <DrawCall.h>
#include <ReflectionAttributes.h>
#include <refl.hpp>
#include <glm/glm.hpp>

#include <vector>
#include <memory>

class TilesetController;
class MacrosurfaceData;
class MesostructureData;
class MesostructureRenderer;
class RenderTarget;
namespace Model{
struct Tileset;
}

/**
 * This is used only for the editable tiles (the non deformed ones)
 * It wraps around the MesostructureRenderer the drawing of profiles and edge
 * type/orientation indicators.
 */
class TilesetRenderer : public NonCopyable {
public:
	/**
	 * Path Slices tell which slice of the m_pathDrawCall corresponds to which tileType
	 * It is also used in activePathsSsbo and so replicates the PathSlice struct of tile25d shader
	 */
	struct PathSlice {
		GLuint start;
		GLuint size;
	};

	/**
	 * For each tileType, a vector of potential paths
	 * NB: This holds the same data than m_pathDrawCall, though the latter is in
	 * VRAM while this is in RAM for mouse collision detection
	 */
	struct Path {
		std::vector<glm::vec3> points;
		bool hover = false;
		bool active = false;
		PathSlice slice; // a subslice of m_pathSlices with only this path
		Model::TileInnerPath model;
	};

public:
	TilesetRenderer();
	
	void setController(std::shared_ptr<TilesetController> controller);

	std::vector<std::vector<Path>>& paths() { return m_paths; }
	const std::vector<std::vector<Path>>& paths() const { return m_paths; }

	// TO be called when the model has been updated
	void rebuild();
	// Rebuild path, both as a GPU-side ssbo and as a CPU side array for interaction
	void rebuildPotentialPaths();
	void rebuildMockMesostructure();
	void rebuildShapesDrawCall();

	void update();

	void renderTile(
		int tileIndex,
		const Camera& camera,
		const glm::mat4& modelMatrix
	) const;

private:
	void renderEdgeTypeMarker(const Model::TileType& tile, int tileEdgeIndex, const Camera& camera, const glm::mat4& modelMatrix) const;

public:
	struct Properties {
		bool enable = true;
		bool drawCrossSections = true;
		bool drawTrajectories = true;
		bool drawLabels = true;
		bool drawCage = true;
	};
	const Properties& properties() const { return m_properties; }
	Properties& properties() { return m_properties; }

private:
	Properties m_properties;
	std::weak_ptr<TilesetController> m_controller;
	
	// This renderer holds a mock mesostructure that shares the same
	// tileset model than the real one but has a different macrosurface
	// that is just one face per tile type
	std::shared_ptr<MacrosurfaceData> m_macrosurfaceData;
	std::shared_ptr<MesostructureData> m_mesostructureData;
	std::shared_ptr<MesostructureRenderer> m_mesostructureRenderer;

	// Draw call for drawing potential paths
	DrawCall m_pathDrawCall;

	std::vector<PathSlice> m_pathSlices;

	// Per tile slices within m_activePathsSsbo
	std::vector<PathSlice> m_activePathSlices;
	
	std::vector<std::vector<Path>> m_paths;

	// The vbo of this draw call contains all tiles, the slice vector is a way to
	// render only one of them.
	DrawCall m_shapesDrawCall;
	std::vector<PathSlice> m_shapesSlices;

	mutable QuadObject m_quad;
};

#define _ ReflectionAttributes::
REFL_TYPE(TilesetRenderer::Properties)
REFL_FIELD(enable)
REFL_FIELD(drawCrossSections)
REFL_FIELD(drawTrajectories)
REFL_FIELD(drawLabels)
REFL_FIELD(drawCage)
REFL_END
#undef _
