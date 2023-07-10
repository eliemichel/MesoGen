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


#include <regex>
#include <algorithm>
#include <string>

#include "utils/fileutils.h"
#include "Logger.h"

using namespace std;

string baseDir(const string & path) {
	std::string fixedPath = fixPath(path);
	size_t pos = fixedPath.find_last_of(PATH_DELIM);
	return pos != string::npos ? fixedPath.substr(0, pos) : "";
}

string shortFileName(const string& path) {
	size_t pos = path.find_last_of(PATH_DELIM);
	return pos != string::npos ? path.substr(pos + 1) : path;
}

string fixPath(const string & path) {
	string p = path;
	p = replaceAll(p, "/", PATH_DELIM);
	p = replaceAll(p, "\\", PATH_DELIM);
	return p;
}

bool isAbsolutePath(const string & path) {
#ifdef _WIN32
	return path.length() >= 2 && path[1] == ':';
#else
	return path.length() >= 1 && path[0] == '/';
#endif
}

string canonicalPath(const string & path) {
	std::string out = path;
	regex r(string() + PATH_DELIM_ESCAPED + "[^" + PATH_DELIM_ESCAPED + "]+" + PATH_DELIM_ESCAPED + "\\.\\.");
	while (regex_search(out, r)) {
		out = regex_replace(out, r, "", regex_constants::format_first_only);
	}
	return out;
}

string resolvePath(const string & path, const string & basePath) {
	if (isAbsolutePath(path)) {
		return path;
	} else {
		return canonicalPath(joinPath(basePath, path));
	}
}
