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

#include "Shader.h"
#include "ColorLayerInfo.h"
#include "Framebuffer.h"

class Texture;

/**
 * A RenderTarget is a Framebuffer plus the attached textures
 */
class RenderTarget {
public:
	RenderTarget(GLsizei width, GLsizei height, const std::vector<ColorLayerInfo> & colorLayerInfos = {});

	void bind() const;

	size_t colorTextureCount() const { return m_colorTextures.size(); }
	Texture& colorTexture(int i) { return *m_colorTextures[i]; }
	const Texture& colorTexture(int i) const { return *m_colorTextures[i]; }
	std::shared_ptr<Texture> colorTexturePointer(int i) { return m_colorTextures[i]; }

	Texture& depthTexture() { return *m_depthTexture; }
	const Texture& depthTexture() const { return *m_depthTexture; }
	std::shared_ptr<Texture> depthTexturePointer() { return m_depthTexture; }

	GLsizei width() const { return m_width; }
	GLsizei height() const { return m_height; }

	const Framebuffer& framebuffer() const { return m_framebuffer; }

	/**
	 * Use this with caution, it reallocates video memory
	 */
	void setResolution(GLsizei width, GLsizei height);

	void deactivateColorAttachments();
	void activateColorAttachments();

private:
	void init();

private:
	GLsizei m_width, m_height;
	std::vector<ColorLayerInfo> m_colorLayerInfos;

	Framebuffer m_framebuffer;

	std::vector<std::shared_ptr<Texture>> m_colorTextures;
	std::shared_ptr<Texture> m_depthTexture;
};

