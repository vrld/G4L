#include "image.h"
#include "helper.h"

#include <lua.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <stdio.h>

#include <zlib.h>
#include <png.h>

#include <turbojpeg.h>

static const char *INTERNAL_NAME = "G4L.image";

image *l_checkimage(lua_State *L, int idx)
{
	return (image *)luaL_checkudata(L, idx, INTERNAL_NAME);
}

static int l_image_map(lua_State *L)
{
	image *img = l_checkimage(L, 1);
	if (!lua_isfunction(L, 2))
		return luaL_typerror(L, 2, "function");

	rgba_pixel *pixel = (rgba_pixel *)img->data;
	for (int x = 0; x < img->width; ++x)
	{
		for (int y = 0; y < img->height; ++y)
		{
			lua_pushvalue(L, 2);
			lua_pushinteger(L, x);
			lua_pushinteger(L, y);
			lua_pushnumber(L, (lua_Number)(pixel->r) / 255.);
			lua_pushnumber(L, (lua_Number)(pixel->g) / 255.);
			lua_pushnumber(L, (lua_Number)(pixel->b) / 255.);
			lua_pushnumber(L, (lua_Number)(pixel->a) / 255.);
			lua_call(L, 6, 4);

			pixel->r = (unsigned char)(lua_tonumber(L, -4) * 255.);
			pixel->g = (unsigned char)(lua_tonumber(L, -3) * 255.);
			pixel->b = (unsigned char)(lua_tonumber(L, -2) * 255.);
			pixel->a = (unsigned char)(lua_tonumber(L, -1) * 255.);

			lua_pop(L, 4);
			++pixel;
		}
	}

	lua_settop(L, 1);
	return 1;
}

static int l_image_get(lua_State *L)
{
	image *img = l_checkimage(L, 1);
	int x = luaL_checkinteger(L, 2);
	int y = luaL_checkinteger(L, 3);

	if (x < 0 || x >= img->width || y < 0 || y > img->height)
		return luaL_error(L, "Pixel out of range: %dx%d", x,y);

	rgba_pixel *p = (rgba_pixel *)(img->data) + (x + y * img->width);
	lua_pushnumber(L, (lua_Number)(p->r) / 255.);
	lua_pushnumber(L, (lua_Number)(p->g) / 255.);
	lua_pushnumber(L, (lua_Number)(p->b) / 255.);
	lua_pushnumber(L, (lua_Number)(p->a) / 255.);
	return 4;
}

static int l_image_set(lua_State *L)
{
	image *img = l_checkimage(L, 1);
	int x = luaL_checkinteger(L, 2);
	int y = luaL_checkinteger(L, 3);

	if (x < 0 || x >= img->width || y < 0 || y > img->height)
		return luaL_error(L, "Pixel out of range: %dx%d", x,y);

	rgba_pixel new_pixel;
	new_pixel.r = (unsigned char)(luaL_checknumber(L, 4) * 255.);
	new_pixel.g = (unsigned char)(luaL_checknumber(L, 5) * 255.);
	new_pixel.b = (unsigned char)(luaL_checknumber(L, 6) * 255.);
	new_pixel.a = (unsigned char)(luaL_checknumber(L, 7) * 255.);

	rgba_pixel *p = (rgba_pixel *)(img->data) + 4 * (x + y * img->width);
	*p = new_pixel;

	lua_settop(L, 1);
	return 1;
}

static int l_image___index(lua_State *L)
{
	luaL_getmetatable(L, INTERNAL_NAME);
	lua_pushvalue(L, 2);
	lua_rawget(L, -2);
	if (!lua_isnoneornil(L, -1))
		return 1;

	image *img = (image *)lua_touserdata(L, 1);
	const char *key = lua_tostring(L, 2);
	if (0 == strcmp(key, "width"))
	{
		lua_pushinteger(L, img->width);
	}
	else if (0 == strcmp(key, "height"))
	{
		lua_pushinteger(L, img->height);
	}
	else
	{
		lua_pushnil(L);
	}

	return 1;
}

