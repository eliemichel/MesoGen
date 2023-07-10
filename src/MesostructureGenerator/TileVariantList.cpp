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
#include "TileVariantList.h"

#include <magic_enum.hpp>

#include <sstream>

TileVariantList::TileVariantList(const Model::Tileset& model) {
	using TileTransform = Model::TileTransform;
	using TileOrientation = Model::TileOrientation;

	int tileIndex = -1;
	for (const auto& tile : model.tileTypes) {
		++tileIndex;

		if (tile->ignore) continue;

		std::vector<TileTransform> transforms;
		transforms.push_back(TileTransform());
		if (tile->transformPermission.flipX) {
			std::vector<TileTransform> prevTransforms(transforms);
			transforms.resize(0);
			for (TileTransform tr : prevTransforms) {
				transforms.push_back(tr);
				tr.flipX = !tr.flipX;
				transforms.push_back(tr);
			}
		}
		if (tile->transformPermission.flipY) {
			std::vector<TileTransform> prevTransforms(transforms);
			transforms.resize(0);
			for (TileTransform tr : prevTransforms) {
				transforms.push_back(tr);
				tr.flipY = !tr.flipY;
				transforms.push_back(tr);
			}
		}
		if (tile->transformPermission.rotation) {
			std::vector<TileTransform> prevTransforms(transforms);
			transforms.resize(0);
			for (TileTransform tr : prevTransforms) {
				TileOrientation baseOrientation = tr.orientation;
				for (int i = 0; i < magic_enum::enum_count<TileOrientation>(); ++i) {
					tr.orientation = static_cast<TileOrientation>(static_cast<int>(baseOrientation) + i);
					transforms.push_back(tr);
				}
			}
		}

		for (const auto& tr : transforms) {
			m_variants.push_back(Model::TransformedTile{ tileIndex, tr });
		}
	}
}

int TileVariantList::variantCount() const {
	return static_cast<int>(m_variants.size());
}

Model::TransformedTile TileVariantList::variantToTile(int variantIndex) const {
	return m_variants[variantIndex];
}

std::string TileVariantList::variantRepr(int variantIndex) const {
	std::ostringstream ss;
	auto transforedTile = variantToTile(variantIndex);

	ss << variantIndex << " (tile #" << transforedTile.tileIndex << ", transform";

	if (transforedTile.transform.flipX) {
		ss << " fX";
	}
	if (transforedTile.transform.flipY) {
		ss << " fY";
	}

	switch (transforedTile.transform.orientation) {
	case Model::TileOrientation::Deg0:
		break;
	case Model::TileOrientation::Deg90:
		ss << " 90d";
		break;
	case Model::TileOrientation::Deg180:
		ss << " 180d";
		break;
	case Model::TileOrientation::Deg270:
		ss << " 270d";
		break;
	}

	ss << ")";
	return ss.str();
}
