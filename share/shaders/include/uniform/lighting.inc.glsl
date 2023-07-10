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

uniform vec3[] uLightDirections = vec3[](
    vec3(-0.35254600644111633, 0.17093099653720856, -0.9200509786605835),
    vec3(-0.40816301107406616, 0.34693899750709534, 0.844415009021759),
    vec3(0.5217390060424805, 0.8260869979858398, -0.21299900114536285),
    vec3(0.6245189905166626, -0.5620669722557068, -0.5422689914703369)
);
uniform vec3[] uLightDiffuseColors = vec3[](
    vec3(1.0, 0.98, 0.95) * 3.5,
    vec3(1.0, 0.9, 0.95) * vec3(1.1, 1.0, 0.8),
    vec3(0.95, 0.95, 1.0) * vec3(1.0) * 1.0,
    vec3(0.9, 0.9, 1.0) * vec3(0.75, 1.0, 1.5) * 0.0
);
uniform float[] uLightSize = float[](
    1.0,
    2.0,
    1.0,
    2.0
);
uniform uint uLightCount = 4;
