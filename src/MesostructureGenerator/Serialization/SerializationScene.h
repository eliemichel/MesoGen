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

#include "Serialization.h"

#include <refl.hpp>
#include <magic_enum.hpp>

#include <string>

class App;

namespace Serialization {

template <>
void serialize(const App& app, Document& document, Value& json);

template <>
void deserialize(App& app, const Document& document, const Value& json);

template<typename T>
Value autoSerialize(const T& properties, Document& document) {
	auto& allocator = document.GetAllocator();
	Value json(rapidjson::kObjectType);
	for_each(refl::reflect(properties).members, [&](auto member) {
		using type = typename decltype(member)::value_type;
		std::string name(member.name);
		Value key;
		key.SetString(name.c_str(), (rapidjson::SizeType)name.size(), allocator);

		if constexpr (std::is_same_v<type, std::string>) {
			Value value;
			std::string valueAsString(member(properties));
			value.SetString(valueAsString.c_str(), (rapidjson::SizeType)valueAsString.size(), allocator);
			json.AddMember(key, value, allocator);
		}
		else if constexpr (std::is_enum_v<type>) {
			Value value;
			std::string_view valueAsString = magic_enum::enum_name<type>(member(properties));
			value.SetString(valueAsString.data(), (rapidjson::SizeType)valueAsString.size(), allocator);
			json.AddMember(key, value, allocator);
		}
		else {
			Value value;
			serialize(member(properties), document, value);
			json.AddMember(key, value, allocator);
		}
	});
	return json;
}

template<typename T>
void autoDeserialize(T& properties, const Document& document, const Value& json, bool ignoreMissing = true) {
	for_each(refl::reflect(properties).members, [&](auto member) {
		using type = typename decltype(member)::value_type;
		std::string name(member.name);
		rapidjson::GenericStringRef key(name.c_str(), (rapidjson::SizeType)name.size());

		if (!json.HasMember(key)) {
			if (ignoreMissing) return;
			throw Serialization::DeserializationError(std::string("Missing key: '") + name + "'");
		}

		if constexpr (std::is_same_v<type, std::string>) {
			member(properties) = json[name].GetString();
		}
		else if constexpr (std::is_enum_v<type>) {
			const Value& value = json[name];
			if (value.IsInt()) {
				auto maybe_value = magic_enum::enum_cast<type>(value.GetInt());
				if (maybe_value) {
					member(properties) = *maybe_value;
				}
				else {
					throw Serialization::DeserializationError(std::string("Invalid enum value at key: '") + name + "': " + std::to_string(value.GetInt()));
				}
			}
			else if (value.IsString()) {
				std::string valueAsString = value.GetString();
				auto entries = magic_enum::enum_entries<type>();
				bool found = false;
				for (const auto& e : entries) {
					if (std::string(e.second) == valueAsString) {
						member(properties) = e.first;
						found = true;
						break;
					}
				}
				if (!found) {
					throw Serialization::DeserializationError(std::string("Invalid enum value at key: '") + name + "': " + valueAsString);
				}
			}
			else {
				throw Serialization::DeserializationError(std::string("Enum value must be either an int or a string, at key: '") + name + "'");
			}
		}
		else {
			deserialize(member(properties), document, json[name]);
		}
	});
}

}
