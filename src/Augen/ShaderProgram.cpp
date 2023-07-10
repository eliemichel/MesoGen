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


#include "utils/fileutils.h"
#include "Logger.h"
#include "ShaderProgram.h"

#include <glm/gtc/type_ptr.hpp>

#include <filesystem>
namespace fs = std::filesystem;

static std::string shaderFullPath(const std::string& shaderName, GLenum type) {
	static const std::string shaderRoot = joinPath(SHARE_DIR, "shaders");
	switch (type) {
	case GL_VERTEX_SHADER:
		return fixPath(joinPath(shaderRoot, shaderName + ".vert.glsl"));
	case GL_GEOMETRY_SHADER:
		return fixPath(joinPath(shaderRoot, shaderName + ".geo.glsl"));
	case GL_FRAGMENT_SHADER:
		return fixPath(joinPath(shaderRoot, shaderName + ".frag.glsl"));
	case GL_COMPUTE_SHADER:
		return fixPath(joinPath(shaderRoot, shaderName + ".comp.glsl"));
	case GL_TASK_SHADER_NV:
		return fixPath(joinPath(shaderRoot, shaderName + ".task.glsl"));
	case GL_MESH_SHADER_NV:
		return fixPath(joinPath(shaderRoot, shaderName + ".mesh.glsl"));
	default:
		return "";
	}
}

ShaderProgram::ShaderProgram(const std::string& shaderName)
	: m_shaderName(shaderName)
	, m_type(RenderShader)
	, m_isValid(false)
	, m_programId(0)
{}

ShaderProgram::~ShaderProgram()
{
	if (m_isValid) {
		glDeleteProgram(m_programId);
	}
	m_isValid = false;
}

void ShaderProgram::load() {
	m_programId = glCreateProgram();

	std::vector<std::string> defines(m_defines.begin(), m_defines.end());

	if (type() == RenderShader) {
		Shader vertexShader(GL_VERTEX_SHADER);
		vertexShader.load(shaderFullPath(m_shaderName, GL_VERTEX_SHADER), defines, m_snippets);
		vertexShader.compile();
		vertexShader.check("vertex shader");
		glAttachShader(m_programId, vertexShader.shaderId());

		std::string geometryShaderPath = shaderFullPath(m_shaderName, GL_GEOMETRY_SHADER);
		if (fs::is_regular_file(geometryShaderPath)) {
			Shader geometryShader(GL_GEOMETRY_SHADER);
			geometryShader.load(geometryShaderPath, defines, m_snippets);
			geometryShader.compile();
			geometryShader.check("geometry shader");
			glAttachShader(m_programId, geometryShader.shaderId());
		}

		Shader fragmentShader(GL_FRAGMENT_SHADER);
		fragmentShader.load(shaderFullPath(m_shaderName, GL_FRAGMENT_SHADER), defines, m_snippets);
		fragmentShader.compile();
		fragmentShader.check("fragment shader");
		glAttachShader(m_programId, fragmentShader.shaderId());
	}
	else if (type() == MeshletShader) {
		std::string taskShaderPath = shaderFullPath(m_shaderName, GL_TASK_SHADER_NV);
		if (fs::is_regular_file(taskShaderPath)) {
			Shader taskShader(GL_TASK_SHADER_NV);
			taskShader.load(taskShaderPath, defines, m_snippets);
			taskShader.compile();
			taskShader.check("task shader");
			glAttachShader(m_programId, taskShader.shaderId());
		}

		Shader meshShader(GL_MESH_SHADER_NV);
		meshShader.load(shaderFullPath(m_shaderName, GL_MESH_SHADER_NV), defines, m_snippets);
		meshShader.compile();
		meshShader.check("mesh shader");
		glAttachShader(m_programId, meshShader.shaderId());

		Shader fragmentShader(GL_FRAGMENT_SHADER);
		fragmentShader.load(shaderFullPath(m_shaderName, GL_FRAGMENT_SHADER), defines, m_snippets);
		fragmentShader.compile();
		fragmentShader.check("fragment shader");
		glAttachShader(m_programId, fragmentShader.shaderId());
	}
	else {
		Shader computeShader(GL_COMPUTE_SHADER);
		computeShader.load(shaderFullPath(m_shaderName, GL_COMPUTE_SHADER), defines, m_snippets);
		computeShader.compile();
		computeShader.check("compute shader");
		glAttachShader(m_programId, computeShader.shaderId());
	}

	glLinkProgram(m_programId);
	m_isValid = check();
}

