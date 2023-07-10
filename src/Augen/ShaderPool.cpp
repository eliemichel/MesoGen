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


#include "Logger.h"
#include "ShaderPool.h"

#include <sstream>

#undef GetObject

ShaderPool ShaderPool::s_instance = ShaderPool();

///////////////////////////////////////////////////////////////////////////////
// Public static methods
///////////////////////////////////////////////////////////////////////////////

std::shared_ptr<ShaderProgram> ShaderPool::GetShader(const std::string & shaderName, int variantIndex, bool load)
{
	return s_instance.getShader(shaderName, variantIndex, load);
}

void ShaderPool::ReloadShaders()
{
	s_instance.reloadShaders();
}

///////////////////////////////////////////////////////////////////////////////
// Private singleton methods
///////////////////////////////////////////////////////////////////////////////

ShaderPool::ShaderPool() {}

std::shared_ptr<ShaderProgram> ShaderPool::getShader(const std::string & shaderName, int variantIndex, bool load)
{
	if (shaderName.empty()) return {};
	auto key = std::make_pair(shaderName, variantIndex);
	if (m_shaders.count(key) == 0) {
		m_shaders[key] = std::make_shared<ShaderProgram>(shaderName);
		if (load) {
			m_shaders[key]->load();
		}
	}
	return m_shaders.at(key);
}

void ShaderPool::reloadShaders()
{
	LOG << "Reloading shaders...";
	for (auto const& e : m_shaders)
	{
		e.second->load();
	}
}
