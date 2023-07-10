# Copyright (c) 2020 -- Elie Michel <elie.michel@telecom-paris.fr>
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# The Software is provided "as is", without warranty of any kind, express or
# implied, including but not limited to the warranties of merchantability,
# fitness for a particular purpose and non-infringement. In no event shall the
# authors or copyright holders be liable for any claim, damages or other
# liability, whether in an action of contract, tort or otherwise, arising
# from, out of or in connection with the software or the use or other dealings
# in the Software.

###############################################################################

# This dictionnary holds the list of OpenGL functions made depreciated, along
# with some help message displayed as a warning when using it.
# Values can be either a single word, replaced by "Use <word> instead." or a
# sentence, used as is.
# Some references:
#  - http://on-demand.gputechconf.com/siggraph/2014/presentation/SG4121-OpenGL-Update-NVIDIA-GPUs.pdf (p22+)
depreciated_functions = {
# Textures
	"glGenTextures": "glCreateTextures",
	"glBindTexture": "There should be no need to bind a Texture object if you use DSA functions like glCreateTextures.",
	"glGenBuffers": "glCreateBuffers",
	"glActiveTexture": "Use glBindTextureUnit instead of glActiveTexture+glBindTexture.",
	"glTexParameterf": "glTextureParameterf",
	"glTexParameteri": "glTextureParameteri",
	"glTexParameterfv": "glTextureParameterfv",
	"glTexParameteriv": "glTextureParameteriv",
	"glTexParameterIiv": "glTextureParameterIiv",
	"glTexParameterIuiv": "glTextureParameterIuiv",
	"glTexSubImage1D": "glTextureSubImage1D",
	"glTexSubImage2D": "glTextureSubImage2D",
	"glTexSubImage3D": "glTextureSubImage3D",
	"glTexImage1D": "glTextureStorage1D",
	"glTexImage2D": "glTextureStorage2D",
	"glTexImage3D": "glTextureStorage3D",
	"glGetTexParameteriv": "glGetTextureParameteriv",
	"glGetTexParameterfv": "glGetTextureParameterfv",
	"glGetTexParameterIiv": "glGetTextureParameterIiv",
	"glGetTexParameterIuiv": "glGetTextureParameterIuiv",
	"glGetTexLevelParameteriv": "glGetTextureLevelParameteriv",
	"glGetTexLevelParameterfv": "glGetTextureLevelParameterfv",
	
	"glGetCompressedTexImage": "glGetCompressedTextureImage",
	"glGetnCompressedTexImage": "glGetCompressedTextureImage",
	"glGetTexImage": "glGetTextureImage",
	"glGetnTexImage": "glGetTextureImage",

	# Uniforms
	"glUniform1f": "glProgramUniform1f",
	"glUniform2f": "glProgramUniform2f",
	"glUniform3f": "glProgramUniform3f",
	"glUniform4f": "glProgramUniform4f",
	"glUniform1i": "glProgramUniform1i",
	"glUniform2i": "glProgramUniform2i",
	"glUniform3i": "glProgramUniform3i",
	"glUniform4i": "glProgramUniform4i",
	"glUniform1ui": "glProgramUniform1ui",
	"glUniform2ui": "glProgramUniform2ui",
	"glUniform3ui": "glProgramUniform3ui",
	"glUniform4ui": "glProgramUniform4ui",
	"glUniform1fv": "glProgramUniform1fv",
	"glUniform2fv": "glProgramUniform2fv",
	"glUniform3fv": "glProgramUniform3fv",
	"glUniform4fv": "glProgramUniform4fv",
	"glUniform1iv": "glProgramUniform1iv",
	"glUniform2iv": "glProgramUniform2iv",
	"glUniform3iv": "glProgramUniform3iv",
	"glUniform4iv": "glProgramUniform4iv",
	"glUniform1uiv": "glProgramUniform1uiv",
	"glUniform2uiv": "glProgramUniform2uiv",
	"glUniform3uiv": "glProgramUniform3uiv",
	"glUniform4uiv": "glProgramUniform4uiv",
	"glUniformMatrix2fv": "glProgramUniformMatrix2fv",
	"glUniformMatrix3fv": "glProgramUniformMatrix3fv",
	"glUniformMatrix4fv": "glProgramUniformMatrix4fv",
	"glUniformMatrix2x3fv": "glProgramUniformMatrix2x3fv",
	"glUniformMatrix3x2fv": "glProgramUniformMatrix3x2fv",
	"glUniformMatrix2x4fv": "glProgramUniformMatrix2x4fv",
	"glUniformMatrix4x2fv": "glProgramUniformMatrix4x2fv",
	"glUniformMatrix3x4fv": "glProgramUniformMatrix3x4fv",
	"glUniformMatrix4x3fv": "glProgramUniformMatrix4x3fv",

	# Renderbuffers
	"glGenRenderbuffers": "glCreateRenderbuffers",
	"glBindRenderbuffer": "There should be no need to bind a Renderbuffer object if you use DSA functions like glCreateRenderbuffers.",
	"glGetRenderbufferParameteriv": "glGetNamedRenderbufferParameteriv",

	# Framebuffers
	"glGenFramebuffers": "glCreateFramebuffers",
	# "glBindFramebuffer": "There should be no need to bind a Framebuffer object if you use DSA functions like glCreateFramebuffers.",
	"glFramebufferTexture": "glNamedFramebufferTexture",
	"glFramebufferTextureLayer": "glNamedFramebufferTextureLayer",
	"glDrawBuffer": "glNamedFramebufferDrawBuffer",
	"glDrawBuffers": "glNamedFramebufferDrawBuffers",
	"glReadBuffer": "glNamedFramebufferReadBuffer",
	"glInvalidateFramebufferSubData": "glInvalidateNamedFramebufferSubData",
	"glInvalidateFramebufferData": "glInvalidateNamedFramebufferData",
	"glClearBufferiv": "glClearNamedFramebufferiv",
	"glClearBufferuiv": "glClearNamedFramebfferuiv",
	"glClearBufferfv": "glClearNamedFramebufferfv",
	"glClearBufferfi": "glClearNamedFramebufferfi",
	"glBlitFramebuffer": "glBlitNamedFramebuffer",
	"glCheckFramebufferStatus": "glCheckNamedFramebufferStatus",
	"glFramebufferParameteri": "glNamedFramebufferParameteri",
	"glGetFramebufferParameteriv": "glGetNamedFramebufferParameteriv",
	"glGetFramebufferAttachmentParameteriv": "glGetNamedFramebufferAttachmentParameteriv",

	# Buffers
	"glGenBuffers": "glCreateBuffers",
	#"glBindBuffer": "There should be no need to bind a Buffer object if you use DSA functions like glCreateBuffers. If you want an equivalent of glBindBuffer(GL_ARRAY_BUFFER), use glVertexArrayVertexBuffer instead. If you want an equivalent of glBindBuffer(ELEMENT_ARRAY_BUFFER), use glVertexArrayElementBuffer instead.",
	"glBufferSubData": "glNamedBufferSubData",
	"glBufferData": "glNamedBufferData",
	"glCopyBufferSubData": "glCopyNamedBufferSubData",
	"glClearBufferSubData": "glClearBufferSubData",
	"glClearBufferData": "glClearBufferData",
	"glMapBuffer": "glMapNamedBuffer",
	"glMapBufferRange": "glMapNamedBufferRange",
	"glUnmapBuffer": "glUnmapNamedBuffer",
	"glFlushMappedBufferRange": "glFlushMappedNamedBufferRange",
	"glGetBufferParameteriv": "glGetNamedBufferParameteriv",
	"glGetBufferParameteri64v": "glGetNamedBufferParameteri64v",
	"glGetBufferPointerv": "glGetNamedBufferPointerv",
	"glGetBufferSubData": "glGetNamedBufferSubData",

	# Transform Feedbacks
	"glGenTransformFeedbacks": "glCreateTransformFeedbacks",
	"glBindTransformFeedback": "There should be no need to bind a Transform Feedback object if you use DSA functions like glCreateTransformFeedbacks.",
	# "glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER)": "glTransformFeedbackBufferBase",  # cannot restrict use of the function, glBindBufferBase is still needed for uniform blocks for instance
	"glBindBufferRange": "glTransformFeedbackBufferRange",

	# Vertex Arrays
	"glGenVertexArrays": "glCreateVertexArrays",
	# "glBindVertexArray": "There should be no need to bind a Vertex Array object if you use DSA functions like glCreateVertexArrays.",  # I don't see how to avoid this
	"glEnableVertexAttribArray": "glEnableVertexArrayAttrib",
	"glDisableVertexAttribArray": "glDisableVertexArrayAttrib",
	"glBindVertexBuffer": "glVertexArrayVertexBuffer",
	"glBindVertexBuffers": "glVertexArrayVertexBuffers",
	"glVertexAttribFormat": "glVertexArrayAttribFormat",
	"glVertexAttribIFormat": "glVertexArrayAttribIFormat",
	"glVertexAttribLFormat": "glVertexArrayAttribLFormat",
	"glVertexBindingDivisor": "glVertexArrayBindingDivisor",

	# Queries
	"glGenQueries": "glCreateQueries",

	# Robustness
	"glReadPixels": "This can lead to memory overflows. Use glReadnPixels for safe read instead.",
	"glGetUniformfv": "This can lead to memory overflows. Use glGetnUniformfv for safe get instead.",
	"glGetUniformiv": "This can lead to memory overflows. Use glGetnUniformiv for safe get instead.",
	"glGetUniformuiv": "This can lead to memory overflows. Use glGetnUniformuiv for safe get instead.",
	"glGetUniformdv": "This can lead to memory overflows. Use glGetnUniformdv for safe get instead.",
}