bool ShaderProgram::check(const std::string& name) const {
	int ok;

	glGetProgramiv(m_programId, GL_LINK_STATUS, &ok);
	if (!ok) {
		int len;
		glGetProgramiv(m_programId, GL_INFO_LOG_LENGTH, &len);
		char* log = new char[len];
		glGetProgramInfoLog(m_programId, len, &len, log);
		ERR_LOG
			<< "ERROR: Unable to link " << name << " (" << m_shaderName << "). OpenGL returned:" << std::endl
			<< log << std::endl;
		delete[] log;
	}

	return ok;
}


void ShaderProgram::setUniform(const std::string& name, bool value) const {
	GLint loc = uniformLocation(name);
	if (loc != GL_INVALID_INDEX) {
		glProgramUniform1i(m_programId, loc, static_cast<GLint>(value));
	}
	else {
		// Problem with this warning: it is repeated at every call to update.
		// There should be either a caching mechanism to avoid repeating it
		// for a given uniform or an on/off toggle called in releoadShaders.
		//WARN_LOG << "Uniform does not exist: '" << name << "'";
	}
}
void ShaderProgram::setUniform(const std::string& name, GLint value) const {
	GLint loc = uniformLocation(name);
	if (loc != GL_INVALID_INDEX) {
		glProgramUniform1i(m_programId, loc, value);
	}
	else {
		// Problem with this warning: it is repeated at every call to update.
		// There should be either a caching mechanism to avoid repeating it
		// for a given uniform or an on/off toggle called in releoadShaders.
		//WARN_LOG << "Uniform does not exist: '" << name << "'";
	}
}
void ShaderProgram::setUniform(const std::string& name, GLuint value) const {
	GLint loc = uniformLocation(name);
	if (loc != GL_INVALID_INDEX) {
		glProgramUniform1ui(m_programId, loc, value);
	}
	else {
		//WARN_LOG << "Uniform does not exist: '" << name << "'";
	}
}
void ShaderProgram::setUniform(const std::string& name, GLsizeiptr value) const {
	GLint loc = uniformLocation(name);
	if (loc != GL_INVALID_INDEX) {
		glProgramUniform1ui(m_programId, loc, static_cast<GLuint>(value));
	}
	else {
		//WARN_LOG << "Uniform does not exist: '" << name << "'";
	}
}
void ShaderProgram::setUniform(const std::string& name, GLfloat value) const {
	GLint loc = uniformLocation(name);
	if (loc != GL_INVALID_INDEX) {
		glProgramUniform1f(m_programId, loc, value);
	}
	else {
		//WARN_LOG << "Uniform does not exist: '" << name << "'";
	}
}
void ShaderProgram::setUniform(const std::string& name, const glm::vec2& value) const {
	GLint loc = uniformLocation(name);
	if (loc != GL_INVALID_INDEX) {
		glProgramUniform2f(
			m_programId,
			loc,
			static_cast<GLfloat>(value.x),
			static_cast<GLfloat>(value.y));
	}
	else {
		//WARN_LOG << "Uniform does not exist: '" << name << "'";
	}
}
void ShaderProgram::setUniform(const std::string& name, const glm::vec3& value) const {
	GLint loc = uniformLocation(name);
	if (loc != GL_INVALID_INDEX) {
		glProgramUniform3f(
			m_programId,
			loc,
			static_cast<GLfloat>(value.x),
			static_cast<GLfloat>(value.y),
			static_cast<GLfloat>(value.z));
	}
	else {
		//WARN_LOG << "Uniform does not exist: '" << name << "'";
	}
}
void ShaderProgram::setUniform(const std::string& name, const glm::vec4& value) const {
	GLint loc = uniformLocation(name);
	if (loc != GL_INVALID_INDEX) {
		glProgramUniform4f(
			m_programId,
			loc,
			static_cast<GLfloat>(value.x),
			static_cast<GLfloat>(value.y),
			static_cast<GLfloat>(value.z),
			static_cast<GLfloat>(value.w));
	}
	else {
		//WARN_LOG << "Uniform does not exist: '" << name << "'";
	}
}
void ShaderProgram::setUniform(const std::string& name, const glm::ivec2& value) const {
	GLint loc = uniformLocation(name);
	if (loc != GL_INVALID_INDEX) {
		glProgramUniform2i(
			m_programId,
			loc,
			static_cast<GLint>(value.x),
			static_cast<GLint>(value.y));
	}
	else {
		//WARN_LOG << "Uniform does not exist: '" << name << "'";
	}
}
void ShaderProgram::setUniform(const std::string& name, const glm::uvec2& value) const {
	GLint loc = uniformLocation(name);
	if (loc != GL_INVALID_INDEX) {
		glProgramUniform2ui(
			m_programId,
			loc,
			static_cast<GLuint>(value.x),
			static_cast<GLuint>(value.y));
	}
	else {
		//WARN_LOG << "Uniform does not exist: '" << name << "'";
	}
}
void ShaderProgram::setUniform(const std::string& name, const glm::mat3& value) const {
	GLint loc = uniformLocation(name);
	if (loc != GL_INVALID_INDEX) {
		glProgramUniformMatrix3fv(m_programId, loc, 1, GL_FALSE, glm::value_ptr(value));
	}
	else {
		//WARN_LOG << "Uniform does not exist: '" << name << "'";
	}
}
void ShaderProgram::setUniform(const std::string& name, const glm::mat4& value) const {
	GLint loc = uniformLocation(name);
	if (loc != GL_INVALID_INDEX) {
		glProgramUniformMatrix4fv(m_programId, loc, 1, GL_FALSE, glm::value_ptr(value));
	}
	else {
		//WARN_LOG << "Uniform does not exist: '" << name << "'";
	}
}

