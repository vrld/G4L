#include "image.h"
#include "helper.h"

#include <lua.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <stdio.h>

#include <zlib.h>
#include <png.h>

#include <jpeglib.h>
#include <jerror.h>

static const char* INTERNAL_NAME = "G4L.image";

image* l_checkimage(lua_State* L, int idx)
{
	return (image*)luaL_checkudata(L, idx, INTERNAL_NAME);
}

static int l_image_map(lua_State* L)
{
	image* img = l_checkimage(L, 1);
	if (!lua_isfunction(L, 2))
		return luaL_typerror(L, 2, "function");

	rgba_pixel* pixel = (rgba_pixel*)img->data;
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

static int l_image_get(lua_State* L)
{
	image* img = l_checkimage(L, 1);
	int x = luaL_checkinteger(L, 2);
	int y = luaL_checkinteger(L, 3);

	if (x < 0 || x >= img->width || y < 0 || y > img->height)
		return luaL_error(L, "Pixel out of range: %dx%d", x,y);

	rgba_pixel* p = (rgba_pixel*)(img->data) + (x + y * img->width);
	lua_pushnumber(L, (lua_Number)(p->r) / 255.);
	lua_pushnumber(L, (lua_Number)(p->g) / 255.);
	lua_pushnumber(L, (lua_Number)(p->b) / 255.);
	lua_pushnumber(L, (lua_Number)(p->a) / 255.);
	return 4;
}

static int l_image_set(lua_State* L)
{
	image* img = l_checkimage(L, 1);
	int x = luaL_checkinteger(L, 2);
	int y = luaL_checkinteger(L, 3);

	if (x < 0 || x >= img->width || y < 0 || y > img->height)
		return luaL_error(L, "Pixel out of range: %dx%d", x,y);

	rgba_pixel new_pixel;
	new_pixel.r = (unsigned char)(luaL_checknumber(L, 4) * 255.);
	new_pixel.g = (unsigned char)(luaL_checknumber(L, 5) * 255.);
	new_pixel.b = (unsigned char)(luaL_checknumber(L, 6) * 255.);
	new_pixel.a = (unsigned char)(luaL_checknumber(L, 7) * 255.);

	rgba_pixel* p = (rgba_pixel*)(img->data) + 4 * (x + y * img->width);
	*p = new_pixel;

	lua_settop(L, 1);
	return 1;
}

static int l_image___index(lua_State* L)
{
	luaL_getmetatable(L, INTERNAL_NAME);
	lua_pushvalue(L, 2);
	lua_rawget(L, -2);
	if (!lua_isnoneornil(L, -1))
		return 1;

	image* img = (image*)lua_touserdata(L, 1);
	const char* key = lua_tostring(L, 2);
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

static int l_image___gc(lua_State* L)
{
	image* img = (image*)lua_touserdata(L, 1);
	free(img->data);
	img->data = NULL;
	return 0;
}

static void push_image_from_dimensions(lua_State* L);
static void push_image_from_file(lua_State* L);

int l_image_new(lua_State* L)
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

static void push_image_from_dimensions(lua_State* L)
{
	int w = luaL_checkinteger(L, 1);
	int h = luaL_checkinteger(L, 2);

	image* img = (image*)lua_newuserdata(L, sizeof(image));
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

// on success, return DECODE_OK
// on format mismatch, return DECODE_WRONG_FORMAT
// on error, push error message and return DECODE_ERR
static decode_status _decode_png(lua_State* L, FILE* fp, int* w, int* h, void** data);
static decode_status _decode_jpeg(lua_State* L, FILE* fp, int* w, int* h, void** data);

static void push_image_from_file(lua_State* L)
{
	const char* path = luaL_checkstring(L, 1);
	FILE* fp = fopen(path, "rb");

	if (NULL == fp)
		luaL_error(L, "Cannot open file `%s' for reading", path);

	int w, h;
	void* data = NULL;
	decode_status status = _decode_png(L, fp, &w, &h, &data);
	if (DECODE_WRONG_FORMAT == status)
		status = _decode_jpeg(L, fp, &w, &h, &data);
	fclose(fp);

	if (DECODE_WRONG_FORMAT == status)
		luaL_error(L, "Error decoding file `%s': Unsupported format", path);
	else if (DECODE_ERR == status)
		luaL_error(L, "Error decoding file `%s': %s", path, lua_tostring(L, -1));

	image* img = (image*)lua_newuserdata(L, sizeof(image));
	img->width  = w;
	img->height = h;
	img->data   = data;
}

//// PNG DECODING ////
static void _png_error_function(png_structp reader, png_const_charp error_msg)
{
	lua_State* L = (lua_State*)png_get_error_ptr(reader);
	if (NULL == L)
		fprintf(stderr, "Decoding error: %s", error_msg);
	else
		lua_pushstring(L, error_msg);
}

static decode_status _decode_png(lua_State* L, FILE* fp, int* w, int* h, void** data)
{
	png_byte signature[8];
	if (8 != fread(signature, 1, 8, fp))
	{
		rewind(fp);
		lua_pushstring(L, "Cannot file png signature");
		return DECODE_ERR;
	}

	int is_png = !png_sig_cmp(signature, 0, 8);
	if (0 == is_png)
	{
		rewind(fp);
		return DECODE_WRONG_FORMAT;
	}

	png_structp reader = NULL;
	png_infop info     = NULL;

	reader = png_create_read_struct(PNG_LIBPNG_VER_STRING, (void*)L, _png_error_function, NULL);
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

	png_init_io(reader, fp);
	png_set_sig_bytes(reader, 8);
	png_read_info(reader, info);

	png_uint_32 width      = png_get_image_width(reader, info);
	png_uint_32 height     = png_get_image_height(reader, info);
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

	*data = malloc(sizeof(png_byte) * rowbytes * height);
	if (NULL == data)
	{
		lua_pushstring(L, "Cannot allocate memory for image data");
		goto error;
	}

	png_bytep *rows = (png_bytep*)malloc(sizeof(png_bytep) * height);
	for (png_uint_32 i = 0; i < height; ++i)
		rows[i] = (png_byte*)*data + i * rowbytes;

	// FINALLY decode the image
	png_read_image(reader, rows);
	free(rows);

	// stuff to return
	*w = width;
	*h = height;
	return DECODE_OK;

error:
	if (NULL != *data)
	{
		free(*data);
		*data = NULL;
	}
	png_destroy_read_struct(
	    (NULL == reader)   ? &reader   : (png_structpp)NULL,
	    (NULL == info)     ? &info     : (png_infopp)NULL,
	    (png_infopp)NULL);
	rewind(fp);
	return DECODE_ERR;
}


//// JPEG DECODING ////
struct _jpeg_error_mgr
{
	struct jpeg_error_mgr info;
	lua_State* L;
	jmp_buf setjmp_buffer;
};

void _jpeg_error_function(j_common_ptr reader)
{
	struct _jpeg_error_mgr* err = (struct _jpeg_error_mgr*)reader->err;
	lua_pushstring(err->L, err->info.jpeg_message_table[err->info.last_jpeg_message]);
	longjmp(err->setjmp_buffer, 1);
}

static decode_status _decode_jpeg(lua_State* L, FILE* fp, int* w, int* h, void** data)
{
	struct jpeg_decompress_struct reader;
	struct _jpeg_error_mgr jerr;

	reader.err = jpeg_std_error(&jerr.info);
	jerr.info.error_exit = _jpeg_error_function;
	if (setjmp(jerr.setjmp_buffer))
		goto error;

	jpeg_create_decompress(&reader);
	jpeg_stdio_src(&reader, fp);
	int status = jpeg_read_header(&reader, 0);
	if (JPEG_HEADER_OK != status)
	{
		jpeg_destroy_decompress(&reader);
		return DECODE_WRONG_FORMAT;
	}

	jpeg_start_decompress(&reader);
	if (JCS_RGB != reader.out_color_space || reader.output_components != 3)
	{
		lua_pushstring(L, "Unsupported JPEG format: Only 8-bit RGB images are supported");
		goto error;
	}

	*w = reader.output_width;
	*h = reader.output_height;
	int row_stride = reader.output_width * reader.output_components;
	JSAMPARRAY buffer = (*reader.mem->alloc_sarray)((j_common_ptr)&reader, JPOOL_IMAGE, row_stride, 1);
	*data = malloc(sizeof(rgba_pixel) * reader.output_height * reader.output_width);
	rgba_pixel* pixel = (rgba_pixel*)*data;

	while (reader.output_scanline < reader.output_height)
	{
		if (1 != jpeg_read_scanlines(&reader, buffer, 1))
		{
			lua_pushstring(L, "Cannot read scanline");
			goto error;
		}

		// put RGB data as RGBA
		for (int x = 0; x < row_stride; x += reader.output_components, ++pixel)
		{
			memcpy(pixel, buffer[0]+x, 3);
			pixel->a = 255;
		}
	}

	jpeg_finish_decompress(&reader);
	jpeg_destroy_decompress(&reader);
	return DECODE_OK;

error:
	if (NULL != *data)
	{
		free(*data);
		*data = NULL;
	}
	jpeg_destroy_decompress(&reader);
	return DECODE_ERR;
}
