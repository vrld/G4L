#ifndef OPENGLUA_SHADER_H
#define OPENGLUA_SHADER_H

#include <GL/glew.h>

struct lua_State;

typedef struct shader {
	GLuint id;
} shader;

shader* l_checkshader(struct lua_State* L, int idx);
int l_shader_new(struct lua_State* L);

#endif