###############################################################################

MODERN_GLAD_BEGIN = """
/**
 * Wrapper around glad.h that flags most functions as depreciated (because they are).
 * by Elie Michel (c) 2020
 * Requires C++14 (for the [[depreciated]] attribute)
 */

#include <glad/glad.h>

#ifndef __modern_glad_h_
#define __modern_glad_h_

"""

MODERN_GLAD_END = """

#endif // __modern_glad_h_
"""

MODERN_GLAD_FUNCTION_TPL = """
#undef {0}
#define {0}(...) {{auto {0} [[deprecated("Not part of ModernGlad. {1}")]] = [](){{}}; {0}; }}
"""

###############################################################################

import os
import sys

# polyfill for python2
def makedirs(dirname, exist_ok=False):
    if not exist_ok or not os.path.exists(dirname):
        os.makedirs(dirname)

def main():
	header_file = sys.argv[1]
	print("Generating ModernGLAD header at '{}'...".format(header_file))
	makedirs(os.path.dirname(header_file), exist_ok=True)

	with open(header_file, "w") as f:
		f.write(MODERN_GLAD_BEGIN)
		for func, reason in depreciated_functions.items():
			if " " not in reason:
				reason = "Use {} instead.".format(reason)
			f.write(MODERN_GLAD_FUNCTION_TPL.format(func, reason))
		f.write(MODERN_GLAD_END)

if __name__ == "__main__":
	main()
