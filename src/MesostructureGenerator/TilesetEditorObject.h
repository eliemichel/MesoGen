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
#include "TilesetRenderer.h"

#include <DrawCall.h>
#include <ReflectionAttributes.h>
#include <Ray.h>
#include <refl.hpp>
#include <glm/glm.hpp>

#include <vector>
#include <memory>

class TilesetController;
class MesostructureRenderer;
namespace Model{
struct Tileset;
}

class TilesetEditorObject : public RuntimeObject {
public:
	enum class Brush {
		Circle,
		Square,
	};
	enum class Operation {
		Add,
		Substract,
		Remove,
	};

public:
	TilesetEditorObject();
	TilesetEditorObject(const TilesetEditorObject&) = delete;
	TilesetEditorObject& operator=(const TilesetEditorObject&) = delete;

	void setController(std::weak_ptr<TilesetController> controller);
	void setRenderer(std::weak_ptr<TilesetRenderer> renderer);
	void setMesostructureRenderer(std::weak_ptr<MesostructureRenderer> mesostructureRenderer);

	std::weak_ptr<TilesetRenderer> renderer() const { return m_renderer; }

	void update() override;

	void render(const Camera& camera) const override;

	void onMouseScroll(float scroll);
	void onMousePress();
	void onMouseRelease();
	void onCursorPosition(float x, float y, const Camera& camera);

	// Get the position of a tile in the canvas by its index
	static glm::vec3 GetTileCenter(int tileIndex);

private:
	void setCursorUniforms(const ShaderProgram& shader) const;
	void rebuild();
	void rebuildCursorDrawCalls();

	void testPathHit(std::vector<std::vector<TilesetRenderer::Path>>& paths, const Ray& ray, float& minLambda, float& hitLambda);
	void testInterfaceHit(const Ray& ray, float& minLambda, float& hitLambda);

	void onMousePressOnInterface();
	void onMousePressOnPath();

public:
	struct Properties {
		Operation operation = Operation::Add;
		Brush brush = Brush::Circle;
		float brushScale = 0.1f;
		glm::vec2 brushSize = glm::vec2(1.0f);
		float brushAngle = 0.0f;
		float brushSizeSensitivity = 0.05f;
		float lineThickness = 0.025f;
	};
	const Properties& properties() const { return m_properties; }
	Properties& properties() { return m_properties; }

private:
	Properties m_properties;
	std::weak_ptr<TilesetController> m_controller;
	std::weak_ptr<TilesetRenderer> m_renderer;
	std::weak_ptr<MesostructureRenderer> m_mesostructureRenderer;

	QuadObject m_cursor;
	glm::mat4 m_brushMatrix;
	DrawCall m_circleCursorDrawCall;
	DrawCall m_squareCursorDrawCall;

	// This camera is not used for rendering, it is only used for orbiting the tiles
	std::shared_ptr<Camera> m_camera;

	// Structure used to remember which face of which tile the mouse is currently hovering
	struct TileEdgeIdentifier {
		bool isValid = false;
		int tileIndex;
		Model::TileEdge::Direction tileEdge;
		glm::vec2 localCoord; // in [0,1]²
	};
	// Corresponds to indices in m_paths
	struct PathIdentifier {
		bool isValid = false;
		int tileIndex;
		int pathIndex; // within the tile
	};
	// Info about what the mouse is currently hovering
	// NB: Only one of teh field is valid at the same time
	struct MouseHit {
		TileEdgeIdentifier hitTileEdge;
		PathIdentifier hitPath;
	};
	MouseHit m_mouseHit;
};

#define _ ReflectionAttributes::
REFL_TYPE(TilesetEditorObject::Properties)
REFL_FIELD(operation)
REFL_FIELD(brush)
REFL_FIELD(brushScale, _ Range(1e-2f, 2.0f))
REFL_FIELD(brushSize, _ Range(1e-2f, 2.0f))
REFL_FIELD(brushAngle, _ Range(-180.0f, 180.0f))
REFL_FIELD(brushSizeSensitivity, _ Range(1e-6f, 0.1f))
REFL_FIELD(lineThickness, _ Range(1e-4f, 0.3f))
REFL_END
#undef _
