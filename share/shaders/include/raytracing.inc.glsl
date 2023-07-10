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
//////////////////////////////////////////////////////
// Ray-tracing routines

// requires that you include utils.inc.glsl for isOrthographic() and uniforms/camera.inc.glsl for resolution

struct Ray {
    vec3 origin;
    vec3 direction;
};

/**
 * Apply an affine transformation matrix to a ray
 */
Ray transformRay(Ray ray, mat4 transform) {
    Ray transformed_ray;
    transformed_ray.origin = (transform * vec4(ray.origin, 1)).xyz;
    transformed_ray.direction = mat3(transform) * ray.direction;
    return transformed_ray;
}

/**
 * Get the ray corresponding to a pixel, in camera space
 */
Ray fragmentRay(in vec2 fragCoord, in mat4 projectionMatrix, in vec2 resolution) {
    vec2 uv = fragCoord.xy / resolution.xy * 2.f - vec2(1.f);
    
    if (isOrthographic(projectionMatrix)) {
        float a = 1.0 / projectionMatrix[0][0];
        float b = a * projectionMatrix[3][0];
        float c = 1.0 / projectionMatrix[1][1];
        float d = c * projectionMatrix[3][1];
        return Ray(
            vec3(a * uv.x + b, c * uv.y + d, 0.0),
            vec3(0.0, 0.0, -1.f)
        );
    } else {
        float fx = projectionMatrix[0][0];
        float fy = projectionMatrix[1][1];
        return Ray(
            vec3(0.0),
            vec3(uv.x / fx, uv.y / fy, -1.f)
        );
    }
}


bool intersectRaySphere(out vec3 hitPosition, in Ray ray, in vec3 center, in float radius) {
    vec3 o = center - ray.origin;
    float d2 = dot(ray.direction, ray.direction);
    float r2 = radius * radius;
    float l2 = dot(o, o);
    float dp = dot(ray.direction, o);
    float delta = dp * dp / d2 - l2 + r2;

    if (delta >= 0) {
        float lambda = dp / d2 - sqrt(delta / d2);
        if (lambda < 0) lambda += 2. * sqrt(delta / d2);
        if (lambda >= 0) {
            hitPosition = ray.origin + lambda * ray.direction;
            return true;
        }
    }

    return false;
}


vec3 intersectRayPlane(in Ray ray, in vec3 pointOnPlane, in vec3 planeNormal) {
    vec3 o = pointOnPlane - ray.origin;
    float lambda = dot(o, planeNormal) / dot(ray.direction, planeNormal);
    return ray.origin + lambda * ray.direction;
}

float intersectRayRay(in Ray ray0, in Ray ray1, out float lambda0, out float lambda1) {
    vec3 n = cross(ray0.direction, ray1.direction);
    float ln2 = dot(n, n);
    float ln = sqrt(ln2);
    lambda0 = dot(cross(ray1.origin - ray0.origin, ray1.direction), n) / ln2;
    lambda1 = dot(cross(ray1.origin - ray0.origin, ray0.direction), n) / ln2;
    float distance = abs(dot(ray0.origin - ray1.origin, n) / ln);
    return distance;
}

