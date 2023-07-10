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
#version 450 core
#include "sys:defines"

layout (location = 0) out vec4 color;

in GeometryData {
    vec2 uv;
} geo;

const float RAYMARCHING_EPSILON = 1e-3;
const float LIPSCHITZ_FAC = 0.25;
const int RAYMARCHING_MAX_STEPS = 100;

#include "include/utils.inc.glsl"
#include "include/raytracing.inc.glsl"
#include "include/uniform/lighting.inc.glsl"
#include "include/uniform/camera.inc.glsl"
uniform vec4 uColor = vec4(0.0, 0.0, 0.0, 0.5);
uniform sampler2D uTexture;
uniform bool uHasTexture = false;
uniform float uColorMultiplier = 1.0;
uniform float uColorGamma = 1.0;

readonly layout(std430) buffer CagePoints
{
    vec4 bCagePoints[];
};
#include "include/mvc.inc.glsl"

mat2 rot(float th) {
    float c = cos(th), s = sin(th);
    return mat2(c, s, -s, c);
}

float map(vec3 p) {
    p.xy = rot(5.0 * p.z) * p.xy;
    p.xz = rot(0.5) * p.xz;
    p.xy *= 2.0;
    p.z *= 0.5;
    return length(p) - 0.5;
}

void main() {
    color = vec4(geo.uv, 0.5, 1.0);

    Ray cRay = fragmentRay(gl_FragCoord.xy, uCamera.projectionMatrix, uCamera.resolution.xy);
    Ray ray = transformRay(cRay, inverse(uCamera.viewMatrix));

    float d0 = length((uCamera.viewMatrix * vec4(bCagePoints[0].xyz, 1.0)).xyz);
    float d1 = length((uCamera.viewMatrix * vec4(bCagePoints[1].xyz, 1.0)).xyz);
    float d2 = length((uCamera.viewMatrix * vec4(bCagePoints[2].xyz, 1.0)).xyz);
    float d3 = length((uCamera.viewMatrix * vec4(bCagePoints[3].xyz, 1.0)).xyz);
    float d4 = length((uCamera.viewMatrix * vec4(bCagePoints[4].xyz, 1.0)).xyz);
    float d5 = length((uCamera.viewMatrix * vec4(bCagePoints[5].xyz, 1.0)).xyz);
    float d6 = length((uCamera.viewMatrix * vec4(bCagePoints[6].xyz, 1.0)).xyz);
    float d7 = length((uCamera.viewMatrix * vec4(bCagePoints[7].xyz, 1.0)).xyz);
    float minD = d0;
    minD = min(minD, d1);
    minD = min(minD, d2);
    minD = min(minD, d3);
    minD = min(minD, d4);
    minD = min(minD, d5);
    minD = min(minD, d6);
    minD = min(minD, d7);
    float maxD = d0;
    maxD = max(maxD, d1);
    maxD = max(maxD, d2);
    maxD = max(maxD, d3);
    maxD = max(maxD, d4);
    maxD = max(maxD, d5);
    maxD = max(maxD, d6);
    maxD = max(maxD, d7);

    ray.origin += ray.direction * minD;

    vec3 origin = ray.origin;
    int i = 0;
    vec3 p;
    float s;
    float cageWeights[8];
    for (i = 0 ; i < RAYMARCHING_MAX_STEPS ; ++i) {
        p = ray.origin;

        computeInverseMeanValueCoordinates(p, cageWeights);
        p = vec3(0.0);
        for (int i = 0 ; i < 8 ; ++i) {
            p += cageWeights[i] * cRestCagePoints[i].xyz;
        }

        s = map(p);
        if (s < RAYMARCHING_EPSILON) {
            color.rgb = i < 1 ? vec3(1.0, 0.0, 0.5) : vec3(float(i) / float(RAYMARCHING_MAX_STEPS));
            break;
        }
        if (length(origin - ray.origin) > maxD) {
            p = vec3(9999);
            break;
        }
        ray.origin += s * LIPSCHITZ_FAC * ray.direction;
    }

    if (i < RAYMARCHING_MAX_STEPS) {
        vec2 eps = vec2(1e-5, 0);

        vec3 N = normalize(vec3(
            s - map(p + eps.xyy),
            s - map(p + eps.yxy),
            s - map(p + eps.yyx)
        ));

        // Lighting
        color.rgb = vec3(0.0);
        for (int i = 0 ; i < uLightCount ; ++i) {
            //if (i != 0) continue;
            vec3 L = -uLightDirections[i];
            vec3 diff = uLightDiffuseColors[i];
            vec3 R = normalize(reflect(-L, N));
            vec3 V = -normalize(ray.direction);
            color.rgb += max(dot(N, L), 0) * diff * 0.1;
        }
    } else {
        discard;
    }

    color = pow(color * uColorMultiplier, vec4(uColorGamma));
}
