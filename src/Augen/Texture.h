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

class Texture {
public:
	/**
	 * Create a texture for a given target (same target as in glCreateTextures)
	 */
	explicit Texture(GLenum target);
	
	/**
	 * Take ownership of a raw gl texture.
	 * This object will take care of deleting texture, unless you release() it.
	 */
	explicit Texture(GLuint id, GLenum target);

	~Texture();
	Texture(const Texture&) = delete;
	Texture& operator=(const Texture&) = delete;
	Texture(Texture&&) = default;
	Texture& operator=(Texture&&) = default;

	void storage(GLsizei levels, GLenum internalFormat, GLsizei width);
	void storage(GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height);
	void storage(GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth);
	
	void subImage(GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void* pixels);
	void subImage(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels);
	void subImage(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels);

	void generateMipmap() const;
	void setWrapMode(GLenum wrap) const;

	GLuint raw() const { return m_id; }
	GLsizei width() const { return m_width; }
	GLsizei height() const { return m_height; }
	GLsizei depth() const { return m_depth; }
	GLsizei levels() const { return m_levels; }
	GLenum target() const { return m_target; }

	bool isValid() const { return m_id != invalid; }
	void bind(GLint unit) const;
	void bind(GLuint unit) const;

	// forget texture without deleting it, use with caution
	void release() { m_id = invalid; }
	
private:
	static const GLuint invalid;

private:
	GLuint m_id;
	GLenum m_target;
	GLsizei m_width = 0;
	GLsizei m_height = 0;
	GLsizei m_depth = 0;
	GLsizei m_levels = 1;
};
