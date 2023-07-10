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
#include "BMesh.h"

using glm::vec3;

namespace bmesh {

int Edge::IndexOf(Vertex* v) const {
	if (vertex[0] == v) return 0;
	else if (vertex[1] == v) return 1;
	else return -1;
}

Edge*& Edge::Next(Vertex* v) {
	return next[IndexOf(v)];
}

Edge*& Edge::Prev(Vertex* v) {
	return prev[IndexOf(v)];
}

BMesh::~BMesh() {
	for (auto v : vertices) delete v;
	for (auto e : edges) delete e;
	for (auto l : loops) delete l;
	for (auto f : faces) delete f;
}

void BMesh::AddVertices(int count) {
	vertices.reserve(vertices.size() + count);
	for (int i = 0; i < count; ++i) {
		AddVertex();
	}
}

Vertex* BMesh::AddVertex() {
	Vertex* v = new Vertex();
	v->edge = nullptr;
	vertices.push_back(v);
	return v;
}

Edge* BMesh::EnsureEdge(Vertex* v0, Vertex* v1) {
	Edge* e = v0->edge;
	if (e) {
		do {
			if (
				(e->vertex[0] == v0 && e->vertex[1] == v1) ||
				(e->vertex[0] == v1 && e->vertex[1] == v0)
				) return e;
			e = e->next[e->IndexOf(v0)];
		} while (e != v0->edge);
	}

	return AddEdge(v0, v1);
}

Edge* BMesh::AddEdge(Vertex* v0, Vertex* v1) {
	Edge* e = new Edge();
	edges.push_back(e);
	e->vertex[0] = v0;
	e->vertex[1] = v1;
	e->loop = nullptr;

	// Insert
	if (v0->edge == nullptr) {
		v0->edge = e;
		e->next[0] = e;
		e->prev[0] = e;
	}
	else {
		e->next[0] = v0->edge->Next(v0);
		e->prev[0] = v0->edge;
		e->next[0]->Prev(v0) = e;
		e->prev[0]->Next(v0) = e;
	}

	if (v1->edge == nullptr) {
		v1->edge = e;
		e->next[1] = e;
		e->prev[1] = e;
	}
	else {
		e->next[1] = v1->edge->Next(v1);
		e->prev[1] = v1->edge;
		e->next[1]->Prev(v1) = e;
		e->prev[1]->Next(v1) = e;
	}

	return e;
}

Loop* BMesh::AddLoop(Vertex* v, Edge* e, Face* f) {
	Loop* l = new Loop();
	loops.push_back(l);

	l->vertex = v;
	l->edge = e;
	l->face = f;

	if (e->loop == nullptr)
	{
		e->loop = l;
		l->radialNext = l->radialPrev = l;
	}
	else
	{
		l->radialPrev = e->loop;
		l->radialNext = e->loop->radialNext;

		e->loop->radialNext->radialPrev = l;
		e->loop->radialNext = l;

		e->loop = l;
	}
	l->edge = e;

	if (f->loop == nullptr)
	{
		f->loop = l;
		l->next = l->prev = l;
	}
	else
	{
		l->prev = f->loop;
		l->next = f->loop->next;

		f->loop->next->prev = l;
		f->loop->next = l;

		f->loop = l;
	}

	return l;
}

Face* BMesh::AddFace(const std::vector<int>& vertexIndices) {
	int n = static_cast<int>(vertexIndices.size());
	if (n == 0) return nullptr;
	std::vector<Vertex*> faceVertices(n);
	for (int i = 0; i < n; ++i) {
		faceVertices[i] = vertices[vertexIndices[i]];
	}

	std::vector<Edge*> faceEdges(n);
	for (int i = 0; i < n; ++i) {
		faceEdges[i] = EnsureEdge(faceVertices[i], faceVertices[(i + 1) % n]);
	}

	Face* f = new Face();
	faces.push_back(f);
	f->index = static_cast<int>(faces.size() - 1);
	f->loop = nullptr;

	for (int i = 0; i < n; ++i) {
		AddLoop(faceVertices[i], faceEdges[i], f);
	}

	f->loop = f->loop->next;

	return f;
}

std::vector<Face*> BMesh::NeighborFaces(Face* f) const {
	std::vector<Face*> neighbors;
	Loop *it = f->loop;
	do
	{
		Face* nf = it->radialNext->face;
		if (nf != f) {
			neighbors.push_back(nf);
		}
		else {
			neighbors.push_back(nullptr);
		}

		it = it->next;
	} while (it != f->loop);

	return neighbors;
}

std::vector<Face*> BMesh::NeighborFaces(Vertex* v) const {
	std::vector<Face*> neighbors;
	Loop* it = v->edge->loop;
	do
	{
		neighbors.push_back(it->face);
		it = it->radialNext;
	} while (it != v->edge->loop);

	return neighbors;
}

std::vector<Vertex*> BMesh::NeighborVertices(Face* f) const {
	std::vector<Vertex*> neighbors;
	Loop* it = f->loop;
	do
	{
		neighbors.push_back(it->vertex);
		it = it->next;
	} while (it != f->loop);

	return neighbors;
}

std::vector<Loop*> BMesh::NeighborLoops(Face* f) const {
	std::vector<Loop*> neighbors;
	Loop* it = f->loop;
	do
	{
		neighbors.push_back(it);
		it = it->next;
	} while (it != f->loop);

	return neighbors;
}

vec3 BMesh::Center(Face* f) const {
	vec3 center(0);
	int count = 0;
	Loop* it = f->loop;
	do
	{
		center += it->vertex->position;
		++count;
		it = it->next;
	} while (it != f->loop);

	return center / static_cast<float>(count);
}

void BMesh::FromTinyObj(const tinyobj::attrib_t& attrib, const std::vector<tinyobj::shape_t>& shapes) {
	int vertCount = static_cast<int>(attrib.vertices.size() / 3);
	int baseVertexIndex = static_cast<int>(vertices.size());

	AddVertices(vertCount);
	for (int i = 0; i < vertCount; ++i) {
		vertices[i]->position = vec3(
			attrib.vertices[3 * i + 0],
			attrib.vertices[3 * i + 1],
			attrib.vertices[3 * i + 2]
		);
	}

	for (size_t s = 0; s < shapes.size(); s++) {
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			int fv = shapes[s].mesh.num_face_vertices[f];

			std::vector<int> vertexIndices(fv);
			std::vector<vec3> normals(fv);
			for (size_t v = 0; v < fv; v++) {
				tinyobj::index_t index = shapes[s].mesh.indices[index_offset + v];
				vertexIndices[v] = baseVertexIndex + index.vertex_index;
				normals[v] = vec3(
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]
				);
			}

			Face *face = AddFace(vertexIndices);

			int i = 0;
			for (Loop* loop : NeighborLoops(face)) {
				loop->normal = normals[i];
				++i;
			}


			index_offset += fv;
		}
	}
}

} // namespace bmesh
