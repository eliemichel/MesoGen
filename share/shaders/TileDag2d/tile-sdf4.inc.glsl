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

const float minCurveU = -1e-2;
const float maxCurveU = 1 + 1e-2;

void findClosestPointOnPath(in vec3 p, in vec3 P0, in vec3 P1, in vec3 P2, in vec3 P3, out float bestCurveu, out vec3 bestClosestPointOnPath) {
    // Find closest point on bezier curve
    float bestSqDistance = -1;
    bestCurveu = 0;
    bestClosestPointOnPath = vec3(0.0);

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

        // Two options here:
        //   a. use the discretized segment
        //vec3 closestPointOnPath = a + lambda1 * ab;
        //   b. or reproject onto the true bezier curve using the curvilinear parameter
        vec3 closestPointOnPath = bezier(P0, P1, P2, P3, curveu);

        for (int k = 0 ; k < uRefinementSteps ; ++k) {
            {
                vec3 projected = p - closestPointOnPath;
                float sqDistance = dot(projected, projected);
                if (bestSqDistance < 0 || sqDistance < bestSqDistance) {
                    bestSqDistance = sqDistance;
                    bestCurveu = curveu;
                    bestClosestPointOnPath = closestPointOnPath;
                }
            }

            vec3 T = normalize(dbezier(P0, P1, P2, P3, curveu));
            float lambda2 = dot(T, closestPointOnPath) - dot(T, p);
            float worldSpaceToCurveSpaceScale = (1/float(uBezierSteps-1)) / length(ab);
            curveu -= lambda2 * worldSpaceToCurveSpaceScale * uRefinementFactor;
            curveu = min(max(minCurveU, curveu), maxCurveU);

            closestPointOnPath = bezier(P0, P1, P2, P3, curveu);
        }
        
        vec3 projected = p - closestPointOnPath;
        float sqDistance = dot(projected, projected);

        if (bestSqDistance < 0 || sqDistance < bestSqDistance) {
            bestSqDistance = sqDistance;
            bestCurveu = curveu;
            bestClosestPointOnPath = closestPointOnPath;
        }

        prevB = b;
        prevNb = nb;
        prevUb = ub;
    }
}

float mapSweep(vec3 p, uint sweepIndex, inout ActivePath ln) {
    vec3 P0 = bActivePathsP0[sweepIndex].xyz;
    vec3 P1 = bActivePathsP1[sweepIndex].xyz;
    vec3 P2 = bActivePathsP2[sweepIndex].xyz;
    vec3 P3 = bActivePathsP3[sweepIndex].xyz;

    vec2 startShapeCenterUv;
    vec2 endShapeCenterUv;
    if (uDeformationMode == DEFORMATION_BEZIER) {
        ActivePath2 ln2 = bActivePaths2[sweepIndex];
        startShapeCenterUv = ln2.startOffset * vec2(1.0, 0.5);
        endShapeCenterUv = ln2.endOffset * vec2(1.0, 0.5);
    } else {
        startShapeCenterUv = toTileEdgeSpace(P0, ln.startTileEdge);
        endShapeCenterUv = toTileEdgeSpace(P3, ln.endTileEdge);
    }

    float curveu;
    vec3 closestPointOnPath;
    findClosestPointOnPath(p, P0, P1, P2, P3, curveu, closestPointOnPath);
    vec3 projected = p - closestPointOnPath;

    // Build local frame
    vec3 T = normalize(dbezier(P0, P1, P2, P3, curveu));
    vec3 B = normalize(vec3(T.y, -T.x, 0));
    vec3 N = normalize(cross(T, B));

    vec2 sliceUv = vec2(
        dot(projected, B),
        -dot(projected, N)
    ) / uProfileScale;

    float startFlip = 1.0;
    float endFlip = -1.0;
    if (uDeformationMode == DEFORMATION_BEZIER) {
        ActivePath2 ln2 = bActivePaths2[sweepIndex];
        startFlip = ln2.startFlip;
        endFlip = ln2.endFlip;
    }

    vec2 sliceUvStart = startShapeCenterUv + sliceUv * vec2(startFlip, 1.0);
    vec2 sliceUvEnd = endShapeCenterUv + sliceUv * vec2(endFlip, 1.0);

    float distance2DInStartShape = mapFaceShape(sliceUvStart, ln.startTileEdge, ln.startShape);
    float distance2DInEndShape = mapFaceShape(sliceUvEnd, ln.endTileEdge, ln.endShape);
    float d = mix(distance2DInStartShape, distance2DInEndShape, smoothstep(0, 1, curveu)) * uProfileScale;
    //float d = mix(distance2DInStartShape, distance2DInEndShape, curveu) * uProfileScale;

    vec2 uv = sliceUv * vec2(endFlip, 1.0);
    //distance2DInStartShape = min(length(uv) - 0.05, 0.5 * (length(uv * vec2(1.0, 2.0) - vec2(0.05, 0.0)) - 0.03));
    //d = distance2DInStartShape + 0.05 * curveu;
    //d = distance2DInEndShape + 0.05 * (1 - curveu);

    if (uDeformationMode == DEFORMATION_BEZIER && (curveu == minCurveU || curveu == maxCurveU)) {
        // Cut bezier at ends
        vec2 w = vec2(
            d, // distance to path
            abs(dot(projected, T)) // dist to end plane
        );
        d = min(max(w.x,w.y),0.0) + length(max(w,0.0));
    }

    return d;
}

