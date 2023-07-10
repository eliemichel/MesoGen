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
#include <limits>
#include <vector>

struct Ssbo {
	GLuint raw = std::numeric_limits<GLuint>::max();
	GLsizeiptr size = 0;

	Ssbo() {}
	~Ssbo();
	Ssbo(const Ssbo&) = delete;
	Ssbo& operator=(const Ssbo&) = delete;

	template<typename T>
	void update(const std::vector<T>& data, GLbitfield flags = GL_DYNAMIC_STORAGE_BIT) {
		Update(*this, data, flags);
	}

	void resize(GLsizeiptr newSsboSize, GLbitfield flags = GL_DYNAMIC_STORAGE_BIT);

public:
	template<typename T>
	static void Update(Ssbo& ssbo, const std::vector<T>& data, GLbitfield flags = GL_DYNAMIC_STORAGE_BIT) {
		GLsizeiptr newSsboSize = sizeof(T) * data.size();
		if (ssbo.size != newSsboSize) {
			if (ssbo.raw != std::numeric_limits<GLuint>::max()) {
				glDeleteBuffers(1, &ssbo.raw);
			}

			if (newSsboSize > 0) {
				glCreateBuffers(1, &ssbo.raw);
				glNamedBufferStorage(ssbo.raw, newSsboSize, data.data(), flags);
			}
			else {
				ssbo.raw = std::numeric_limits<GLuint>::max();
			}
			ssbo.size = newSsboSize;
		}
		else if (ssbo.size > 0) {
			glNamedBufferSubData(ssbo.raw, 0, newSsboSize, data.data());
		}
	}
};
