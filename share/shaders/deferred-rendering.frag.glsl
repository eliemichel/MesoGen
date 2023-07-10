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
layout (location = 1) out float out_linearDepth;

in GeometryData {
	vec2 uv;
} geo;

#define IN_GBUFFER
#include "include/gbuffer.inc.glsl"
#include "include/utils.inc.glsl"
#include "include/zbuffer.inc.glsl"

#include "include/uniform/camera.inc.glsl"
#include "include/uniform/lighting.inc.glsl"
uniform int uMsaaFactor;
uniform uvec2 uResolution; // of the gbuffer
uniform bool uExportLinearDepth;

//-----------------------------------------------------------------------------

#include "filament/common_math.fs"
#include "filament/brdf.fs"

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
    float NoL = abs(dot(n, l));
    float NoH = abs(dot(n, h));
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

vec4 computeStandardShading(in GFragment fragment, vec3 camera_ws) {
	vec3 V = -normalize(fragment.ws_coord - camera_ws);
	vec3 N = fragment.normal;
    vec3 albedo = (1.0 - fragment.metallic) * fragment.baseColor;
    //vec3 albedo = fragment.baseColor;
	float roughness = fragment.roughness;
    float reflectance = 0.5;
    vec3 f0 = 0.16 * reflectance * reflectance * (1.0 - fragment.metallic) + fragment.baseColor * fragment.metallic;
    //vec3 f0 = vec3(0.3);

    vec3 color = vec3(0.0);
    for (int i = 0 ; i < uLightCount ; ++i) {
        vec3 L = -normalize(uLightDirections[i]);
        vec3 lightColor = uLightDiffuseColors[i];
        lightColor = mix(lightColor, vec3(1.0), 0.5);
        //*
        float size = uLightSize[i];
        float attenuation = 1.0;
        color.rgb += lightColor * BRDF(V, L, N, pow(roughness, 1/size), albedo, f0) * attenuation;
        /*/
        vec3 R = normalize(reflect(-L, N));
        color.rgb += (
            max(dot(N, L), 0) * lightColor * albedo.rgb + max(dot(V, R), 0) * 0.1
        );
        //*/
    }
    return vec4(color, 1.0);
}

//-----------------------------------------------------------------------------

const int SAMPLE_COUNT = 32;
uniform float uAoRadius = 0.5;
uniform float uAoFallOff = 0.0;
uniform float uAoIntensity = 1.0;
uniform float uTime = 0.0;

float zrand(vec2 uv) {
    return fract(sin(dot(uv, vec2(12.9898, 78.233))) * 43758.5453);
}

