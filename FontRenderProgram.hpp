#pragma once

#include "GL.hpp"
#include "Load.hpp"

//Shader program that draws fonts on screen with ortho projection
struct FontRenderProgram {
	FontRenderProgram();
	~FontRenderProgram();

	GLuint program = 0;

	//Attribute (per-vertex variable) locations:
	GLuint PosTex_vec4 = -1U;


	//Uniform (per-invocation variable) locations:
	GLuint PROJECTION_mat4 = -1U;
	GLuint TexColor_vec3 = -1U;
	
};

extern Load< FontRenderProgram > font_render_program;
