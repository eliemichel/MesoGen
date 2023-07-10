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
#include "Serialization.h"

namespace Serialization {

// basic types

template <>
void serialize(const bool& value, Document&, Value& json) {
	json.Set(value);
}

template <>
void deserialize(bool& value, const Document& document, const Value& json) {
	(void)document;
	DESER_ASSERT(json, Bool);
	value = json.GetBool();
}

template <>
void serialize(const int& value, Document&, Value& json) {
	json.Set(value);
}

template <>
void deserialize(int& value, const Document& document, const Value& json) {
	(void)document;
	DESER_ASSERT(json, Number);
	value = json.GetInt();
}

template <>
void serialize(const float& value, Document&, Value& json) {
	json.Set(value);
}

template <>
void deserialize(float& value, const Document& document, const Value& json) {
	(void)document;
	DESER_ASSERT(json, Number);
	value = json.GetFloat();
}

// glm::vec2

template <>
void serialize(const glm::vec2& vec, Document& document, Value& json) {
	json.SetArray();
	auto& allocator = document.GetAllocator();
	json.PushBack(vec.x, allocator);
	json.PushBack(vec.y, allocator);
}


template <>
void deserialize(glm::vec2& vec, const Document& document, const Value& json) {
	DESER_ASSERT(json, Array);
	const auto& arr = json.GetArray();
	if (arr.Size() != 2) throw DeserializationError("vec2 must has a length of 2");

	DESER_ASSERT(arr[0], Number);
	DESER_ASSERT(arr[1], Number);

	vec.x = arr[0].GetFloat();
	vec.y = arr[1].GetFloat();
}

// glm::ivec2

template <>
void serialize(const glm::ivec2& vec, Document& document, Value& json) {
	json.SetArray();
	auto& allocator = document.GetAllocator();
	json.PushBack(vec.x, allocator);
	json.PushBack(vec.y, allocator);
}


template <>
void deserialize(glm::ivec2& vec, const Document& document, const Value& json) {
	DESER_ASSERT(json, Array);
	const auto& arr = json.GetArray();
	if (arr.Size() != 2) throw DeserializationError("vec2 must has a length of 2");

	DESER_ASSERT(arr[0], Number);
	DESER_ASSERT(arr[1], Number);

	vec.x = arr[0].GetInt();
	vec.y = arr[1].GetInt();
}

// glm::vec3

template <>
void serialize(const glm::vec3& vec, Document& document, Value& json) {
	json.SetArray();
	auto& allocator = document.GetAllocator();
	json.PushBack(vec.x, allocator);
	json.PushBack(vec.y, allocator);
	json.PushBack(vec.z, allocator);
}


template <>
void deserialize(glm::vec3& vec, const Document& document, const Value& json) {
	DESER_ASSERT(json, Array);
	const auto& arr = json.GetArray();
	if (arr.Size() != 3) throw DeserializationError("vec3 must has a length of 3");

	DESER_ASSERT(arr[0], Number);
	DESER_ASSERT(arr[1], Number);
	DESER_ASSERT(arr[2], Number);

	vec.x = arr[0].GetFloat();
	vec.y = arr[1].GetFloat();
	vec.z = arr[2].GetFloat();
}

// glm::vec4

template <>
void serialize(const glm::vec4& vec, Document& document, Value& json) {
	json.SetArray();
	auto& allocator = document.GetAllocator();
	json.PushBack(vec.x, allocator);
	json.PushBack(vec.y, allocator);
	json.PushBack(vec.z, allocator);
	json.PushBack(vec.w, allocator);
}


template <>
void deserialize(glm::vec4& vec, const Document& document, const Value& json) {
	DESER_ASSERT(json, Array);
	const auto& arr = json.GetArray();
	if (arr.Size() != 4) throw DeserializationError("vec4 must has a length of 4");

	DESER_ASSERT(arr[0], Number);
	DESER_ASSERT(arr[1], Number);
	DESER_ASSERT(arr[2], Number);
	DESER_ASSERT(arr[3], Number);

	vec.x = arr[0].GetFloat();
	vec.y = arr[1].GetFloat();
	vec.z = arr[2].GetFloat();
	vec.w = arr[3].GetFloat();
}

// glm::mat

template<int W, int H, typename T>
void deserialize(glm::mat<W, H, T>& mat, const Document& document, const Value& json) {
	DESER_ASSERT(json, Array);
	const auto& arr = json.GetArray();

	for (int j = 0; j < H; ++j) {
		DESER_ASSERT(arr[j], Array);
		const auto& row = arr[j].GetArray();
		for (int i = 0; i < W; ++i) {
			DESER_ASSERT(row[i], Float);
			mat[i][j] = static_cast<T>(row[i].GetFloat());
		}
	}
}

template<int W, int H, typename T>
void serialize(const glm::mat<W, H, T>& mat, Document& document, Value& json) {
	json.SetArray();
	auto& allocator = document.GetAllocator();

	for (int j = 0; j < H; ++j) {
		Value row(rapidjson::kArrayType);
		for (int i = 0; i < W; ++i) {
			row.PushBack(mat[i][j], allocator);
		}
		json.PushBack(row, allocator);
	}
}

} // namespace Serialization
