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
//#define USE_PLACEHOLDER

//-----------------------------------------------------------------------------

float mapFace1(vec2 p) {
#include "sys:mapFace1"
}
float mapFace2(vec2 p) {
#include "sys:mapFace2"
}
float mapFace3(vec2 p) {
#include "sys:mapFace3"
}
float mapFace4(vec2 p) {
#include "sys:mapFace4"
}

float mapFace1Shape(vec2 p, uint shapeIndex) {
#include "sys:mapFace1Shape"
}
float mapFace2Shape(vec2 p, uint shapeIndex) {
#include "sys:mapFace2Shape"
}
float mapFace3Shape(vec2 p, uint shapeIndex) {
#include "sys:mapFace3Shape"
}
float mapFace4Shape(vec2 p, uint shapeIndex) {
#include "sys:mapFace4Shape"
}

float mapFaceShape(vec2 p, uint faceIndex, uint shapeIndex) {
    switch (faceIndex) {
    case 0:
        return mapFace1Shape(p, shapeIndex);
    case 1:
        return mapFace2Shape(p, shapeIndex);
    case 2:
        return mapFace3Shape(p, shapeIndex);
    case 3:
        return mapFace4Shape(p, shapeIndex);
    }
    return 9999.0;
}

vec2 toTileEdgeSpace(vec3 p, uint tileEdgeIndex) {
    switch (tileEdgeIndex) {
    case 0:
        return vec2(p.y + 0.5, p.z);
    case 1:
        return vec2(0.5 - p.x, p.z);
    case 2:
        return vec2(0.5 - p.y, p.z);
    case 3:
        return vec2(p.x + 0.5, p.z);
    }
    return p.xy;
}

//-----------------------------------------------------------------------------

vec3 bezier(vec3 a, vec3 b, vec3 c, vec3 d, float t) {
    vec3 Bab = a + t * (b - a);
    vec3 Bbc = b + t * (c - b);
    vec3 Bcd = c + t * (d - c);

    vec3 Babc = Bab + t * (Bbc - Bab);
    vec3 Bbcd = Bbc + t * (Bcd - Bbc);

    return Babc + t * (Bbcd - Babc);
}

// derivative
vec3 dbezier(vec3 a, vec3 b, vec3 c, vec3 d, float t) {
    vec3 u = 3 * (b - a);
    vec3 v = 3 * (c - b);
    vec3 w = 3 * (d - c);

    vec3 Buv = u + t * (v - u);
    vec3 Bvw = v + t * (w - v);

    return Buv + t * (Bvw - Buv);
}

//-----------------------------------------------------------------------------

float frameMap(vec3 p) {
    return mUnion(
        mBox(p - vec3(0.45, 0.0, 0.05), vec3(0.05, 0.5, 0.05)),
        mBox(p - vec3(-0.45, 0.0, 0.05), vec3(0.05, 0.5, 0.05))
    );
}
float placeHolderMap(vec3 p) {
    return mUnion(
        mUnion(
            frameMap(p.xyz),
            frameMap(p.yxz)
        ),
        mUnion(
            frameMap(p.xyz - vec3(0.0, 0.0, 0.4)),
            frameMap(p.yxz - vec3(0.0, 0.0, 0.4))
        )
    );
}

float map(vec3 p) {
#ifdef USE_PLACEHOLDER
    return placeHolderMap(p);
#endif

    float sdf = 999;

    for (uint k = 0u; k < uActivePathsCount; ++k) {
        vec3 P0 = bActivePathsP0[uActivePathsStart + k].xyz;
        vec3 P1 = bActivePathsP1[uActivePathsStart + k].xyz;
        vec3 P2 = bActivePathsP2[uActivePathsStart + k].xyz;
        vec3 P3 = bActivePathsP3[uActivePathsStart + k].xyz;

        ActivePath ln = bActivePaths[uActivePathsStart + k];

        vec2 startShapeCenterUv = toTileEdgeSpace(P0, ln.startTileEdge);
        vec2 endShapeCenterUv = toTileEdgeSpace(P3, ln.endTileEdge);

        vec3 prevB = P0;
        vec3 prevNb = normalize(dbezier(P0, P1, P2, P3, 0));
        float prevUb = 0;

        for (uint i = 1u; i < uBezierSteps; ++i) {
            float ua = prevUb;
            float ub = float(i) / float(uBezierSteps-1);
            vec3 a = prevB;
            vec3 na = prevNb;
            vec3 b = bezier(P0, P1, P2, P3, ub);
            vec3 nb = normalize(dbezier(P0, P1, P2, P3, ub));

            vec3 ab = b - a;

            float lambda1 = (dot(p, ab) - dot(a, ab)) / dot(ab, ab);
            lambda1 = min(max(0, lambda1), 1);
            float curveu = mix(ua, ub, lambda1);

            //vec3 projected = p - (a + lambda1 * ab);
            vec3 projected = p - bezier(P0, P1, P2, P3, curveu);;
            
            vec3 T = normalize(dbezier(P0, P1, P2, P3, curveu));
            vec3 B = normalize(vec3(T.y, -T.x, 0));
            vec3 N = normalize(cross(T, B));

            vec2 sliceUvStart = (
                startShapeCenterUv +
                vec2(
                    dot(projected, B),
                    -dot(projected, N)
                )
            );

            vec2 sliceUvEnd = (
                endShapeCenterUv +
                vec2(
                    -dot(projected, B),
                    -dot(projected, N)
                )
            );

            float d1 = mapFaceShape(sliceUvStart, ln.startTileEdge, ln.startShape);
            float d2 = mapFaceShape(sliceUvEnd, ln.endTileEdge, ln.endShape);
            //float d = mix(d1, d2, curveu);
            float d = mix(d1, d2, curveu);

            //float radius = 0.1;

            //float d = length(projected) - radius;

            // Project p on end planes
            float distToAPlane = dot(na, a) - dot(na, p);
            float distToBPlane = dot(nb, b) - dot(nb, p);

            float distToEndPlane = 0;
            if (uUseShearing) {
                distToEndPlane = max(distToAPlane, -distToBPlane);
            } else {
                distToEndPlane = (abs(lambda1 - 0.5) - 0.5) * length(ab);
            }

            vec2 w = vec2(d, distToEndPlane);

            d = min(max(w.x,w.y),0.0) + length(max(w,0.0));
            //d = length(p - a) - 0.05;

            //sdf = mSmoothUnion(sdf, d, 0.01).x;
            sdf = mUnion(sdf, d);
            sdf = mUnion(sdf, mSphere(p - (a + b) * 0.5, 0.001f));

            prevB = b;
            prevNb = nb;
            prevUb = ub;
        }
    }

    return mIntersection(sdf, mBox(p - vec3(0, 0, 0.25), vec3(0.5, 0.5, 0.25)));
}



