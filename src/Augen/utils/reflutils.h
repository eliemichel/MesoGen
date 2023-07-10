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

#include "ShaderProgram.h"
#include "ReflectionAttributes.h"
#include "utils/strutils.h"

#include <Logger.h>
#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <glm/gtc/type_ptr.hpp>
#include <refl.hpp>
#include <magic_enum.hpp>

#include <string>

/**
 * Automatically bind properties using reflection.
 * The type T must have reflection enabled (see refl-cpp)
 */
template<typename T>
void autoSetUniforms(const ShaderProgram& shader, const T& properties) {
	// Automatically bind properties using reflection
	for_each(refl::reflect(properties).members, [&](auto member) {
		using type = typename decltype(member)::value_type;
		// transform property name "foo" into uniform name "uFoo"
			// (ideally this transformation should be performed at compile time)
		std::string name = "u" + std::string(member.name);
		name[1] = static_cast<char>(toupper(name[1]));
		// whatever is supported by setUniform
		if constexpr (
			std::is_same_v<type, bool>
			|| std::is_same_v<type, float>
			|| std::is_same_v<type, int>
			|| std::is_same_v<type, glm::vec2>
			|| std::is_same_v<type, glm::vec3>
			|| std::is_same_v<type, glm::vec4>
			|| std::is_same_v<type, glm::mat2>
			|| std::is_same_v<type, glm::mat3>
			|| std::is_same_v<type, glm::mat4>)
		{
			shader.setUniform(name, member(properties));
		}
		else if constexpr (std::is_enum_v<type>)
		{
			shader.setUniform(name, static_cast<int>(member(properties)));
		}
	});
}

/**
 * Ideally this would be a constexpr function forking on refl-cpp's member.name meta-type
 */
std::string toDisplayName(const std::string& name);

typedef int autoUiState;

/**
 * Automatically create imgui UI for an enum value
 * @return true iff one of the properties changed
 */
template <
	typename Enum,
	typename = std::enable_if_t<std::is_enum_v<Enum>>
>
bool autoUiField_impl(Enum &value, const std::string &displayName, autoUiState &id) {
	bool changed = false;
	// assuming int enum
	int intValue = static_cast<int>(value);
	ImGui::Text(("\n" + displayName + ":").c_str());
	ImGui::PushID(id++);
	constexpr auto options = magic_enum::enum_entries<Enum>();
	for (const auto& opt : options) {
		std::string optName = toDisplayName(std::string(opt.second));
		if (startsWith(optName, displayName)) {
			optName = optName.substr(displayName.size() + 1); // +1 for whitespace
		}
		changed = ImGui::RadioButton(optName.c_str(), &intValue, static_cast<int>(opt.first)) || changed;
	}
	ImGui::PopID();
	value = static_cast<Enum>(intValue);
	return changed;
}

/*
//#define autoUiField(props, member, id) autoUiField_impl(props.member, #member, id);
#define autoUiField(props, member, id) autoUiField_impl2(props, props.member, REFL_MAKE_CONST_STRING(#member), id);

template<int N>
struct Test {
	constexpr ::refl::util::const_string<N>& propertyName;
};
template<
	int N,
	//::refl::util::const_string<N> propertyName,
	//typename Name,
	typename Properties,
	typename T
>
bool autoUiField_impl2(Properties &properties, T& value, ::refl::util::const_string<N>& propertyName, autoUiState& id) {
	for_each(refl::reflect(properties).members, [&](auto member) {
		if constexpr (member.name == propertyName) {
			return true;
		}
	});

	return false;
}
*/

/**
 * Automatically create imgui UI
 * The type T must have reflection enabled (see refl-cpp)
 * @return true iff one of the properties changed
 */