float mapCap(vec3 p, uint capIndex, inout ActivePath ln) {
    vec3 a = bActivePathsP0[capIndex].xyz;
    vec3 b = bActivePathsP1[capIndex].xyz;

    vec2 startShapeCenterUv;
    if (uDeformationMode == DEFORMATION_BEZIER) {
        ActivePath2 ln2 = bActivePaths2[capIndex];
        startShapeCenterUv = ln2.startOffset * vec2(1.0, 0.5);
    } else {
        startShapeCenterUv = toTileEdgeSpace(a, ln.startTileEdge);
    }

    vec3 ab = b - a;

    float curveu = (dot(p, ab) - dot(a, ab)) / dot(ab, ab);
    curveu = min(max(minCurveU, curveu), 0.25);

    // Build local frame
    vec3 T = normalize(ab);
    vec3 B = normalize(vec3(T.y, -T.x, 0));
    vec3 N = normalize(cross(T, B));

    vec3 closestPointOnPath = a + curveu * ab;
    vec3 projected = p - closestPointOnPath;

    vec2 sliceUv = vec2(
        dot(projected, B),
        -dot(projected, N)
    ) / uProfileScale;

    float startFlip = 1.0;
    if (uDeformationMode == DEFORMATION_BEZIER) {
        ActivePath2 ln2 = bActivePaths2[capIndex];
        startFlip = ln2.startFlip;
    }

    float s = pow(1 - 0.99 * curveu * 4, 0.25);
    //s = 1.0;
    vec2 sliceUvStart = startShapeCenterUv + sliceUv * vec2(startFlip, 1.0) / s;

    float distance2DInStartShape = mapFaceShape(sliceUvStart, ln.startTileEdge, ln.startShape);
    float d = distance2DInStartShape * s * uProfileScale;

    // Cut at ends
    vec2 w = vec2(
        d, // distance to path
        abs(dot(projected, T)) // dist to end plane
    );
    d = min(max(w.x,w.y),0.0) + length(max(w,0.0));

    return d;
}

float map(vec3 p, inout CageDeformer deformer) {
#ifdef USE_PLACEHOLDER
    return placeHolderMap(p);
#endif

    float sdf = 999;

    uint faceId = geo.faceId;

    uint activePathsStart =
        uDeformationMode == DEFORMATION_BEZIER
        ? bActivePathsOffsets[faceId]
        : uActivePathsStart;

    uint activePathsCount =
        uDeformationMode == DEFORMATION_BEZIER
        ? bActivePathsOffsets[faceId + 1] - activePathsStart
        : uActivePathsCount;

    for (uint k = 0u; k < activePathsCount; ++k) {
        uint sweepIndex = activePathsStart + k;
        vec3 P0 = bActivePathsP0[sweepIndex].xyz;
        vec3 P1 = bActivePathsP1[sweepIndex].xyz;
        vec3 P2 = bActivePathsP2[sweepIndex].xyz;
        vec3 P3 = bActivePathsP3[sweepIndex].xyz;

        //return length(bezier(P0, P1, P2, P3, 0.2) - p) - 0.02;

        ActivePath ln = bActivePaths[sweepIndex];

        float d = 999;
        bool isCap = ln.endTileEdge < 0;
        //isCap = true;
        if (isCap) {
            d = mapCap(p, sweepIndex, ln);
        } else {
            d = mapSweep(p, sweepIndex, ln);
        }

        //sdf = mSmoothUnion(sdf, d, 0.01).x;
        sdf = mUnion(sdf, d);
    }

    if (uDeformationMode == DEFORMATION_BEZIER) {
        return sdf;
    } else {
        return mIntersection(sdf, mBox(p - vec3(0, 0, 0.25), vec3(0.5, 0.5, 0.25)));
    }
}