static int l_image___gc(lua_State *L)
{
	image *img = (image *)lua_touserdata(L, 1);
	free(img->data);
	img->data = NULL;
	return 0;
}

static void push_image_from_dimensions(lua_State *L);
static void push_image_from_file(lua_State *L);

int l_image_new(lua_State *L)
{
	if (lua_isnumber(L,1))
		push_image_from_dimensions(L);
	else
		push_image_from_file(L);

	if (luaL_newmetatable(L, INTERNAL_NAME))
	{
		luaL_reg meta[] =
		{
			{"__gc",    l_image___gc},
			{"__index", l_image___index},
			{"map",     l_image_map},
			{"get",     l_image_get},
			{"set",     l_image_set},
			{NULL, NULL}
		};
		l_registerFunctions(L, -1, meta);
	}
	lua_setmetatable(L, -2);
	return 1;
}

static void push_image_from_dimensions(lua_State *L)
{
	int w = luaL_checkinteger(L, 1);
	int h = luaL_checkinteger(L, 2);

	image *img = (image *)lua_newuserdata(L, sizeof(image));
	img->width  = w;
	img->height = h;
	img->data   = malloc(w * h * 4);
	// leave img->data uncleared for glitchy effects :)

	if (!img->data)
		luaL_error(L, "Cannot allocate image memory");
}

typedef enum
{
    DECODE_OK,
    DECODE_ERR,
    DECODE_WRONG_FORMAT
} decode_status;

typedef struct encoded_info
{
	size_t size;
	size_t pos;
	void *data;
} encoded_info;

typedef struct decoded_info
{
	int width;
	int height;
	void *data;
} decoded_info;

// on success, return DECODE_OK
// on format mismatch, return DECODE_WRONG_FORMAT
// on error, push error message and return DECODE_ERR
static decode_status _decode_png(lua_State *L, encoded_info *encoded, decoded_info *decoded);
static decode_status _decode_jpeg(lua_State *L, encoded_info *encoded, decoded_info *decoded);

static void push_image_from_buffer(lua_State *L, encoded_info *encoded)
{
	decoded_info decoded = {0,0, NULL};
	decode_status status = _decode_png(L, encoded, &decoded);
	if (DECODE_WRONG_FORMAT == status)
		status = _decode_jpeg(L, encoded, &decoded);

	if (DECODE_ERR == status)
		luaL_error(L, "Error decoding image: %s", lua_tostring(L, -1));

	image *img = (image *)lua_newuserdata(L, sizeof(image));
	img->width  = decoded.width;
	img->height = decoded.height;
	img->data   = decoded.data;
}

static void push_image_from_file(lua_State *L)
{
	encoded_info encoded = {0,0, NULL};
	const char *path = luaL_checkstring(L, 1);
	FILE *fp = fopen(path, "rb");

	if (NULL == fp)
		luaL_error(L, "Cannot open file `%s' for reading", path);

	fseek(fp, 0, SEEK_END);
	encoded.size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	encoded.data = malloc(encoded.size);
	if (NULL == encoded.data)
	{
		fclose(fp);
		luaL_error(L, "Out of memory");
	}

	fread(encoded.data, sizeof(char), encoded.size, fp);
	fclose(fp);

	push_image_from_buffer(L, &encoded);
	free(encoded.data);
}

//// PNG DECODING ////
static void _png_error_function(png_structp reader, png_const_charp error_msg)
{
	lua_State *L = (lua_State *)png_get_error_ptr(reader);
	if (NULL == L)
		fprintf(stderr, "Decoding error: %s", error_msg);
	else
		lua_pushstring(L, error_msg);
}

static void _png_read_data_function(png_structp reader, png_bytep data, png_size_t length)
{
	encoded_info *info = (encoded_info *)png_get_io_ptr(reader);
	if (info->pos + length > info->size)
	{
		_png_error_function(reader, "Requested more data than available");
		longjmp(png_jmpbuf(reader), 1);
	}

	memcpy(data, (void *)((char *)info->data + info->pos), length);
	info->pos += length;
}

