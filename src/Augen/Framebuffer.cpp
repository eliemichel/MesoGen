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


#include "Framebuffer.h"
#include "Logger.h"
#include "Texture.h"

#include <vector>
#include <cassert>

Framebuffer::Framebuffer()
{
	glCreateFramebuffers(1, &m_raw);
}

Framebuffer::~Framebuffer()
{
	glDeleteFramebuffers(1, &m_raw);
}

void Framebuffer::bind() const
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_raw);
}

bool Framebuffer::check() const
{
	GLenum status = glCheckNamedFramebufferStatus(m_raw, GL_FRAMEBUFFER);
#ifndef NDEBUG
	switch (status) {
	case GL_FRAMEBUFFER_COMPLETE:
		//DEBUG_LOG << "Framebuffer status: GL_FRAMEBUFFER_COMPLETE";
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		DEBUG_LOG << "Framebuffer status: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		DEBUG_LOG << "Framebuffer status: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
		DEBUG_LOG << "Framebuffer status: GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
		DEBUG_LOG << "Framebuffer status: GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
		break;
	case GL_FRAMEBUFFER_UNSUPPORTED:
		DEBUG_LOG << "Framebuffer status: GL_FRAMEBUFFER_UNSUPPORTED";
		break;
	default:
		DEBUG_LOG << "Framebuffer status: UNKNOWN (" << status << ")";
		break;
	}
#endif // NDEBUG
	return status == GL_FRAMEBUFFER_COMPLETE;
}

void Framebuffer::attachTexture(int attachment, const Texture& texture, GLint level)
{
	glNamedFramebufferTexture(m_raw, GL_COLOR_ATTACHMENT0 + attachment, texture.raw(), level);
}

void Framebuffer::attachTexture(GLenum attachment, const Texture& texture, GLint level)
{
	glNamedFramebufferTexture(m_raw, attachment, texture.raw(), level);
}

void Framebuffer::attachDepthTexture(const Texture& texture, GLint level)
{
	glNamedFramebufferTexture(m_raw, GL_DEPTH_ATTACHMENT, texture.raw(), level);
}

void Framebuffer::enableDrawBuffers(int n)
{
	assert(n > 0);
	std::vector<GLenum> drawBuffers(n);
	for (size_t k = 0; k < n; ++k) {
		drawBuffers[k] = GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(k);
	}
	glNamedFramebufferDrawBuffers(m_raw, static_cast<GLsizei>(drawBuffers.size()), &drawBuffers[0]);
}

void Framebuffer::disableDrawBuffers()
{
	glNamedFramebufferDrawBuffer(m_raw, GL_NONE);
}
