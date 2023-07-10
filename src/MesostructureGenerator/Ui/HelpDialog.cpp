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
#include "HelpDialog.h"

#include <imgui.h>

void HelpDialog::draw()
{
	ImGuiIO& io = ImGui::GetIO();
	ImGui::InputFloat("Font Size", &io.FontGlobalScale);
	/*
	if (ImGui::Button("Style Editor")) {
		ImGui::OpenPopup("style editor");
	}
	if (ImGui::BeginPopup("style editor"))
	{
		ImGui::ShowStyleEditor();
		ImGui::EndPopup();
	}
	*/

	ImGui::TextWrapped("Welcome to MesostructureGenerator!");
	ImGui::Spacing();

	ImGui::TextWrapped(
		"To navigate the viewport:\n"
		" - Use Mouse Middle Button to pan\n"
		" - Use Mouse Right Button to zoom\n"
		"\n"
		"Recommended workflow:\n"
		" 1. Go to the 'Model' tab\n"
		" 2. Add a few edges types with the first 'Add' buttons\n"
		" 3. Add a few tile types with the second 'Add' buttons\n"
		" 4. Assign edges to tiles by unfolding each tile's details (in the 'Tile Types' panel)\n"
		" 6. Go to the 'Generator' tab\n"
		" 7. Tune the dimensions of the generated area\n"
		" 8. Click 'Generate'\n"
		"Hopefully it'll generate something, otherwise try again, but the tile set might be deemed too complicated by the solver.\n"
		"\n"
		"When editing a tile type:\n"
		" - Mouse Left Button adds a shape on the nearest edge\n"
		" - Mouse Wheel changes the size of the next shape\n"
		" - Mouse Left Button over a path joining two shapes toggles its active state\n"
		"\n"
		"Misc advice:\n"
		" - On numeric input fields, hold Ctrl to directly enter a value, or Alt to slide slowly.\n"
	);
}
