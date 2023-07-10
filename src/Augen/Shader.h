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

#ifndef NDEBUG
#include "ShaderPreprocessor.h"
#endif

#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <set>
#include <map>
#include <memory>

/**
 * Utility class providing an OO API to OpenGL shaders
 */
class Shader {
public:
    Shader(GLenum shaderType = 0);
    ~Shader();

    /**
     * Load file into the shader
     * @param filename Shader file
     */
    bool load(const std::string &filename, const std::vector<std::string> & defines = {}, const std::map<std::string, std::string> & snippets = {});

    /**
     * Compile the shader
     */
    inline void compile() { glCompileShader(m_shaderId); }

    /**
     * Check that the shader has been successfully compiled
     * @param name Name displayed in error message
     * @param success status
     */
    bool check(const std::string & name = "shader") const;

    inline GLuint shaderId() const { return m_shaderId; }

private:
    GLuint m_shaderId;

#ifndef NDEBUG
	// Keep shader source
	ShaderPreprocessor m_preprocessor;
#endif
};
