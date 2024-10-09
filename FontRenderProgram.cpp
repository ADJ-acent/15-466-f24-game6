#include "FontRenderProgram.hpp"

#include "gl_compile_program.hpp"
#include "gl_errors.hpp"

Load< FontRenderProgram > font_render_program(LoadTagEarly, []() -> FontRenderProgram const * {
	FontRenderProgram *ret = new FontRenderProgram();

	return ret;
});

FontRenderProgram::FontRenderProgram() {
	//Compile vertex and fragment shaders using the convenient 'gl_compile_program' helper function:
	program = gl_compile_program(
		//vertex shader:
		"#version 330 core\n"
		"layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>\n"
		"out vec2 TexCoords;\n"
		"uniform mat4 projection;\n"
		"void main()\n"
		"{\n"
		"    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);\n"
		"    TexCoords = vertex.zw;\n"
		"}  \n"
	,
		//fragment shader:
		"#version 330 core\n"
		"in vec2 TexCoords;\n"
		"out vec4 color;\n"
		"uniform sampler2D text;\n"
		"uniform vec3 textColor;\n"
		"void main()\n"
		"{\n"    
		"	vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);\n"
		"	color = vec4(textColor, 1.0) * sampled;\n"
		"} \n" 
	);
	//As you can see above, adjacent strings in C/C++ are concatenated.
	// this is very useful for writing long shader programs inline.

	//look up the locations of vertex attributes:
	PosTex_vec4 = glGetAttribLocation(program, "vertex");

	//look up the locations of uniforms:
	PROJECTION_mat4 = glGetUniformLocation(program, "projection");
	TexColor_vec3 = glGetUniformLocation(program, "textColor");

	GLuint TEX_sampler2D = glGetUniformLocation(program, "text");

	//set TEX to always refer to texture binding zero:
	glUseProgram(program); //bind program -- glUniform* calls refer to this program now

	glUniform1i(TEX_sampler2D, 0); //set TEX to sample from GL_TEXTURE0

	glUseProgram(0); //unbind program -- glUniform* calls refer to ??? now
}

FontRenderProgram::~FontRenderProgram() {
	glDeleteProgram(program);
	program = 0;
}

