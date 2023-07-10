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
#include "ModelDialog.h"
#include "Model.h"
#include "TilesetController.h"
#include "Serialization.h"

#include <utils/reflutils.h>

#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

void ModelDialog::draw()
{
	if (auto cont = m_cont.lock()) {
		if (auto model = cont->lockModel()) {
			if (ImGui::CollapsingHeader("Controller", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::PushID(0);
				autoUi(cont->properties());

				const auto& filename = cont->properties().tilesetFilename;
				if (ImGui::Button("Load")) {
					Serialization::deserializeFrom(model, filename.c_str());
					cont->setDirty();
				}
				ImGui::SameLine();
				if (ImGui::Button("Save")) {
					Serialization::serializeTo(model, filename.c_str());
				}
				ImGui::PopID();
			}

			if (ImGui::CollapsingHeader("Edge Types", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::PushID(1);
				drawEdgeTypes(*cont, *model);
				ImGui::PopID();
			}

			if (ImGui::CollapsingHeader("Tile Types", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::PushID(2);
				drawTileTypes(*cont, *model);
				ImGui::PopID();
			}
		}
	}
}

void ModelDialog::drawEdgeTypes(TilesetController& controller, Model::Tileset& model) {
	ImGui::Text("%d elements:", model.edgeTypes.size());

	int i = -1;
	int toRemove = -1;
	for (const auto& edgeType : model.edgeTypes) {
		++i;
		ImGui::PushID(i);
		bool expanded = ImGui::TreeNode("");
		ImGui::SameLine();
		ImGui::Text("#%d (%d shapes)", i, edgeType->shapes.size());
		ImGui::SameLine();
		float* color = glm::value_ptr(edgeType->color);
		if (ImGui::ColorButton("#open-color-popup", ImVec4(color[0], color[1], color[2], 1))) {
			ImGui::OpenPopup("color picker");
		}
		if (ImGui::BeginPopup("color picker"))
		{
			if (ImGui::ColorPicker3("", glm::value_ptr(edgeType->color))) {
				edgeType->isExplicit = true;
			}
			ImGui::EndPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Remove")) {
			toRemove = i;
		}

		if (expanded) {
			ImGui::Checkbox("Is Explicit", &edgeType->isExplicit);
			ImGui::Checkbox("Border Edge", &edgeType->borderEdge);
			if (edgeType->borderEdge) {
				ImGui::Checkbox("Border Only", &edgeType->borderOnly);
			}

			ImGui::Text("Shapes:");
			int j = -1;
			for (auto& shape : edgeType->shapes) {
				++j;
				ImGui::PushID(j);
				bool shapeExpanded = ImGui::TreeNode("");
				ImGui::SameLine();
				ImGui::Text(" - type=%d", shape.csg.index());
				ImGui::SameLine();
				if (ImGui::Button("Remove")) {
					controller.removeEdgeShape(*edgeType, j);
				}
				if (shapeExpanded) {
					float* color = glm::value_ptr(shape.color);
					if (ImGui::ColorButton("Base Color #open-color-popup", ImVec4(color[0], color[1], color[2], color[3]))) {
						ImGui::OpenPopup("color picker");
					}
					if (ImGui::BeginPopup("color picker"))
					{
						ImGui::ColorPicker4("", color);
						ImGui::EndPopup();
					}

					ImGui::SliderFloat("Roughness", &shape.roughness, 0.0f, 1.0f);
					ImGui::SliderFloat("Metallic", &shape.metallic, 0.0f, 1.0f);

					ImGui::TreePop();
				}
				ImGui::PopID();
			}

			ImGui::TreePop();
		}

		ImGui::PopID();
	}

	if (toRemove != -1) {
		controller.removeEdgeType(toRemove);
	}

	if (ImGui::Button("Add")) {
		controller.addEdgeType();
	}
}

void ModelDialog::drawTileTypes(TilesetController& controller, Model::Tileset& model) {
	drawTileTransform("Default Allowed Transform", model.defaultTileTransformPermission);

	ImGui::Text("%d elements:", model.tileTypes.size());

	std::map<std::shared_ptr<Model::EdgeType>, int> edgeTypeAddressToIndex;
	std::vector<std::string> edgeTypeStringItems(model.edgeTypes.size() + 1);
	std::vector<const char*> edgeTypeItems(model.edgeTypes.size() + 1);
	int i = -1;
	for (const auto& edgeType : model.edgeTypes) {
		++i;
		edgeTypeAddressToIndex[edgeType] = i;
		edgeTypeStringItems[i + 1] = std::string("#") + std::to_string(i);
		edgeTypeItems[i + 1] = edgeTypeStringItems[i + 1].c_str();
	}
	edgeTypeStringItems[0] = "(none)";
	edgeTypeItems[0] = edgeTypeStringItems[0].c_str();

	i = -1;
	int toRemove = -1;
	int toCopy = -1;
	for (const auto& tileType : model.tileTypes) {
		++i;
		ImGui::PushID(i);
		bool expanded = ImGui::TreeNode("");
		ImGui::SameLine();
		ImGui::Text("#%d", i);
		ImGui::SameLine();
		if (ImGui::Button("Copy")) {
			toCopy = i;
		}
		ImGui::SameLine();
		if (ImGui::Button("Remove")) {
			toRemove = i;
		}
		if (expanded) {
			ImGui::Checkbox("Ignore", &tileType->ignore);
			
			if (drawTileTransform("Allowed Transform", tileType->transformPermission)) {
				controller.setDirty();
			}

			if (drawTileSweeps("Sweeps", controller, *tileType)) {
				controller.setDirty();
			}

			if (drawTileGeometry("Geometry", controller, *tileType)) {
				controller.setDirty();
			}

			int changedEdgeType = -1;
			int newEdgeTypeIndex = -1;
			for (int j = 0; j < Model::TileEdge::Count; ++j) {
				auto& edge = tileType->edges[j];
				int edgeTypeIndex = -1;
				if (auto edgeTypePtr = edge.type.lock()) edgeTypeIndex = edgeTypeAddressToIndex[edgeTypePtr];
				++edgeTypeIndex;

				ImGui::PushID(j);
				ImGui::Text("Edge #%d: type", j);
				ImGui::SameLine();
				ImGui::SetNextItemWidth(ImGui::GetFontSize() * 6);
				if (ImGui::Combo("", &edgeTypeIndex, edgeTypeItems.data(), static_cast<int>(edgeTypeItems.size()))) {
					changedEdgeType = j;
					newEdgeTypeIndex = edgeTypeIndex - 1;
				}
				ImGui::SameLine();
				if (ImGui::Checkbox("flipped", &edge.transform.flipped)) {
					controller.setDirty();
				}
				ImGui::PopID();
			}
			if (changedEdgeType != -1) {
				auto& edge = tileType->edges[changedEdgeType];
				auto newEdgeType =
					newEdgeTypeIndex >= 0
					? model.edgeTypes[newEdgeTypeIndex]
					: nullptr;
				controller.setTileEdgeType(edge, newEdgeType);
			}
			ImGui::TreePop();
		}
		ImGui::PopID();
	}

	if (toRemove != -1) {
		controller.removeTileType(toRemove);
	}

	if (toCopy != -1) {
		controller.copyTileType(*model.tileTypes[toCopy]);
	}

	if (ImGui::Button("Add")) {
		controller.addTileType();
	}
}

bool ModelDialog::drawTileTransform(const char* name, Model::TileTransformPermission& transform) {
	bool changed = false;
	if (ImGui::TreeNode(name)) {
		changed = ImGui::Checkbox("Rotation", &transform.rotation) || changed;
		changed = ImGui::Checkbox("Flip X", &transform.flipX) || changed;
		changed = ImGui::Checkbox("Flip Y", &transform.flipY) || changed;
		ImGui::TreePop();
	}
	return changed;
}

bool ModelDialog::drawTileSweeps(const char* name, TilesetController& controller, Model::TileType& tileType) {
	bool changed = false;
	if (ImGui::TreeNode(name)) {
		int j = -1;
		for (auto& sw : tileType.innerPaths) {
			++j;
			ImGui::PushID(j);
			bool expanded = ImGui::TreeNode("");
			ImGui::SameLine();
			ImGui::Text(" - from %d:%d to %d:%d", sw.tileEdgeFrom, sw.shapeFrom, sw.tileEdgeTo, sw.shapeTo);
			ImGui::SameLine();
			if (ImGui::Button("Remove")) {
				controller.removeTileInnerPath(tileType, sw);
				changed = true;
			}
			if (expanded) {
				changed = ImGui::SliderInt("Assignment offset", &sw.assignmentOffset, 0, 100) || changed;
				changed = ImGui::Checkbox("Add half turn", &sw.addHalfTurn) || changed;
				changed = ImGui::Checkbox("Flip End", &sw.flipEnd) || changed;
				ImGui::TreePop();
			}
			ImGui::PopID();
		}
		ImGui::TreePop();
	}
	return changed;
}

bool ModelDialog::drawTileGeometry(const char* name, TilesetController& controller, Model::TileType& tileType) {
	bool changed = false;
	if (ImGui::TreeNode(name)) {
		int j = -1;
		for (auto& geo : tileType.innerGeometry) {
			++j;
			ImGui::PushID(j);
			bool expanded = ImGui::TreeNode("");
			ImGui::SameLine();
			geo.objFilename.reserve(256);
			bool filenameChanged = ImGui::InputText("filename", &geo.objFilename);
			if (filenameChanged) {
				// should we trigger obj reload from here?
			}
			changed = filenameChanged || changed;
			if (expanded) {
				changed = ImGui::DragFloat4("X", glm::value_ptr(geo.transform[0]), 0.1f) || changed;
				changed = ImGui::DragFloat4("Y", glm::value_ptr(geo.transform[1]), 0.1f) || changed;
				changed = ImGui::DragFloat4("Z", glm::value_ptr(geo.transform[2]), 0.1f) || changed;
				changed = ImGui::DragFloat4("W", glm::value_ptr(geo.transform[3]), 0.1f) || changed;

				float* color = glm::value_ptr(geo.color);
				if (ImGui::ColorButton("Base Color #open-color-popup", ImVec4(color[0], color[1], color[2], color[3]))) {
					ImGui::OpenPopup("color picker");
				}
				if (ImGui::BeginPopup("color picker"))
				{
					changed = ImGui::ColorPicker4("", color) || changed;
					ImGui::EndPopup();
				}

				changed = ImGui::SliderFloat("Roughness", &geo.roughness, 0.0f, 1.0f) || changed;
				changed = ImGui::SliderFloat("Metallic", &geo.metallic, 0.0f, 1.0f) || changed;

				if (ImGui::Button("Remove")) {
					controller.removeTileInnerGeometry(tileType, j);
					changed = true;
				}
				ImGui::TreePop();
			}
			ImGui::PopID();
		}
		if (ImGui::Button("Add")) {
			controller.addTileInnerGeometry(tileType);
		}
		ImGui::TreePop();
	}
	return changed;
}
