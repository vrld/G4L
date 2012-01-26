#ifndef __G4L_VBO_H
#define __G4L_VBO_H

#include <GL/glew.h>

struct lua_State;

typedef struct bufferobject {
	GLuint id;
	GLenum target;
	GLenum usage;
	GLsizei pos;
	GLsizei max_size;
	GLsizei record_size;
	void* data;
} bufferobject;

bufferobject* l_checkbufferobject(struct lua_State* L, int idx);
int l_bufferobject_new(struct lua_State* L);

#endif
