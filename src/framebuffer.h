#ifndef __G4L_FRAMEBUFFER_H
#define __G4L_FRAMEBUFFER_H

#include <glew.h>
#include "texture.h"

struct lua_State;

typedef struct
{
	GLuint id;
	GLuint renderbuffer;
	int width;
	int height;
} framebuffer;

framebuffer* l_checkframebuffer(struct lua_State* L, int idx);
int l_isframebuffer(struct lua_State* L, int idx);
int l_framebuffer_new(struct lua_State* L);
int l_framebuffer_bind(struct lua_State* L);

#endif
