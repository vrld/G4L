#ifndef OPENGLUA_VBO_H
#define OPENGLUA_VBO_H

#include <GL/glew.h>

struct lua_State;

typedef struct vbo {
	GLuint id;
	GLenum usage;
	GLsizei pos;
	GLsizei max_size;
	GLsizei record_size;
	void* data;
} vbo;

vbo* l_checkvbo(struct lua_State* L, int idx);
int l_vbo_new(struct lua_State* L);

#endif
