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
#include "TilingSolver.h"
#include "MacrosurfaceData.h"
#include "MesostructureData.h"
#include "TilesetController.h"

// Model Synthesis
#include "Synthesizer.h"

#include <Logger.h>

bool TilingSolver::solveGridMs(MesostructureData& mesostructureData) {
	LOG << "Generating output...";
	std::chrono::microseconds synthesisTime{ 0 };

	const MacrosurfaceData& macrosurfaceData = *mesostructureData.macrosurface.lock();

	auto adapter = TilesetController::ConvertModelToSignedWangRuleset(*mesostructureData.model);
	mesostructureData.tileVariantList = std::move(adapter.tileVariantList);

	int tileTypeCount = mesostructureData.tileVariantList->variantCount();

	// Model Synthesis settings
	InputSettings opts;

	const auto& dimensions = macrosurfaceData.properties().dimensions;
	opts.name = "TileDag2D";
	opts.size[0] = dimensions[0];
	opts.size[1] = dimensions[1];
	opts.size[2] = 1;
	opts.blockSize[0] = dimensions[0];
	opts.blockSize[1] = dimensions[1];
	opts.blockSize[2] = 1;

	opts.weights = std::vector<float>(tileTypeCount, 1.0f);
	opts.numLabels = tileTypeCount;

	int initialLabel = 0;
	opts.initialLabels = &initialLabel;

	// transition[direction][labelA][labelB]
	// direction: x = 0, y = 1, z = 2
	auto transitionTable = new bool** [3]; // these are freed in InputSettings dtor
	for (int direction = 0; direction < 3; ++direction) {
		auto column = new bool* [tileTypeCount];

		int tileEdgeIndexA = 0;
		int tileEdgeIndexB = 0;
		if (direction == 0) {
			tileEdgeIndexA = Model::TileEdge::PosX;
			tileEdgeIndexB = Model::TileEdge::NegX;
		}
		else if (direction == 1) {
			tileEdgeIndexA = Model::TileEdge::PosY;
			tileEdgeIndexB = Model::TileEdge::NegY;
		}

		for (int labelA = 0; labelA < tileTypeCount; ++labelA) {
			auto row = new bool[tileTypeCount];

			int edgeLabelA = adapter.table.get_at(labelA, tileEdgeIndexA);

			for (int labelB = 0; labelB < tileTypeCount; ++labelB) {
				int edgeLabelB = adapter.table.get_at(labelB, tileEdgeIndexB);

				row[labelB] = direction < 2 && edgeLabelB == -edgeLabelA;
			}
			column[labelA] = row;
		}
		transitionTable[direction] = column;
	}
	opts.transition = transitionTable;

	opts.supporting.resize(tileTypeCount);
	opts.supportCount.resize(tileTypeCount);
	for (int labelA = 0; labelA < tileTypeCount; ++labelA) {
		opts.supporting[labelA].resize(4);
		opts.supportCount[labelA].resize(4);
	}

	for (int labelA = 0; labelA < tileTypeCount; ++labelA) {
		for (int direction = 0; direction < 2; ++direction) {
			for (int labelB = 0; labelB < tileTypeCount; ++labelB) {
				if (transitionTable[direction][labelA][labelB]) {
					opts.supporting[labelB][2 * direction + 0].push_back(labelA);
					opts.supporting[labelA][2 * direction + 1].push_back(labelB);
				}
			}
		}
	}

	for (int labelA = 0; labelA < tileTypeCount; ++labelA) {
		for (int signedDirection = 0; signedDirection < 4; ++signedDirection) {
			opts.supportCount[labelA][signedDirection] = static_cast<int>(opts.supporting[labelA][signedDirection].size());
		}
	}

	opts.periodic = false;
	opts.numDims = 2;

	opts.type = "simpletiled";

	opts.numAttempts = properties().maxAttempts;

	Synthesizer synthesizer(&opts, synthesisTime);
	synthesizer.synthesize(synthesisTime);

	LOG << "Generation took " << (synthesisTime.count() / 1000.0) << " ms";

	if (!synthesizer.isSuccessful()) {
		mesostructureData.slotAssignments.resize(0);
		m_message = "Generation failed";
		return false;
	}

	int*** output = synthesizer.getModel();

#if 0
	std::ostringstream ss;
	for (int i = 0; i < opts.size[0]; ++i) {
		if (i > 0) ss << "\n";
		for (int j = 0; j < opts.size[1]; ++j) {
			if (j > 0) ss << ", ";
			ss << output[i][j][0];
		}
	}
	LOG << "Output: [\n" << ss.str() << "\n]";
#endif

	// save output in this class's attribute before synthesizer frees it
	mesostructureData.slotAssignments.resize(dimensions[0] * dimensions[1]);
	for (int i = 0; i < dimensions[0]; ++i) {
		for (int j = 0; j < dimensions[1]; ++j) {
			mesostructureData.slotAssignments[i + j * dimensions[0]] = output[i][j][0];
		}
	}

	m_message = "Generation succeeded";
	return true;
}
