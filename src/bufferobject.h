#ifndef __G4L_BUFFEROBJECT_H
#define __G4L_BUFFEROBJECT_H

#include <glew.h>

struct lua_State;

typedef struct bufferobject
{
	GLuint  id;
	GLenum  target;
	GLenum  usage;
	GLsizei count;
	GLenum  element_type;
	GLsizei element_size;
} bufferobject;

bufferobject* l_checkbufferobject(struct lua_State* L, int idx);
int l_bufferobject_new(struct lua_State* L);

#endif
