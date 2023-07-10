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
//precision highp float;
#include "sys:defines"

layout (location = 0) out vec4 color;
layout (depth_greater) out float gl_FragDepth;

in GeometryData {
    sample vec3 position_ws;
    sample vec3 normal_ws;
    sample vec3 uv;
} geo;

uniform mat4 uModelMatrix;
uniform vec4 uColor = vec4(1.0, 1.0, 1.0, 1.0);
uniform float uRoughness = 0.5;
uniform bool uUseCheckboard = true;

#include "include/utils.inc.glsl"
#include "include/raytracing.inc.glsl"

#include "include/uniform/camera.inc.glsl"
#include "include/uniform/lighting.inc.glsl"

#include "filament/common_math.fs"
#include "filament/brdf.fs"

//-----------------------------------------------------------------------------

vec3 BRDF(vec3 v, vec3 l, vec3 n, float perceptualRoughness, vec3 diffuseColor, vec3 f0) {
    vec3 h = normalize(v + l);

    /*
    if (dot(n, v) < 0.0) {
        return vec3(-dot(n, v), 0, 0);
    } else {
        return vec3(0, 0, dot(n, v));
    }
    */

    float NoV = abs(dot(n, v)) + 1e-5;
    //float NoV = clamp(dot(n, v), 1e-5, 1.0);
    float NoL = saturate(dot(n, l));
    float NoH = saturate(dot(n, h));
    float LoH = saturate(dot(l, h));

    // perceptually linear roughness to roughness (see parameterization)
    float roughness = perceptualRoughness * perceptualRoughness;

    float D = distribution(roughness, NoH, h);
    float V = visibility(roughness, NoV, NoL);
    vec3  F = fresnel(f0, LoH);

    vec3 Fr = (D * V) * F;
    vec3 Fd = diffuseColor * diffuse(roughness, NoV, NoL, LoH);

    return (Fd + Fr) * NoL;
    //return (color * light.colorIntensity.rgb) *
    //        (light.colorIntensity.w * light.attenuation * NoL * occlusion);
}

//-----------------------------------------------------------------------------

vec3 computeLighting(vec3 V, vec3 N, vec3 P, vec3 albedo, float roughness) {
    vec3 color = vec3(0.0);
    // Lighting
    //*
    for (int i = 0 ; i < uLightCount ; ++i) {
        //if (i != 0) continue;
        vec3 L = -normalize(uLightDirections[i]);
        vec3 lightColor = uLightDiffuseColors[i];
        float size = uLightSize[i];
        float attenuation = 1.0;
        color.rgb += lightColor * BRDF(V, L, N, pow(roughness, 1/size), albedo, vec3(0.3)) * attenuation;
    }
    /*/
    color.rgb = N * 0.5 + 0.5;
    //*/
    return color;
}

//-----------------------------------------------------------------------------

void main() {
    color.a = uColor.a;

    // orthographic
    vec3 view_ws = inverse(uCamera.viewMatrix * uModelMatrix)[2].xyz;


    // UV Checkerboard
    bool checkerboard = false;
    if (uUseCheckboard) {
        vec2 uvi = step(fract(geo.uv.xy * 3), vec2(0.5));
        checkerboard = uvi.x == uvi.y;
    }

    float roughness = uRoughness;
    vec3 albedo = uColor.rgb;
    albedo *= checkerboard ? 1.0 : 0.9;
    roughness *= checkerboard ? 1.0 : 0.85;

    color.rgb = computeLighting(
        normalize(view_ws),
        normalize(geo.normal_ws),
        geo.position_ws,
        albedo,
        roughness
    );

    //color.rgb = checkerboard ? vec3(0,0,0.5) : vec3(0,0,1);
    
    //color.rgb = geo.normal_ws * 0.5 + 0.5;
    //color.rgb = normalize(view_ws) * 0.5 + 0.5;
}
