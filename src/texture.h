#ifndef __G4L_TEXTURE_H
#define __G4L_TEXTURE_H

#include <glew.h>

struct lua_State;

typedef struct
{
	GLuint id;
	GLuint unit;
	//GLenum target; <-- later, maybe
} texture;

texture* l_checktexture(struct lua_State* L, int idx);
int l_istexture(struct lua_State* L, int idx);
int l_texture_new(struct lua_State* L);
void texture_bind(texture* tex);

#endif
