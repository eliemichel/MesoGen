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
#include "include/common-defines.inc.glsl"
#include "include/standard-outs.inc.glsl"

in GeometryData {
    vec3 position_ws;
    vec2 uv;
    vec3 normal_ws;
} geo;

#include "include/utils.inc.glsl"
#include "include/raytracing.inc.glsl"
#include "include/uniform/lighting.inc.glsl"
#include "include/uniform/camera.inc.glsl"
uniform vec4 uColor = vec4(0.0, 0.0, 0.0, 0.5);
uniform sampler2D uTexture;
uniform bool uHasTexture = false;
uniform float uColorMultiplier = 1.0;
uniform float uColorGamma = 1.0;

void main() {
    vec4 albedo = uColor;
    if (uHasTexture) {
        albedo = texture(uTexture, geo.uv);
    }
    albedo.rgb = vec3(0.65, 0.6, 0.5);
    //albedo.rgb = vec3(0.5, 0.45, 0.4);
    albedo.rgb = vec3(1);

    vec3 N = normalize(geo.normal_ws);

    vec3 camera_ws = (inverse(uCamera.viewMatrix) * vec4(0,0,0,1)).xyz;
    vec3 V = normalize(geo.position_ws - camera_ws);

#ifdef USE_DEFERRED_RENDERING
    vec4 color = albedo;
#else // USE_DEFERRED_RENDERING
    // Lighting
    vec4 color = vec4(vec3(0.0), 1.0);
    for (int i = 0 ; i < uLightCount ; ++i) {
        vec3 L = -uLightDirections[i];
        vec3 lightColor = uLightDiffuseColors[i];
        lightColor = mix(lightColor, vec3(1.0), 0.5);
        //vec3 spec = uLightSpecularColors[i];
        //float smoothness = 1.0 / uLightSmoothness[i];
        vec3 R = normalize(reflect(-L, N));
        color.rgb += (
            max(dot(N, L), 0) * lightColor * albedo.rgb
        );
    }

    color = pow(color * uColorMultiplier, vec4(uColorGamma));
#endif // USE_DEFERRED_RENDERING

    //color.rgb = N * 0.5 + 0.5;
#ifdef USE_DEFERRED_RENDERING
    GFragment fragment;
    fragment.baseColor = color.rgb;
    fragment.ws_coord = geo.position_ws;
    fragment.normal = geo.normal_ws;
    fragment.material_id = standardMaterial;
    autoPackGFragment(fragment);
#else // USE_DEFERRED_RENDERING
    out_color = color;
#endif // USE_DEFERRED_RENDERING
}
