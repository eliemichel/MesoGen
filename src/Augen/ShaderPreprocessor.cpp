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


#include "ShaderPreprocessor.h"
#include "Logger.h"
#include "utils/strutils.h"
#include "utils/fileutils.h"

#include <fstream>
#include <map>

using namespace std;

constexpr const char* BEGIN_INCLUDE_TOKEN = "// _AUGEN_BEGIN_INCLUDE";
constexpr const char* END_INCLUDE_TOKEN = "// _AUGEN_END_INCLUDE";

bool ShaderPreprocessor::load(const string & filename, const vector<string> & defines, const std::map<std::string, std::string> & snippets) {
	return loadShaderSourceAux(filename, defines, snippets, m_lines);
}

void ShaderPreprocessor::source(vector<GLchar> & buf) const {
	size_t length = 0;
	for (string s : m_lines) {
		length += s.size() + 1;
		buf.reserve(length);
		for (char& c : s) {
			buf.push_back(c);
		}
		buf.push_back('\n');
	}
	buf.push_back(0);
}

void ShaderPreprocessor::logTraceback(size_t line) const {
	LOG << "Traceback:";
	size_t i = 0, localOffset = 0;
	string filename = "";
	vector<pair<string, size_t>> stack;
	size_t ignoreLine = 0;
	for (auto l : m_lines) {
		if (startsWith(l, BEGIN_INCLUDE_TOKEN)) {
			// Stack context
			stack.push_back(make_pair(filename, localOffset));
			if (i > line) {
				++ignoreLine;
			}
			size_t n = string(BEGIN_INCLUDE_TOKEN).size();
			filename = l.substr(n, l.size() - n);
			localOffset = 0;
		}
		else if (startsWith(l, END_INCLUDE_TOKEN)) {
			if (!stack.empty()) {
				pair<string, size_t> p = stack.back();
				stack.pop_back();
				if (!stack.empty() && i > line) {
					if (ignoreLine) {
						ignoreLine--;
					}
					else {
						LOG << "Included in " << p.first << ", line " << p.second;
					}
				}
				// Unstack context
				filename = p.first;
				localOffset = p.second;
			}
		}
		else if (!stack.empty() && line == i) {
			LOG << "In " << filename << ", line " << (localOffset - 1);
		}
		++i;
		++localOffset;
	}
}


// Note: no include loop check is done, beware of infinite loops
bool ShaderPreprocessor::loadShaderSourceAux(const string & filename, const vector<string> & defines, const std::map<std::string, std::string> & snippets, vector<string> & lines_accumulator) {
	static const string includeKeywordLower = "#include";
	static const string defineKeywordLower = "#define";
	static const string systemPrefix = "sys:";
	static const string sysDefinesFilename = "defines";

	string shortName = shortFileName(filename);
	if (startsWith(shortName, systemPrefix)) {
		lines_accumulator.push_back(string() + BEGIN_INCLUDE_TOKEN + " " + shortName);

		const string key = shortName.substr(systemPrefix.length());
		if (key == sysDefinesFilename) {
			for (auto def : defines) {
				lines_accumulator.push_back(defineKeywordLower + " " + def);
			}
		} else {
			if (snippets.count(key) > 0) {
				for (const auto& line : split(snippets.at(key), '\n')) {
					lines_accumulator.push_back(line);
				}
			}
		}

		lines_accumulator.push_back(string() + END_INCLUDE_TOKEN + " " + shortName);
		return true;
	}

	lines_accumulator.push_back(string() + BEGIN_INCLUDE_TOKEN + " " + filename);

	ifstream in(filename);
	if (!in.is_open()) {
		WARN_LOG << "Unable to open file: " << filename;
		return false;
	}

	string line;

	size_t i = 0;
	while (getline(in, line)) {
		++i;
		// Poor man's #include directive parser
		if (startsWith(toLower(line), includeKeywordLower)) {
			string includeFilename = line.substr(includeKeywordLower.size());
			trim(includeFilename);
			if (includeFilename[0] != '"' || includeFilename[includeFilename.size() - 1] != '"') {
				ERR_LOG << "Syntax error in #include directive at line " << i << " in file " << filename;
				ERR_LOG << "  filename is expected to be enclosed in double quotes (\")";
				return false;
			}
			includeFilename = includeFilename.substr(1, includeFilename.size() - 2);
			string fullIncludeFilename = joinPath(baseDir(filename), includeFilename);
			{
				ifstream check(fullIncludeFilename);
				if (!check.is_open()) {
					fullIncludeFilename = joinPath(SHARE_DIR, "shaders", includeFilename);
				}
			}
			if (!loadShaderSourceAux(fullIncludeFilename, defines, snippets, lines_accumulator)) {
				ERR_LOG << "Include error at line " << i << " in file " << filename;
				return false;
			}
		}
		else {
			lines_accumulator.push_back(line);
		}
	}
	lines_accumulator.push_back(END_INCLUDE_TOKEN);
	return true;
}
