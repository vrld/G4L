#ifndef __G4L_SHADER_H
#define __G4L_SHADER_H

#include <glew.h>

struct lua_State;

typedef struct shader
{
	GLuint id;
} shader;

shader* l_checkshader(struct lua_State* L, int idx);
int l_shader_new(struct lua_State* L);
int l_shader_set(struct lua_State* L);

#endif
