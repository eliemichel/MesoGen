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

#include <Logger.h>

#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdio>
#include <string>
#include <sstream>
#include <memory>
#include <map>
#include <vector>
#include <array>

#ifdef _WIN32
static const char* readMode = "rb";
static const char* writeMode = "wb";
#else
static const char* readMode = "r";
static const char* writeMode = "w";
#endif

namespace Serialization {

using Document = rapidjson::Document;
using StringBuffer = rapidjson::StringBuffer;
using FileReadStream = rapidjson::FileReadStream;
using FileWriteStream = rapidjson::FileWriteStream;
using Value = rapidjson::Value;

// Base templates

class SerializationError : public std::exception
{
public:
	SerializationError(const std::string& message) : m_message(message) {}

	const char* what() const throw () {
		return m_message.c_str();
	}

private:
	std::string m_message;
};

class DeserializationError : public std::exception
{
public:
	DeserializationError(const std::string& message) : m_message(message) {}

	const char* what() const throw () {
		return m_message.c_str();
	}

private:
	std::string m_message;
};

#define S(x) #x
#define S_(x) S(x)
#define S__LINE__ S_(__LINE__)
#define DESER_ASSERT(json, expected) \
if (!(json).Is ## expected()) throw Serialization::DeserializationError("Expected " #expected " (line " S__LINE__ ") at offset " + document.GetErrorOffset())

#define DESER_ASSERT_HAS_MEMBER(json, key) \
if (!json.HasMember(key)) throw Serialization::DeserializationError(std::string("Missing key: '") + key + "'")

/*
template <typename T>
void serialize(const T& obj, Document& document, Value& json) {
	std::ostringstream ss;
	ss << "Cannot serialize type '" << typeid(T).name() << "'";
	throw SerializationError(ss.str());
	(void)obj;
	(void)document;
	(void)json;
}
*/
template <typename T>
void serialize(const T& obj, Document& document, Value& json) = delete;

template <typename T, typename Ctx>
void serialize(const T& obj, Document& document, Value& json, Ctx& context) {
	(void)context;
	serialize(obj, document, json);
}

/*
template <typename T>
void deserialize(T& obj, const Document& document, const Value& json) {
	std::ostringstream ss;
	ss << "Cannot deserialize type '" << typeid(T).name() << "'";
	throw DeserializationError(ss.str());
	(void)obj;
	(void)document;
	(void)json;
}
*/
template <typename T>
void deserialize(T& obj, const Document& document, const Value& json) = delete;

template <typename T, typename Ctx>
void deserialize(T& obj, const Document& document, const Value& json, Ctx& context) {
	(void)context;
	deserialize(obj, document, json);
}

// Template specializations

template <typename T>
void serialize(const std::shared_ptr<T>& ptr, Document& document, Value& json) {
	serialize(*ptr, document, json);
}

template <typename T, typename Ctx>
void serialize(const std::shared_ptr<T>& ptr, Document& document, Value& json, Ctx& context) {
	serialize(*ptr, document, json, context);
}

template <typename T>
void serialize(const std::vector<T>& vec, Document& document, Value& json) {
	json.SetArray();
	auto& allocator = document.GetAllocator();
	auto it = vec.cbegin();
	auto end = vec.cend();
	for (; it != end; ++it) {
		const T& item = *it;
		Value v;
		serialize(item, document, v);
		json.PushBack(v, allocator);
	}
}

template <typename T, size_t N>
void serialize(const std::array<T,N>& arr, Document& document, Value& json) {
	json.SetArray();
	auto& allocator = document.GetAllocator();
	auto it = arr.cbegin();
	auto end = arr.cend();
	for (; it != end; ++it) {
		const T& item = *it;
		Value v;
		serialize(item, document, v);
		json.PushBack(v, allocator);
	}
}

template <typename T>
void deserialize(std::shared_ptr<T>& ptr, const Document& document, const Value& json) {
	if (!ptr) ptr = std::make_shared<T>();
	deserialize(*ptr, document, json);
}

template <typename T, typename Ctx>
void deserialize(std::shared_ptr<T>& ptr, const Document& document, const Value& json, Ctx& context) {
	if (!ptr) ptr = std::make_shared<T>();
	deserialize(*ptr, document, json, context);
}
#pragma endregion

// basic types

template <>
void serialize(const bool& value, Document&, Value& json);

template <>
void deserialize(bool& value, const Document& document, const Value& json);

template <>
void serialize(const int& value, Document&, Value& json);

template <>
void deserialize(int& value, const Document& document, const Value& json);

template <>
void serialize(const float& value, Document&, Value& json);

template <>
void deserialize(float& value, const Document& document, const Value& json);

// glm::vec2

template <>
void serialize(const glm::vec2& vec, Document& document, Value& json);

template <>
void deserialize(glm::vec2& vec, const Document& document, const Value& json);

// glm::ivec2

template <>
void serialize(const glm::ivec2& vec, Document& document, Value& json);

template <>
void deserialize(glm::ivec2& vec, const Document& document, const Value& json);

// glm::vec3

template <>
void serialize(const glm::vec3& vec, Document& document, Value& json);

template <>
void deserialize(glm::vec3& vec, const Document& document, const Value& json);

// glm::vec4

template <>
void serialize(const glm::vec4& vec, Document& document, Value& json);

template <>
void deserialize(glm::vec4& vec, const Document& document, const Value& json);

// glm::mat

template<int W, int H, typename T>
void deserialize(glm::mat<W, H, T>& mat, const Document& document, const Value& json);

template<int W, int H, typename T>
void serialize(const glm::mat<W, H, T>& mat, Document& document, Value& json);

template void deserialize(glm::mat4& mat, const Document& document, const Value& json);
template void serialize(const glm::mat4& mat, Document& document, Value& json);

} // Model-specific specializations
#include "Serialization/SerializationModel.h"
namespace Serialization {

template <typename T>
void serializeTo(const T& obj, const std::string& filename) {
	Document d;

	try {
		serialize(obj, d, d);
	}
	catch (SerializationError err) {
		ERR_LOG << "SerializationError: " << err.what();
		return;
	}

	FILE* fp = nullptr;
	if (fopen_s(&fp, filename.c_str(), writeMode) != 0) {
		ERR_LOG << "Cannot open file for serialization: '" << filename << "'";
		return;
	}

	char writeBuffer[65536];
	FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));

