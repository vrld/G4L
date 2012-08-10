#ifndef __G4L_IMAGE_H
#define __G4L_IMAGE_H

struct lua_State;

typedef struct
{
	int width;
	int height;
	void* data;
} image;

typedef struct
{
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
} rgba_pixel;

image* l_checkimage(struct lua_State *L, int idx);
int l_image_new(struct lua_State *L);

#endif
