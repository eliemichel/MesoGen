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
#include "SimpleDialog.h"
#include "App.h"
#include "TilesetController.h"
#include "MacrosurfaceData.h"
#include "Serialization.h"
#include "Serialization/SerializationScene.h"
#include "TilingSolver.h"
#include "MesostructureRenderer.h"
#include "MacrosurfaceRenderer.h"

#include <utils/reflutils.h>

#include <imgui.h>

void SimpleDialog::draw()
{
	if (auto app = m_cont) {
		if (ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::PushID(-1);

			static std::string filename = "default.json";
			ImGui::InputText("Scene file", &filename);

			if (ImGui::Button("Load")) {
				Serialization::deserializeFrom(*app, filename.c_str());
			}
			ImGui::SameLine();
			if (ImGui::Button("Save")) {
				Serialization::serializeTo(*app, filename.c_str());
			}
			ImGui::PopID();
		}

		if (ImGui::CollapsingHeader("Model", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::PushID(0);
			auto cont = app->tilesetController();
			if (auto model = cont->lockModel()) {
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
			}
			ImGui::PopID();
		}

		if (ImGui::CollapsingHeader("Macrosurface", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::PushID(1);
			auto cont = app->macrosurfaceData();
			auto& props = cont->properties();
			
			props.modelFilename.reserve(256);
			ImGui::InputText("Path", props.modelFilename.data(), 256);

			if (ImGui::Button("Load")) {
				cont->loadMesh();
			}
			ImGui::PopID();
		}

		if (ImGui::CollapsingHeader("Generator", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::PushID(2);
			auto cont = app->tilingSolver();

			if (ImGui::Button("Generate")) {
				cont->generate();
			}

			if (!cont->message().empty()) {
				ImGui::Text(cont->message().c_str());
			}

			if (cont->tilingSolverData()->solver) {
				if (cont->message() == "Generation failed") {
					if (ImGui::Button("Compute Tile Suggestion")) {
						cont->computeTileSuggestion();
					}
				}
			}
			ImGui::PopID();
		}

		if (ImGui::CollapsingHeader("Macrosurface Renderer")) {
			ImGui::PushID(3);
			{
				auto cont = app->macrosurfaceRenderer();
				auto& props = cont->properties();
				ImGui::Checkbox("Enabled", &props.enabled);
			}

			{
				auto cont = app->macrosurfaceData();
				auto& props = cont->properties();
				ImGui::SliderFloat2("Viewport Position", glm::value_ptr(props.viewportPosition), -5, 5, "%.5f");
				ImGui::SliderFloat("Viewport Scale", &props.viewportScale, 0.01f, 5, "%.5f");
			}

			ImGui::PopID();
		}

		if (ImGui::CollapsingHeader("Mesostructure Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::PushID(4);
			auto cont = app->mesostructureRenderer();
			auto& props = cont->properties();

			ImGui::Checkbox("Enabled", &props.enabled);
			ImGui::SliderFloat("Thickness", &props.thickness, 0.0f, 1.0f, "%.5f");
			ImGui::SliderFloat("Offset", &props.offset, -1.0f, 1.0f, "%.5f");
			ImGui::SliderFloat("Profile Scale", &props.profileScale, 0.0f, 5.0f, "%.5f");
			ImGui::SliderFloat("Tangent Multiplier", &props.tangentMultiplier, 0.0f, 5.0f, "%.5f");
			ImGui::Checkbox("Wireframe", &props.wireframe);

			ImGui::PopID();
		}
	}
}
