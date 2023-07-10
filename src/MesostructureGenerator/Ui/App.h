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

#include <Ui/BaseApp.h>
#include <Ray.h>
#include <RenderTarget.h>
#include <DeferredRendering.h>

#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/document.h>
#include <glm/vec2.hpp>

#include <memory>
#include <vector>

class RuntimeObject;
class QuadObject;
class TurntableCamera;
class TilesetEditorObject;
class TilesetController;
class TilesetRenderer;
class OutputObject;
class MacrosurfaceData;
class MesostructureData;
class MacrosurfaceRenderer;
class MesostructureRenderer;
class TilingSolver;
class TilingSolverRenderer;
class MesostructureExporter;
class MesostructureMvcRenderer;
class RenderTarget;
class FileRenderer;
namespace Model {
struct Tileset;
}
struct ImFont;

class App : public BaseApp
{
public:
	App(std::shared_ptr<Window> window);

	void update() override;
	void render() const override;

	using Document = rapidjson::Document;
	using Value = rapidjson::Value;
	void deserialize(const Document& document, const Value& json);
	void serialize(Document& document, Value& json) const;

	// Only used by the SimpleDialog
	std::shared_ptr<TilesetController> tilesetController() { return m_tilesetController; }
	std::shared_ptr<MacrosurfaceData> macrosurfaceData() { return m_macrosurfaceData; }
	std::shared_ptr<TilingSolver> tilingSolver() { return m_tilingSolver; }
	std::shared_ptr<MesostructureRenderer> mesostructureRenderer() { return m_mesostructureRenderer; }
	std::shared_ptr<MacrosurfaceRenderer> macrosurfaceRenderer() { return m_macrosurfaceRenderer; }

	// Only used by the FileRenderer
	RenderTarget& gbuffer()  { return *m_gbuffer; }
	DeferredRendering& deferredRendering() { return *m_deferredRendering; }
	const std::vector<std::shared_ptr<RuntimeObject>>& objects() const { return m_objects; }
	const TurntableCamera& camera() const { return *m_camera; }
	TurntableCamera& camera() { return *m_camera; }
	OutputObject& outputSlots() { return *m_outputSlots; }

protected:
	void setupDialogs() override;

	void onResize(int width, int height) override;
	void onMouseButton(int button, int action, int mods) override;
	void onKey(int key, int scancode, int action, int mods) override;
	void onCursorPosition(double x, double y) override;
	void onScroll(double xoffset, double yoffset) override;

private:
	void afterUpdateMousePosition();
	void saveModel();
	void loadModel();
	void printOutputCameraMatrix();

private:
	static bool hitGround(const Ray& ray, glm::vec3& hit);

private:
	std::shared_ptr<TilesetEditorObject> m_tilesetEditor;

	std::shared_ptr<QuadObject> m_cursorObject;

	std::vector<std::shared_ptr<RuntimeObject>> m_objects;
	std::shared_ptr<TurntableCamera> m_camera;

	glm::vec3 m_mousePosition;
	Ray m_mouseRay;

	std::shared_ptr<TilesetController> m_tilesetController;
	std::shared_ptr<Model::Tileset> m_tileset;

	std::shared_ptr<MacrosurfaceData> m_macrosurfaceData;
	std::shared_ptr<MesostructureData> m_mesostructureData;

	std::shared_ptr<MacrosurfaceRenderer> m_macrosurfaceRenderer;
	std::shared_ptr<MesostructureRenderer> m_mesostructureRenderer;

	std::shared_ptr<OutputObject> m_outputSlots;
	std::shared_ptr<TilesetRenderer> m_tilesetRenderer;

	std::shared_ptr<TilingSolver> m_tilingSolver;
	std::shared_ptr<TilingSolverRenderer> m_tilingSolverRenderer;

	std::shared_ptr<MesostructureExporter> m_mesostructureExporter;

	std::shared_ptr<MesostructureMvcRenderer> m_mesostructureMvcRenderer;

	std::unique_ptr<RenderTarget> m_gbuffer;
	std::unique_ptr<DeferredRendering> m_deferredRendering;

	std::shared_ptr<FileRenderer> m_fileRenderer;
};
