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
#include "MesostructureRendererDialog.h"
#include "MesostructureRenderer.h"
#include "MesostructureData.h"

#include <utils/reflutils.h>

#include <imgui.h>

void MesostructureRendererDialog::draw()
{
	if (auto cont = m_cont.lock()) {
		auto& props = cont->properties();
		if (ImGui::CollapsingHeader("Mesostructure Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {
			autoUi(props);

			if (props.manualUpdate) {
				if (ImGui::Button("Update")) {
					cont->onModelChange();
				}
			}

			if (props.measureStats) {
				ImGui::Text("");
				ImGui::Text("Stats:");
				ImGui::Text(" - Point Count: %d", cont->stats().pointCount);
				ImGui::Text(" - Triangle Count: %d", cont->stats().triangleCount);
				ImGui::Text(" - GPU Byte Count: %d + %d", cont->stats().gpuByteCount, cont->stats().extraGpuByteCount);
				ImGui::TextWrapped("NB: Make sure to check 'Always reload' to have valid counters.");
			}
		}
	}
}
