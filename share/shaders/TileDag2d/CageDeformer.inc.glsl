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

struct CageDeformer {
    float cageWeights[8];
    vec3 corners[8];
    float minDeformScale;
    mat3 transform;
};

//-----------------------------------------------------------------------------

void initDeformerFromCornerMatrices(inout CageDeformer deformer, uint faceId) {
    deformer.corners[0] = geo.cornerPositions[0].xyz;
    deformer.corners[1] = geo.cornerPositions[1].xyz;
    deformer.corners[2] = geo.cornerPositions[2].xyz;
    deformer.corners[3] = geo.cornerPositions[3].xyz;
    deformer.corners[4] = geo.cornerPositions[0].xyz + geo.cornerNormals[0].xyz * 0.5;
    deformer.corners[5] = geo.cornerPositions[1].xyz + geo.cornerNormals[1].xyz * 0.5;
    deformer.corners[6] = geo.cornerPositions[2].xyz + geo.cornerNormals[2].xyz * 0.5;
    deformer.corners[7] = geo.cornerPositions[3].xyz + geo.cornerNormals[3].xyz * 0.5;
}

vec3 worldToTile(inout CageDeformer deformer, vec3 position_ws) {
    computeInverseMeanValueCoordinates(position_ws, deformer.corners, deformer.cageWeights);
    vec3 position_sts = vec3(0.0);
    for (int i = 0 ; i < 8 ; ++i) {
        position_sts += deformer.cageWeights[i] * cRestCagePoints[i].xyz;
    }
    vec3 position_ts = (position_sts + vec3(0.0, 0.0, 1.0)) * vec3(0.5, 0.5, 0.25);
    //position_ts = geo.tileVariantMatrixInverse * position_ts;
    return position_ts;
}

vec3 tileToWorld(inout CageDeformer deformer, vec3 position_ts) {
    //position_ts = geo.tileVariantMatrix * position_ts;
    vec3 position_sts = position_ts / vec3(0.5, 0.5, 0.25) - vec3(0.0, 0.0, 1.0);
    computeMeanValueCoordinates(position_sts, deformer.cageWeights);
    vec3 position_ws = vec3(0.0);
    for (int i = 0 ; i < 8 ; ++i) {
        position_ws += deformer.cageWeights[i] * deformer.corners[i].xyz;
    }
    return position_ws;
}

float minDeformScale(inout CageDeformer deformer) {
    const float restCageScale = 1;

    float e0 = sqLength(deformer.corners[1] - deformer.corners[0]);
    float sqSdfScale = e0;
    float e1 = sqLength(deformer.corners[2] - deformer.corners[1]);
    sqSdfScale = min(e1, sqSdfScale);
    float e2 = sqLength(deformer.corners[3] - deformer.corners[2]);
    sqSdfScale = min(e2, sqSdfScale);
    float e3 = sqLength(deformer.corners[0] - deformer.corners[3]);
    sqSdfScale = min(e3, sqSdfScale);
    float e4 = sqLength(deformer.corners[5] - deformer.corners[4]);
    sqSdfScale = min(e4, sqSdfScale);
    float e5 = sqLength(deformer.corners[6] - deformer.corners[5]);
    sqSdfScale = min(e5, sqSdfScale);
    float e6 = sqLength(deformer.corners[7] - deformer.corners[6]);
    sqSdfScale = min(e6, sqSdfScale);
    float e7 = sqLength(deformer.corners[4] - deformer.corners[7]);
    sqSdfScale = min(e7, sqSdfScale);
    float e8 = sqLength(deformer.corners[4] - deformer.corners[0]);
    sqSdfScale = min(e8, sqSdfScale);
    float e9 = sqLength(deformer.corners[5] - deformer.corners[1]);
    sqSdfScale = min(e9, sqSdfScale);
    float e10 = sqLength(deformer.corners[5] - deformer.corners[2]);
    sqSdfScale = min(e10, sqSdfScale);
    float e11 = sqLength(deformer.corners[7] - deformer.corners[3]);
    sqSdfScale = min(e11, sqSdfScale);
    
    deformer.minDeformScale = restCageScale * sqrt(sqSdfScale);
    return deformer.minDeformScale;
}

void getCageMinMax(inout CageDeformer deformer, out float minD, out float maxD) {
    minD = 1e9;
    maxD = -1e9;
    for (int i = 0 ; i < 8 ; ++i) {
        vec4 corner_cs = uCamera.viewMatrix * uModelMatrix * vec4(deformer.corners[i].xyz, 1.0);
        float d = length(corner_cs.xyz);
        minD = min(minD, d);
        maxD = max(maxD, d);
    }
}