void ShaderProgram::setUniform(const std::string& name, const std::vector<glm::uvec3>& value) const {
	GLint loc = uniformLocation(name);
	if (loc != GL_INVALID_INDEX) {
		glProgramUniform3uiv(m_programId, loc, static_cast<GLsizei>(value.size()), &value.data()->x);
	}
	else {
		//WARN_LOG << "Uniform does not exist: '" << name << "'";
	}
}

bool ShaderProgram::bindUniformBlock(const std::string& uniformBlockName, GLuint buffer) const {
	GLuint index = uniformBlockIndex(uniformBlockName);
	if (index == GL_INVALID_INDEX) {
		// WARN_LOG << "Uniform Block not found with name " << uniformBlockName; // pb: this warning is repeated at each frame, enable verbosity onl right after loading
		return false;
	}
	GLuint uniformBlockBinding = index;
	glBindBufferBase(GL_UNIFORM_BUFFER, uniformBlockBinding, buffer);
	glUniformBlockBinding(m_programId, index, uniformBlockBinding);
	return true;
}

bool ShaderProgram::bindStorageBlock(const std::string& storageBlockName, GLuint buffer) const {
	if (buffer == std::numeric_limits<GLuint>::max()) {
		return false;
	}
	GLuint index = storageBlockIndex(storageBlockName);
	if (index == GL_INVALID_INDEX) {
		// WARN_LOG << "Storage Block not found with name " << storageBlockName; // pb: this warning is repeated at each frame, enable verbosity onl right after loading
		return false;
	}
	GLuint storageBlockBinding = index;
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, storageBlockBinding, buffer);
	glShaderStorageBlockBinding(m_programId, index, storageBlockBinding);
	return true;
}

std::vector<std::string> ShaderProgram::getUniformList() const {
	GLint count;
	GLint bufSize;

	glGetProgramiv(m_programId, GL_ACTIVE_UNIFORMS, &count);
	glGetProgramiv(m_programId, GL_ACTIVE_UNIFORM_MAX_LENGTH, &bufSize);

	std::vector<GLchar>name(bufSize);

	std::vector<std::string> uniformList(count);

	for (GLuint i = 0; i < static_cast<GLuint>(count); i++) {
		GLint size;
		GLenum type;
		GLsizei length;

		glGetActiveUniform(m_programId, (GLuint)i, bufSize, &length, &size, &type, name.data());
		uniformList[i] = std::string(name.data(), length);
	}

	return uniformList;
}

void ShaderProgram::copy(const ShaderProgram & other)
{
	m_shaderName = other.m_shaderName;
	m_type = other.m_type;
	for (const auto &def : other.m_defines) {
		define(def);
	}
	for (const auto& s : other.m_snippets) {
		setSnippet(s.first, s.second);
	}
}

void ShaderProgram::dispatch(GLuint num_threads, GLuint local_size) const {
	use();
	GLuint num_groups = static_cast<GLuint>(ceil(static_cast<float>(num_threads) / local_size));
	glDispatchCompute(num_groups, 1, 1);
}

void ShaderProgram::dispatch(glm::uvec2 num_threads, glm::uvec2 local_size) const {
	if (!isValid()) return;
	use();
	glm::uvec2 num_groups(
		ceil(static_cast<float>(num_threads.x) / local_size.x),
		ceil(static_cast<float>(num_threads.y) / local_size.y)
	);
	glDispatchCompute(num_groups.x, num_groups.y, 1);
}

void ShaderProgram::dispatch(glm::uvec3 num_threads, glm::uvec3 local_size) const {
	use();
	glm::uvec3 num_groups(
		ceil(static_cast<float>(num_threads.x) / local_size.x),
		ceil(static_cast<float>(num_threads.y) / local_size.y),
		ceil(static_cast<float>(num_threads.z) / local_size.z)
	);
	glDispatchCompute(num_groups.x, num_groups.y, num_groups.z);
}
