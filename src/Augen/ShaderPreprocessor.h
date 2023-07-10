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

#include <string>
#include <vector>
#include <map>

class ShaderPreprocessor {
public:
	/**
	 * Load a shader from a file, and reccursively include #include'd files
	 * You can specity a list of defines to #define where ever
	 * `#include "sys:defines"` is found in the shader.
	 */
	bool load(const std::string & filename, const std::vector<std::string> & defines = {}, const std::map<std::string, std::string> & snippets = {});

	/**
	 * Return a buffer to the pure GLSL source, post-preprocessing, to be fed into
	 * glShaderSource.
	 */
	void source(std::vector<GLchar> & buf) const;

	/**
	 * Log a traceback corresponding to the line `line` in the processed source.
	 */
	void logTraceback(size_t line) const;

private:
	std::vector<std::string> m_lines;

private:
	static bool loadShaderSourceAux(const std::string & filename, const std::vector<std::string> & defines, const std::map<std::string, std::string> & snippets, std::vector<std::string> & lines_accumulator);
};

