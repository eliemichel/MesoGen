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

#include "Camera.h"

#include <glm/gtc/quaternion.hpp>

class TurntableCamera : public Camera {
public:
	TurntableCamera();

	void setCenter(glm::vec3 center) override;
	void setOrientation(const glm::quat& orientation);
	void setZoom(float zoom);

	const glm::vec3& center() const { return m_center; }
	const glm::quat& orientation() const { return m_quat; }
	float getZoom() const { return m_zoom; }

protected:
	void updateDeltaMouseRotation(float x1, float y1, float x2, float y2) override;
	void updateDeltaMouseZoom(float x1, float y1, float x2, float y2) override;
	void updateDeltaMousePanning(float x1, float y1, float x2, float y2) override;
	void tilt(float theta);

private:
	/**
	* Construct view matrix given quat, center and zoom
	*/
	void updateViewMatrix();

private:
	glm::vec3 m_center;
	glm::quat m_quat;
	float m_zoom;
	float m_sensitivity;  // relative to screen size
	float m_zoomSensitivity;
};

