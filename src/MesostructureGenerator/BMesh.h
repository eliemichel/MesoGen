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

#include <tiny_obj_loader.h>
#include <glm/glm.hpp>
#include <vector>

namespace bmesh {

struct Vertex;
struct Edge;
struct Loop;
struct Face;

struct Vertex {
	Edge* edge;
	glm::vec3 position;
};

struct Edge {
	Vertex* vertex[2];
	Edge* next[2]; // around each vertex
	Edge* prev[2];
	Loop* loop;

	int IndexOf(Vertex* v) const;
	Edge*& Next(Vertex* v);
	Edge*& Prev(Vertex* v);
};

struct Loop {
	Vertex* vertex;
	Edge* edge;
	Face* face;
	Loop* next;
	Loop* radialNext;
	Loop* prev;
	Loop* radialPrev;

	glm::vec3 normal;
};

struct Face {
	int index;
	Loop *loop;
};

struct BMesh {
	BMesh() {}
	~BMesh();
	BMesh(const BMesh&) = delete;
	BMesh& operator=(const BMesh&) = delete;

	void AddVertices(int count);
	Vertex* AddVertex();
	Edge* EnsureEdge(Vertex* v0, Vertex* v1);
	Edge* AddEdge(Vertex* v0, Vertex* v1);
	Face* AddFace(const std::vector<int>& vertexIndices);

	/**
	 * May contain some nullptr in directions where there are no faces.
	 * In directions where there is more than one face, pick one arbitrarily.
	 */
	std::vector<Face*> NeighborFaces(Face *f) const;
	std::vector<Face*> NeighborFaces(Vertex* v) const;

	std::vector<Vertex*> NeighborVertices(Face* f) const;
	std::vector<Loop*> NeighborLoops(Face* f) const;

	glm::vec3 Center(Face* f) const;

	int vertexCount() const { return static_cast<int>(vertices.size()); }
	int edgeCount() const { return static_cast<int>(edges.size()); }
	int loopCount() const { return static_cast<int>(loops.size()); }
	int faceCount() const { return static_cast<int>(faces.size()); }

	std::vector<Vertex*> vertices;
	std::vector<Edge*> edges;
	std::vector<Loop*> loops;
	std::vector<Face*> faces;

	void FromTinyObj(const tinyobj::attrib_t& attrib, const std::vector<tinyobj::shape_t>& shapes);

private:
	Loop* AddLoop(Vertex* v, Edge* e, Face* f);
};

} // namespace bmesh
