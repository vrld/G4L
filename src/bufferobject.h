#ifndef __G4L_BUFFEROBJECT_H
#define __G4L_BUFFEROBJECT_H

#include <glew.h>

struct lua_State;

typedef struct bufferobject {
	GLuint id;
	GLenum target;
	GLenum usage;
	GLsizei pos;
	GLsizei max_size;
	GLsizei record_size;

	GLenum element_type;
	size_t element_size;
	void* data;
} bufferobject;

bufferobject* l_checkbufferobject(struct lua_State* L, int idx);
int l_bufferobject_new(struct lua_State* L);

#endif
