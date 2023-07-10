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

#include "Shader.h"
#include "NonCopyable.h"

/**
 * Utility class providing an OO API to OpenGL shader program
 */
class ShaderProgram : public NonCopyable {
public:
	enum ShaderProgramType {
		RenderShader,
		ComputeShader,
		MeshletShader,
	};
public:
	ShaderProgram(const std::string& shaderName = "");
	~ShaderProgram();

	/**
	 * NB: Changing shader name does not reload it. You may want to call load() then.
	 */
	inline void setShaderName(const std::string& shaderName) { m_shaderName = shaderName; }
	inline const std::string& shaderName() const { return m_shaderName; }

	inline void setType(ShaderProgramType type) { m_type = type; }
	inline ShaderProgramType type() const { return m_type; }

	inline void define(const std::string& def) { m_defines.insert(def); }
	inline void undefine(const std::string& def) { m_defines.erase(def); }

	inline const std::set<std::string> & getDefines() const { return m_defines; }

	inline void setSnippet(const std::string& key, const std::string& value) { m_snippets[key] = value; }

	/**
	 * Load and check shaders
	 */
	void load();

	/**
	 * Check that the shader program has been successfully compiled
	 * @param name Name displayed in error message
	 * @param success status
	 */
	bool check(const std::string& name = "shader program") const;

	inline void use() const { if (isValid()) glUseProgram(m_programId); }
	inline bool isValid() const { return m_isValid; }

	// Some overloading: (add whatever you need)
	void setUniform(const std::string& name, bool value) const;
	void setUniform(const std::string& name, GLint value) const;
	void setUniform(const std::string& name, GLuint value) const;
	void setUniform(const std::string& name, GLsizeiptr value) const;
	void setUniform(const std::string& name, GLfloat value) const;
	void setUniform(const std::string& name, const glm::vec2& value) const;
	void setUniform(const std::string& name, const glm::vec3& value) const;
	void setUniform(const std::string& name, const glm::vec4& value) const;
	void setUniform(const std::string& name, const glm::ivec2& value) const;
	void setUniform(const std::string& name, const glm::uvec2& value) const;
	void setUniform(const std::string& name, const glm::mat3& value) const;
	void setUniform(const std::string& name, const glm::mat4& value) const;

	void setUniform(const std::string& name, const std::vector<glm::uvec3>& value) const;

	template <
		typename Enum,
		typename = typename std::enable_if_t<std::is_enum_v<Enum>>
	>
	void setUniform(const std::string& name, Enum value) const {
		setUniform(name, static_cast<int>(value));
	}

	bool bindUniformBlock(const std::string& uniformBlockName, GLuint buffer) const;
	bool bindStorageBlock(const std::string& storageBlockName, GLuint buffer) const;

	std::vector<std::string> getUniformList() const;

	void copy(const ShaderProgram & other);

	void dispatch(GLuint num_threads, GLuint local_size) const;
	void dispatch(glm::uvec2 num_threads, glm::uvec2 local_size) const;
	void dispatch(glm::uvec3 num_threads, glm::uvec3 local_size) const;

	GLuint raw() const { return m_programId; }

private:
	std::string m_shaderName;
	ShaderProgramType m_type;
	std::set<std::string> m_defines;
	std::map<std::string, std::string> m_snippets;
	GLuint m_programId;
	bool m_isValid;

private:
	inline GLint uniformLocation(const std::string& name) const { return m_isValid ? glGetUniformLocation(m_programId, name.c_str()) : GL_INVALID_INDEX; }
	inline GLuint uniformBlockIndex(const std::string& name) const { return m_isValid ? glGetUniformBlockIndex(m_programId, name.c_str()) : GL_INVALID_INDEX; }
	inline GLuint storageBlockIndex(const std::string& name) const { return m_isValid ? glGetProgramResourceIndex(m_programId, GL_SHADER_STORAGE_BLOCK, name.c_str()) : GL_INVALID_INDEX; }
};

