#pragma once

#include "GL.hpp"
#include "Load.hpp"

//Shader program that draws UIs on screen with ortho projection
struct UIRenderProgram {
	UIRenderProgram();
	~UIRenderProgram();

	GLuint program = 0;

	//Attribute (per-vertex variable) locations:
	GLuint PosTex_vec4 = -1U;


	//Uniform (per-invocation variable) locations:
	GLuint PROJECTION_mat4 = -1U;
	GLuint TexColor_vec3 = -1U;
	
};

extern Load< UIRenderProgram > ui_render_program;
