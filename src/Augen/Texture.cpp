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


#include "Texture.h"

#include <climits>

const GLuint Texture::invalid = UINT_MAX;

Texture::Texture(GLenum target)
	: m_id(invalid)
	, m_target(target)
{
	glCreateTextures(m_target, 1, &m_id);

	glTextureParameteri(m_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTextureParameteri(m_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	setWrapMode(GL_CLAMP_TO_EDGE);
}

Texture::Texture(GLuint id, GLenum target)
	: m_id(id)
	, m_target(target)
{}

void Texture::storage(GLsizei levels, GLenum internalFormat, GLsizei width)
{
	m_width = width;
	m_height = 1;
	m_depth = 1;
	m_levels = levels;
	glTextureStorage1D(m_id, levels, internalFormat, width);
}

void Texture::storage(GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height)
{
	m_width = width;
	m_height = height;
	m_depth = 1;
	m_levels = levels;
	glTextureStorage2D(m_id, levels, internalFormat, width, height);
}

void Texture::storage(GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth)
{
	m_width = width;
	m_height = height;
	m_depth = depth;
	m_levels = levels;
	glTextureStorage3D(m_id, levels, internalFormat, width, height, depth);
}

void Texture::subImage(GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void* pixels)
{
	glTextureSubImage1D(m_id, level, xoffset, width, format, type, pixels);
}

void Texture::subImage(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void * pixels)
{
	glTextureSubImage2D(m_id, level, xoffset, yoffset, width, height, format, type, pixels);
}

void Texture::subImage(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void * pixels)
{
	glTextureSubImage3D(m_id, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}

void Texture::generateMipmap() const
{
	glGenerateTextureMipmap(m_id);
}

void Texture::setWrapMode(GLenum wrap) const
{
	glTextureParameteri(m_id, GL_TEXTURE_WRAP_S, wrap);
	glTextureParameteri(m_id, GL_TEXTURE_WRAP_T, wrap);
	glTextureParameteri(m_id, GL_TEXTURE_WRAP_R, wrap);
}

Texture::~Texture()
{
	if (isValid()) {
		glDeleteTextures(1, &m_id);
	}
}

void Texture::bind(GLint unit) const
{
	bind(static_cast<GLuint>(unit));
}

void Texture::bind(GLuint unit) const
{
	glBindTextureUnit(unit, m_id);
}
