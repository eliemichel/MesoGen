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

#include <OpenGL>

#include "VersionCounter.h"

#include <NonCopyable.h>

#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <array>

struct DrawCall;
namespace Model {
struct Tileset;
}
namespace bmesh {
struct BMesh;
}
class TileVariantList;
class MacrosurfaceData;

/**
 * Representation of the final mesostructure, a mesh with transform tiles
 * assigned to each quad face.
 */
class MesostructureData : public NonCopyable {
public:
	MesostructureData();

	/**
	 * A mesostructure is valid iff its model and its macrostructure are not
	 * null, and optionaly slot assignments are compatible.
	 */
	bool isValid(bool checkSlotAssignments = false) const;

	int tileCount() const;
	glm::mat4 modelMatrix() const;

public:
	/**
	 * The tileset used to build the mesostructure
	 */
	std::shared_ptr<Model::Tileset> model;

	/**
	 * Macrosurface (mesh or grid) onto which the mesostrcture has been generated
	 */
	std::weak_ptr<MacrosurfaceData> macrosurface;

	/**
	 * Stores the correspondence from tile variant indices to tile and transform
	 */
	std::shared_ptr<TileVariantList> tileVariantList;

	/**
	 * For each slot (face of the mesh), assign a tile variant index
	 */
	std::vector<int> slotAssignments;

	// ----------------------------------------------------------------------------
	// Counters used to sync with render data

	/**
	 * Incremented each time the list of tiles in the tileset changes
	 */
	VersionCounter tileListVersion;

	/**
	 * When only the content of a tile is changed, prefer incrementing the
	 * corresponding version here than tileListVersion, this will updated only the
	 * data related to this very tile and not affect the other ones.
	 */
	std::vector<VersionCounter> tileVersions;

	/**
	 * Incremented each time slotAssignments changes
	 */
	VersionCounter slotAssignmentsVersion;

};
