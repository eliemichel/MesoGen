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


#include "GlobalTimerDialog.h"
#include "utils/guiutils.h"
#include "utils/strutils.h"
#include "utils/reflutils.h"

#include <imgui.h>

void GlobalTimerDialog::draw()
{
	auto cont = m_cont.lock();
	if (!cont) return;
	if (ImGui::CollapsingHeader("Timers (ms)", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (ImGui::Button("Reset All")) {
			cont->resetAllStats();
		}
		{
			int n = cont->frameStats().sampleCount;
			double avg = cont->frameStats().cumulatedTime;
			double avgGpu = cont->frameStats().cumulatedGpuTime;
			ImGui::Text("Frame: %.05f / %.05f (%d)", avg, avgGpu, n);
		}
		for (auto& s : cont->stats()) {
			//int n = s.second.sampleCount;
			double avg = s.second.cumulatedTime;
			double avgGpu = s.second.cumulatedGpuTime;
			//double offset = s.second.cumulatedFrameOffset;
			ImGui::Checkbox(MAKE_STR(s.first << ":").c_str(), &s.second.ui.visible);
			{
				// color tag
				const ImVec2 p = ImGui::GetCursorScreenPos();
				glm::vec3 c = s.second.ui.color;
				ImDrawList* draw_list = ImGui::GetWindowDrawList();
				float x = p.x, y = p.y - 3;
				draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + 20, y + 3), ImColor(c.r, c.g, c.b, 1.0f));
			}
			ImGui::Text("  %.05f / %.05f", avg, avgGpu);
		}
		autoUi(cont->properties());
	}
}

void GlobalTimerDialog::drawHandles(float x, float y, float w, float h)
{
	auto cont = m_cont.lock();
	if (!cont) return;

	GlobalTimer::Properties& props = cont->properties();

	if (props.showDiagram) {
		y += h - 60;
		w = 540;
		h = 60;
		ImGui::SetNextWindowPos(ImVec2(x, y));
		ImGui::SetNextWindowSize(ImVec2(w, h));
		ImGui::Begin("GlobalTimerDialog Handle", nullptr,
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoTitleBar);

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		//const float spacing = ImGui::GetStyle().ItemSpacing.x;
		const float padding = ImGui::GetStyle().FramePadding.x;
		static float sz = 36.0f;
		//const ImU32 col32 = ImColor(1.0f, 1.0f, 0.4f, 1.0f);
		x += padding;
		y += 1;
		w -= 2 * padding;
		h -= 2;

		// Background
		draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), ImColor(1.0f, 1.0f, 1.0f, 1.0f));
		x += 2; w -= 4;
		y += 2; h -= 4;

		double frameNormalization = 1.0;// / static_cast<double>(cont->frameStats().sampleCount);
		float frameDuration = static_cast<float>(cont->frameStats().cumulatedTime * frameNormalization);
		float frameGpuDuration = static_cast<float>(cont->frameStats().cumulatedGpuTime * frameNormalization);
		float minDuration = 1000.f / 60.f * 1.3f;
		float scale = 1.0f / std::max(std::max(frameDuration, frameGpuDuration), minDuration) * w;

		// 60 FPS limit
		float limit = (1000.f / 60.f) * scale;
		draw_list->AddRectFilled(ImVec2(x + limit, y), ImVec2(x + limit + 1, y + w), ImColor(1.0f, 0.0f, 0.0f, 1.0f));

		// CPU timeline
		draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + frameDuration * scale, y + 20), ImColor(0.7f, 0.7f, 0.7f, 1.0f));
		// GPU timeline
		draw_list->AddRectFilled(ImVec2(x, y + 25), ImVec2(x + frameGpuDuration * scale, y + 25 + 20), ImColor(0.7f, 0.7f, 0.7f, 1.0f));
		for (const auto& s : cont->stats()) {
			if (!s.second.ui.visible) continue;
			double normalization = 1.0;// / static_cast<double>(s.second.sampleCount);
			float avg = static_cast<float>(s.second.cumulatedTime * normalization);
			float start = static_cast<float>(s.second.cumulatedFrameOffset * normalization);
			float offset = start * scale;
			float duration = avg * scale;
			glm::vec3 c = s.second.ui.color;
			draw_list->AddRectFilled(ImVec2(x + offset, y), ImVec2(x + offset + duration, y + 20), ImColor(c.r, c.g, c.b, 1.0f));

			float avgGpu = static_cast<float>(s.second.cumulatedGpuTime * normalization);
			float startGpu = static_cast<float>(s.second.cumulatedGpuFrameOffset * normalization);
			float offsetGpu = startGpu * scale;
			float durationGpu = avgGpu * scale;
			draw_list->AddRectFilled(ImVec2(x + offsetGpu, y + 25), ImVec2(x + offsetGpu + durationGpu, y + 25 + 20), ImColor(c.r, c.g, c.b, 1.0f));
		}

		//draw_list->AddRect(ImVec2(x, y), ImVec2(x + sz, y + sz), col32, 0.0f, ImDrawCornerFlags_All, 1); x += sz + spacing;

		ImGui::End();
	}
}