	rapidjson::PrettyWriter<FileWriteStream> writer(os);
	d.Accept(writer);

	fclose(fp);
}

template <typename T>
void deserializeFrom(T& obj, const std::string& filename) {
	FILE* fp = nullptr;
	if (fopen_s(&fp, filename.c_str(), readMode) != 0) {
		ERR_LOG << "Cannot open file from deserialization: '" << filename << "'";
		return;
	}

	char readBuffer[65536];
	FileReadStream is(fp, readBuffer, sizeof(readBuffer));

	Document d;
	d.ParseStream(is);

	fclose(fp);

	try {
		deserialize(obj, d, d);
	}
	catch (DeserializationError err) {
		ERR_LOG << "DeserializationError: " << err.what();
		return;
	}
}

// Utility tools:

#pragma region Utility tools
template<typename T>
void AddMember(const std::string& key, const T& obj, Document& document, Value& json) {
	auto& allocator = document.GetAllocator();
	Value v;
	serialize(obj, document, v);
	Value jkey(key, allocator);
	json.AddMember(jkey, v, allocator);
}

template<typename T, typename Ctx>
void AddMember(const std::string& key, const T& obj, Document& document, Value& json, Ctx& context) {
	auto& allocator = document.GetAllocator();
	Value v;
	serialize(obj, document, v, context);
	Value jkey(key, allocator);
	json.AddMember(jkey, v, allocator);
}

