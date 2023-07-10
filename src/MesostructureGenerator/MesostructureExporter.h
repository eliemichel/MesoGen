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

#include <NonCopyable.h>
#include <ReflectionAttributes.h>
#include <refl.hpp>
#include <glm/glm.hpp>

#include <vector>
#include <string>

class MesostructureData;
class MesostructureRenderer;
class ShaderProgram;

class MesostructureExporter : public NonCopyable {
public:
	MesostructureExporter(
		std::weak_ptr<MesostructureData> mesostructureData,
		std::weak_ptr<MesostructureRenderer> mesostructureRenderer
	);

	bool exportMesh() const;
	bool exportMeshMvc() const;

private:
	struct OutputDataBatch {
		std::vector<glm::vec4> pointData;
		std::vector<glm::vec4> normalData;
		std::vector<glm::vec4> uvData;
		std::vector<glm::uvec4> elementData;
		std::vector<int> slotIndexData;
	};
	static bool WriteObj(const std::string& filename, const std::vector<OutputDataBatch>& outputData);
	bool writePly(const std::string& filename, const std::vector<OutputDataBatch>& outputData) const;

public:
	struct Properties {
		std::string filename = "output.ply";
		bool exportSlotIndex = false;
	};
	const Properties& properties() const { return m_properties; }
	Properties& properties() { return m_properties; }

private:
	Properties m_properties;

	std::weak_ptr<MesostructureData> m_mesostructureData;
	std::weak_ptr<MesostructureRenderer> m_mesostructureRenderer;
	std::shared_ptr<ShaderProgram> m_exportShader;
	std::shared_ptr<ShaderProgram> m_exportMvcShader;
};

#define _ ReflectionAttributes::
REFL_TYPE(MesostructureExporter::Properties)
REFL_FIELD(filename, _ MaximumSize(256))
REFL_FIELD(exportSlotIndex)
REFL_END
#undef _