static decode_status _decode_png(lua_State *L, encoded_info *encoded, decoded_info *decoded)
{
	png_byte signature[8];
	memcpy(signature, encoded->data, 8);
	int is_png = !png_sig_cmp(signature, 0, 8);
	if (0 == is_png)
		return DECODE_WRONG_FORMAT;

	png_structp reader = NULL;
	png_infop info     = NULL;

	reader = png_create_read_struct(PNG_LIBPNG_VER_STRING, (void *)L, _png_error_function, NULL);
	if (NULL == reader)
	{
		lua_pushstring(L, "Cannot create reader");
		goto error;
	}

	info = png_create_info_struct(reader);
	if (NULL == info)
	{
		lua_pushstring(L, "Cannot create header object");
		goto error;
	}

	// on error, libpng jumps here, and then to error
	if (setjmp(png_jmpbuf(reader)))
		goto error;

	png_set_read_fn(reader, encoded, _png_read_data_function);
	png_read_info(reader, info);

	decoded->width         = png_get_image_width(reader, info);
	decoded->height        = png_get_image_height(reader, info);
	png_byte    bdepth     = png_get_bit_depth(reader, info);
	png_byte    color_type = png_get_color_type(reader, info);

	// normalize image to 8 bit RGBA or 8 bit LA
	if (PNG_COLOR_TYPE_PALETTE == color_type)
		png_set_palette_to_rgb(reader);
	else if (PNG_COLOR_TYPE_GRAY == color_type)
		png_set_gray_to_rgb(reader);

	if (16 == bdepth)
	{
#if PNG_LIBPNG_VER >= 10504
		png_set_scale_16(reader);
#else
		png_set_strip_16(reader);
#endif
	}
	if (!(color_type & PNG_COLOR_MASK_ALPHA))
		png_set_add_alpha(reader, 255, PNG_FILLER_AFTER);

	// prepare decoding
	png_read_update_info(reader, info);
	png_uint_32 rowbytes = png_get_rowbytes(reader, info);

	decoded->data = malloc(sizeof(png_byte) * rowbytes * decoded->height);
	if (NULL == decoded->data)
	{
		lua_pushstring(L, "Cannot allocate memory for image data");
		goto error;
	}

	png_bytep *rows = (png_bytep *)malloc(sizeof(png_bytep) * decoded->height);
	for (png_uint_32 i = 0; i < (png_uint_32)decoded->height; ++i)
		rows[i] = (png_byte *)(decoded->data) + i * rowbytes;

	// FINALLY decode the image
	png_read_image(reader, rows);
	free(rows);

	// stuff to return
	return DECODE_OK;

error:
	if (NULL != decoded->data)
	{
		free(decoded->data);
		decoded->data = NULL;
	}
	png_destroy_read_struct(
	    (NULL != reader) ? &reader : (png_structpp)NULL,
	    (NULL != info)   ? &info   : (png_infopp)NULL,
	    (png_infopp)NULL);
	return DECODE_ERR;
}


//// JPEG DECODING ////
static decode_status _decode_jpeg(lua_State *L, encoded_info *encoded, decoded_info *decoded)
{
	tjhandle jpeg = tjInitDecompress();
	if (NULL == jpeg)
	{
		lua_pushstring(L, tjGetErrorStr());
		return DECODE_ERR;
	}

	int status;
	int unused;
	status = tjDecompressHeader2(jpeg,
	                             (unsigned char *)encoded->data, encoded->size,
	                             &(decoded->width), &(decoded->height), &unused);
	if (0 != status)
	{
		lua_pushstring(L, tjGetErrorStr());
		return DECODE_ERR;
	}

	decoded->data = malloc(sizeof(char) * 4 * decoded->width * decoded->height);
	status = tjDecompress2(jpeg,
	                       (unsigned char *)encoded->data, encoded->size,
	                       decoded->data, 0, 0, 0,
	                       TJPF_RGBA, 0);
	if (0 != status)
	{
		lua_pushstring(L, tjGetErrorStr());
		return DECODE_ERR;
	}

	return DECODE_OK;
}