template<typename T>
void AddArrayMember(const std::string& key, const std::vector<T>& vec, Document& document, Value& json) {
	auto& allocator = document.GetAllocator();

	Value vecJson(rapidjson::kArrayType);
#if 0 // for some reason this line triggers a false positive 'warning C4702: unreachable code'
	for (const T& item : vec) {
#else // so we manually unsugar it:
	auto it = vec.cbegin();
	auto end = vec.cend();
	for (; it != end; ++it) {
#endif
		const T& item = *it;
		Value v;
		serialize(item, document, v);
		vecJson.PushBack(v, allocator);
	}

	Value jkey(key, allocator);
	json.AddMember(jkey, vecJson, allocator);
}

template<typename T, typename Ctx>
void AddArrayMember(const std::string& key, const std::vector<T>& vec, Document& document, Value& json, Ctx& context) {
	auto& allocator = document.GetAllocator();

	Value vecJson(rapidjson::kArrayType);
	for (const T& item : vec) {
		Value v;
		serialize(item, document, v, context);
		vecJson.PushBack(v, allocator);
	}

	Value jkey(key, allocator);
	json.AddMember(jkey, vecJson, allocator);
}

template<typename T>
void GetArrayMember(const std::string& key, std::vector<T>& vec, const Document& document, const Value& json) {
	//Value jkey(key, allocator);
	DESER_ASSERT_HAS_MEMBER(json, key);
	DESER_ASSERT(json[key], Array);
	const auto& arr = json[key].GetArray();

	vec.resize(arr.Size());

	for (int i = 0; i < vec.size(); ++i) {
		deserialize(vec[i], document, arr[i]);
	}
}

template<typename T, typename Ctx>
void GetArrayMember(const std::string& key, std::vector<T>& vec, const Document& document, const Value& json, Ctx& ctx) {
	//Value jkey(key, allocator);
	DESER_ASSERT_HAS_MEMBER(json, key);
	DESER_ASSERT(json[key], Array);
	const auto& arr = json[key].GetArray();

	vec.resize(arr.Size());

	int i = 0;
	for (T& item : vec) {
		deserialize(item, document, arr[i++], ctx);
	}
}

template<typename T, int N>
void AddArrayMember(const std::string& key, const std::array<T,N>& vec, Document& document, Value& json) {
	auto& allocator = document.GetAllocator();

	Value vecJson(rapidjson::kArrayType);
	for (const T& item : vec) {
		Value v;
		serialize(item, document, v);
		vecJson.PushBack(v, allocator);
	}

	Value jkey(key, allocator);
	json.AddMember(jkey, vecJson, allocator);
}

template<typename T, int N, typename Ctx>
void AddArrayMember(const std::string& key, const std::array<T, N>& vec, Document& document, Value& json, Ctx& context) {
	auto& allocator = document.GetAllocator();

	Value vecJson(rapidjson::kArrayType);
	for (const T& item : vec) {
		Value v;
		serialize(item, document, v, context);
		vecJson.PushBack(v, allocator);
	}

	Value jkey(key, allocator);
	json.AddMember(jkey, vecJson, allocator);
}

template<typename T, int N>
void GetArrayMember(const std::string& key, std::array<T,N>& vec, const Document& document, const Value& json) {
	//Value jkey(key, allocator);
	DESER_ASSERT_HAS_MEMBER(json, key);
	DESER_ASSERT(json[key], Array);
	const auto& arr = json[key].GetArray();

	vec.resize(arr.Size());

	int i = 0;
	for (T& item : vec) {
		deserialize(item, document, arr[i++]);
	}
}

template<typename T, int N, typename Ctx>
void GetArrayMember(const std::string& key, std::array<T,N>& vec, const Document& document, const Value& json, Ctx& ctx, bool allowWrongSize = false) {
	//Value jkey(key, allocator);
	DESER_ASSERT_HAS_MEMBER(json, key);
	DESER_ASSERT(json[key], Array);
	const auto& arr = json[key].GetArray();

	if (arr.Size() != vec.size() && !allowWrongSize) {
		throw DeserializationError("Expected array of size " + std::to_string(vec.size()));
	}

	unsigned int i = 0;
	for (T& item : vec) {
		if (i >= arr.Size()) break;
		deserialize(item, document, arr[i++], ctx);
	}
}

}
