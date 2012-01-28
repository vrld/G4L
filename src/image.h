#ifndef __G4L_IMAGE_H
#define __G4L_IMAGE_H

struct lua_State;

static const int BPP_LUMINANCE_ALPHA = 2;
static const int BPP_RGBA = 4;

typedef struct {
	int width;
	int height;
	int bpp; // BYTES (!) per pixel
	void* data;
} image;

typedef struct {
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
} rgba_pixel;

typedef struct {
	unsigned char l;
	unsigned char a;
} grey_pixel;

image* l_checkimage(struct lua_State* L, int idx);
int l_image_new(struct lua_State* L);

#endif