template<typename T>
bool autoUi(T& properties) {
	int id = 0; // for disambiguation in case several enums have identically named values
	bool changed = false;
	for_each(refl::reflect(properties).members, [&](auto member) {
		using type = typename decltype(member)::value_type;
		std::string displayName = toDisplayName(std::string(member.name));

		constexpr bool hasRange = refl::descriptor::has_attribute<ReflectionAttributes::Range>(member);
		constexpr bool hasHideInDialog = refl::descriptor::has_attribute<ReflectionAttributes::HideInDialog>(member);

		if constexpr (hasHideInDialog) {
			// skip
		}
		else if constexpr (std::is_same_v<type, bool>)
		{
			changed = ImGui::Checkbox(displayName.c_str(), &member(properties)) || changed;
		}
		else if constexpr (std::is_same_v<type, float>) {
			if constexpr (hasRange) {
				constexpr auto range = refl::descriptor::get_attribute<ReflectionAttributes::Range>(member);
				changed = ImGui::SliderFloat(displayName.c_str(), &member(properties), range.minimum, range.maximum, "%.5f") || changed;
			}
			else {
				changed = ImGui::InputFloat(displayName.c_str(), &member(properties), 0.0f, 0.0f, "%.5f") || changed;
			}
		}
		else if constexpr (std::is_same_v<type, int>) {
			if constexpr (hasRange) {
				constexpr auto range = refl::descriptor::get_attribute<ReflectionAttributes::Range>(member);
				changed = ImGui::SliderInt(displayName.c_str(), &member(properties), static_cast<int>(range.minimum), static_cast<int>(range.maximum), "%.5f") || changed;
			}
			else {
				changed = ImGui::InputInt(displayName.c_str(), &member(properties)) || changed;
			}
		}
		else if constexpr (std::is_same_v<type, glm::vec2>) {
			if constexpr (hasRange) {
				constexpr auto range = refl::descriptor::get_attribute<ReflectionAttributes::Range>(member);
				changed = ImGui::SliderFloat2(displayName.c_str(), glm::value_ptr(member(properties)), range.minimum, range.maximum, "%.5f") || changed;
			}
			else {
				changed = ImGui::InputFloat2(displayName.c_str(), glm::value_ptr(member(properties)), "%.5f") || changed;
			}
		}
		else if constexpr (std::is_same_v<type, glm::vec3>) {
			if constexpr (hasRange) {
				constexpr auto range = refl::descriptor::get_attribute<ReflectionAttributes::Range>(member);
				changed = ImGui::SliderFloat3(displayName.c_str(), glm::value_ptr(member(properties)), range.minimum, range.maximum, "%.5f") || changed;
			}
			else {
				changed = ImGui::InputFloat3(displayName.c_str(), glm::value_ptr(member(properties)), "%.5f") || changed;
			}
		}
		else if constexpr (std::is_same_v<type, glm::vec4>) {
			if constexpr (hasRange) {
				constexpr auto range = refl::descriptor::get_attribute<ReflectionAttributes::Range>(member);
				changed = ImGui::SliderFloat4(displayName.c_str(), glm::value_ptr(member(properties)), range.minimum, range.maximum, "%.5f") || changed;
			}
			else {
				changed = ImGui::InputFloat4(displayName.c_str(), glm::value_ptr(member(properties)), "%.5f") || changed;
			}
		}
		else if constexpr (std::is_same_v<type, glm::ivec2>) {
			if constexpr (hasRange) {
				constexpr auto range = refl::descriptor::get_attribute<ReflectionAttributes::Range>(member);
				changed = ImGui::SliderInt2(displayName.c_str(), glm::value_ptr(member(properties)), static_cast<int>(range.minimum), static_cast<int>(range.maximum)) || changed;
			}
			else {
				changed = ImGui::InputInt2(displayName.c_str(), glm::value_ptr(member(properties))) || changed;
			}
		}
		else if constexpr (std::is_same_v<type, std::string>) {
			changed = ImGui::InputText(displayName.c_str(), &member(properties)) || changed;
		}
		else if constexpr (std::is_enum_v<type>) {
			changed = autoUiField_impl(member(properties), displayName, id) || changed;
		}
		else {
			ERR_LOG << "Unsupported type for property '" << member.name << "'";
		}
	});
	return changed;
}

// Misc utils (should end up somewhere else)
template <typename Enum>
constexpr Enum lastValue() {
	return magic_enum::enum_value<Enum>(magic_enum::enum_count<Enum>() - 1);
}


#define PROPERTIES_OPERATORS_DECL \
	bool operator==(const Properties& other); \
	bool operator!=(const Properties& other);

#define PROPERTIES_OPERATORS_DEF(Type) \
	bool Type::Properties::operator==(const Properties& other) { \
		bool isEqual = true; \
		for_each(refl::reflect(*this).members, [&](auto member) { \
			isEqual = isEqual && (member(*this) != member(other)); \
		}); \
		return isEqual; \
	} \
	bool Type::Properties::operator!=(const Properties& other) { \
		return !(*this == other); \
	}