float nrand(vec2 uv, float dx, float dy) {
    uv += vec2(dx, dy);
    float offset = zrand(1.0 * vec2(uTime.x, 0));
    return fract(sin(offset + dot(uv, vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 spherical_kernel(vec2 uv, float index) {
    // Uniformaly distributed points
    // http://mathworld.wolfram.com/SpherePointPicking.html
    float u = nrand(uv, 0, index) * 2 - 1;
    float theta = nrand(uv, 1, index) * 3.14159266 * 2;
    float u2 = sqrt(1 - u * u);
    vec3 v = vec3(u2 * cos(theta), u2 * sin(theta), u);
    // Adjustment for distance distribution.
    float l = index / SAMPLE_COUNT;
    return v * mix(0.1, 1.0, l * l);
}

float computeAO(in GFragment fragment,  vec3 camera_ws, ivec2 texel) {
    if (fragment.material_id == noMaterial) return 1.0;

	mat4 proj = uCamera.projectionMatrix;
    float depth = linearizeDepth(proj, texelFetch(uZbuffer, texel, 0).x);

	vec3 normal_cs = mat3(uCamera.viewMatrix) * fragment.normal;
    //vec3 normal_cs = fragment.normal;
	vec3 position_cs = (uCamera.viewMatrix * vec4(fragment.ws_coord, 1.0)).xyz;
    //vec3 position_cs = vec4(fragment.ws_coord, 1.0).xyz;

	float occ = 0.0;
    for (int s = 0; s < SAMPLE_COUNT; s++)
    {
        vec3 delta = spherical_kernel(gl_FragCoord.xy / vec2(uResolution), s);

        // Wants a sample in normal oriented hemisphere.
        if (dot(normal_cs, delta) < 0) delta = -delta;

        // Sampling point.
        vec3 pos_s = position_cs + delta * uAoRadius;

        // Re-project the sampling point.
        vec4 pos_sc = proj * vec4(pos_s, 1.0);
        vec2 uv_s = (pos_sc.xy / pos_sc.w + 1) * 0.5;

        // Sample a linear depth at the sampling point.
        float depth_s = linearizeDepth(proj, texture(uZbuffer, uv_s).x);

        // Occlusion test.
        float dist = -pos_s.z - depth_s;
        //if (dist > 0.01 * uAoRadius && dist < uAoRadius) ++occ;
        //if (dist > 0.01 * uAoRadius && dist < 100 * uAoRadius) ++occ;
        if (dist > 0.001) ++occ;
        //else if (dist > 0.01 * uAoRadius) occ += 0.5;
    }
    
    //float falloff = 1.0 - depth / uAoFallOff;
    //occ = occ * uAoIntensity * falloff / SAMPLE_COUNT;

    occ *= uAoIntensity / SAMPLE_COUNT;

    return saturate(1 - occ);
}

//-----------------------------------------------------------------------------

vec3 ACESFilm(vec3 x) {
    x = max(x, 0.0);
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x*(a*x+b))/(x*(c*x+d)+e));
}

//-----------------------------------------------------------------------------

vec4 computeShading(in GFragment fragment, vec3 camera_ws) {
    //return vec4(fragment.normal * 0.5 + 0.5, 1.0);
    switch (fragment.material_id) {
	case noMaterial:
		return vec4(1.0, 1.0, 1.0, 0.0);

	case forwardBaseColorMaterial:
		return vec4(fragment.baseColor, 1.0);

	case forwardNormalMaterial:
		return vec4(fragment.normal, 1.0);
	default:
	case standardMaterial: {
		vec4 color = computeStandardShading(fragment, camera_ws);// * computeAO(fragment, camera_ws);
        return vec4(ACESFilm(color.rgb * 1.1 - 0.2), color.a);
    }
	}
}

//-----------------------------------------------------------------------------

void main() {
	ivec2 texel = ivec2(gl_FragCoord.xy * uMsaaFactor);
	vec3 camera_ws = uCamera.inverseViewMatrix[3].xyz;

	color = vec4(0.0);
    if (uExportLinearDepth) {
        out_linearDepth = 0.0;
    }

    float ao = 1.0;
	GFragment fragment;
	for (int i = 0 ; i < uMsaaFactor ; ++i) {
		for (int j = 0 ; j < uMsaaFactor ; ++j) {
			autoUnpackGFragmentAt(texel + ivec2(i, j), fragment);
			color += computeShading(fragment, camera_ws);
            if (i == uMsaaFactor / 2 && j == uMsaaFactor / 2) {
                ao = computeAO(fragment, camera_ws, texel + ivec2(i, j));
            }

            if (uExportLinearDepth) {
                mat4 proj = uCamera.projectionMatrix;
                float depth = linearizeDepth01(proj, texelFetch(uZbuffer, texel + ivec2(i, j), 0).x);
                out_linearDepth += depth;
            }
		}
	}

	color /= (uMsaaFactor * uMsaaFactor);
    color.rgb *= ao;

    if (uExportLinearDepth) {
        out_linearDepth /= (uMsaaFactor * uMsaaFactor);
    }

    //color.rgb = ACESFilm(color.rgb * 1.1 - 0.2);
}
