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


#include "utils/fileutils.h"
#include "utils/strutils.h"
#include "Logger.h"
#include "ShaderPreprocessor.h"
#include "Shader.h"

#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <sstream>
#include <vector>
#include <utility>
#include <regex>


Shader::Shader(GLenum shaderType)
    : m_shaderId(0)
{
	m_shaderId = glCreateShader(shaderType);
}


Shader::~Shader() {
    glDeleteShader(m_shaderId);
}

bool Shader::load(const std::string &filename, const std::vector<std::string> & defines, const std::map<std::string, std::string> & snippets) {
	ShaderPreprocessor preprocessor;

	if (!preprocessor.load(filename, defines, snippets)) {
		return false;
	}

	std::vector<GLchar> buf;
	preprocessor.source(buf);
	const GLchar *source = &buf[0];
    glShaderSource(m_shaderId, 1, &source, 0);

#ifndef NDEBUG
	m_preprocessor = preprocessor;
#endif

	return true;
}


bool Shader::check(const std::string & name) const {
    int ok;
    
    glGetShaderiv(m_shaderId, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        int len;
        glGetShaderiv(m_shaderId, GL_INFO_LOG_LENGTH, &len);
        char *log = new char[len];
        glGetShaderInfoLog(m_shaderId, len, &len, log);

		ERR_LOG << "Unable to compile " << name << ". OpenGL returned:";
		if (len == 0) {
			ERR_LOG << "(Driver reported no error message)";
		}
		else {
#ifndef NDEBUG
			// Example of error message:
			// 0(158) : error C0000: syntax error, unexpected '/' at token "/"
			// another example: (intel graphics)
			// ERROR: 0:506: '=' : syntax error syntax error
			// or
			// 0:42(50): error: syntax error, unexpected '=', expecting ')'
			std::regex errorPatternA(R"(^(\d+)\((\d+)\) (.*)$)");
			std::regex errorPatternB(R"(^ERROR: (\d+):(\d+): (.*)$)");
			std::regex errorPatternC(R"(^\d+:(\d+)\((\d+)\): error: (.*))");

			int line = 0;
			std::string s(log);
			std::string msg = s;
			std::smatch match;
			if (regex_search(s, match, errorPatternA)) {
				line = stoi(match[2].str());
				msg = match[3].str();
			}
			else if (regex_search(s, match, errorPatternB)) {
				line = stoi(match[2].str());
				msg = match[3].str();
			}
			else if (regex_search(s, match, errorPatternC)) {
				line = stoi(match[1].str());
				msg = match[3].str();
			}
			ERR_LOG << msg;
			m_preprocessor.logTraceback(static_cast<size_t>(line));
#else // NDEBUG
			ERR_LOG << log;
#endif // NDEBUG
		}

		delete[] log;
    }

    return ok;
}
