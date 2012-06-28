#include "math.h"
#include "helper.h"

#include <lauxlib.h>
#include <lualib.h>

#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.141592653589793238462643383279
#endif

static const char* VEC_INTERNAL_NAME = "G4L.math.Vector";
static const char* MAT_INTERNAL_NAME = "G4L.math.Matrix";
static const char* MATPROXY_INTERNAL_NAME = "G4L.math.Matrix.Proxy";

int l_isvec(int n, lua_State* L, int idx)
{
	vec4* p = (vec4*)lua_touserdata(L, idx);
	if (NULL == p)
		return 0;

	luaL_getmetatable(L, VEC_INTERNAL_NAME);
	lua_getmetatable(L, idx);
	int equal = lua_rawequal(L, -1, -2);
	lua_pop(L, 2);

	return equal && (p->dim == n);
}

int l_isanyvec(lua_State* L, int idx)
{
	void* p = lua_touserdata(L, idx);
	if (NULL == p)
		return 0;

	luaL_getmetatable(L, VEC_INTERNAL_NAME);
	lua_getmetatable(L, idx);
	int equal = lua_rawequal(L, -1, -2);
	lua_pop(L, 2);

	return equal;
}

int l_ismat(int i, int k, lua_State* L, int idx)
{
	mat44* p = (mat44*)lua_touserdata(L, idx);
	if (NULL == p)
		return 0;

	luaL_getmetatable(L, MAT_INTERNAL_NAME);
	lua_getmetatable(L, idx);
	int equal = lua_rawequal(L, -1, -2);
	lua_pop(L, 2);

	return equal && (p->rows == i) && (p->cols == k);
}

int l_isanymat(lua_State* L, int idx)
{
	void* p = lua_touserdata(L, idx);
	if (NULL == p)
		return 0;

	luaL_getmetatable(L, MAT_INTERNAL_NAME);
	lua_getmetatable(L, idx);
	int equal = lua_rawequal(L, -1, -2);
	lua_pop(L, 2);

	return equal;
}

void* l_checkvec(int n, lua_State* L, int idx)
{
	vec4* v = (vec4*)luaL_checkudata(L, idx, VEC_INTERNAL_NAME);
	if ((NULL == v) || (v->dim != n))
	{
		char type[5] = "vec?";
		type[3] = (char)n + '0';
		luaL_typerror(L, idx, type);
	}
	return (void*)v;
}

void* l_checkmat(int rows, int cols, lua_State* L, int idx)
{
	mat44* m = (mat44*)luaL_checkudata(L, idx, MAT_INTERNAL_NAME);
	if ((NULL == m) || (m->rows != rows) || (m->cols != cols))
	{
		char type[6] = "mat??";
		type[3] = (char)(rows + 48);
		type[4] = (char)(cols + 48);
		luaL_typerror(L, idx, type);
	}
	return (void*)m;
}

// forward declarations for __unm, __clone, etc.
static int l_vec_new(lua_State* L);
static int l_mat_new(lua_State* L);


////< VECTOR >//////////////////////////////////////////////////////////////////////

#define FOR_VECTOR(iter, dim, expr) do { int iter = 0; \
		switch (dim) { \
			case 4:  expr; ++iter; \
			case 3:  expr; ++iter; \
			case 2:  expr; ++iter; \
			case 1:  expr; ++iter; \
		} \
	} while (0)

// vector meta methods
static int l_vec___index(lua_State* L)
{
	vec4* v = (vec4*)lua_touserdata(L, 1);

	luaL_getmetatable(L, VEC_INTERNAL_NAME);
	lua_pushvalue(L, 2);
	lua_rawget(L, -2);

	if (!lua_isnoneornil(L, -1))
		return 1;

	const char* key = luaL_checkstring(L, 2);
	int n = (int)key[0] - (int)'x';
	if (n >= 0 && n < 4)
		lua_pushnumber(L, v->v[n]);
	else if (n == -1) // w
		lua_pushnumber(L, v->v[0]);
	else
		lua_pushnil(L);
	return 1;
}

static int l_vec___newindex(lua_State* L)
{
	vec4* v = (vec4*)lua_touserdata(L, 1);
	const char* key = luaL_checkstring(L, 2);
	int n = (int)key[0] - (int)'x';
	if (n >= v->dim)
		return luaL_error(L, "Cannot set component %c in vec%d", key[0], v->dim);
	v->v[n] = luaL_checknumber(L, 3);
	return 0;
}

static int l_vec___tostring(lua_State* L)
{
	vec4* v = (vec4*)lua_touserdata(L, 1);

	luaL_Buffer B;
	luaL_buffinit(L, &B);
	luaL_addstring(&B, "vec");
	luaL_addchar(&B, (char)v->dim + '0');
	luaL_addchar(&B, '(');
	FOR_VECTOR(i, v->dim,
	           lua_pushnumber(L, v->v[i]);
	           luaL_addvalue(&B);
	           luaL_addchar(&B, i+1 < v->dim ? ',' : ')'));

	luaL_pushresult(&B);
	return 1;
}

static int l_vec___unm(lua_State* L)
{
	vec4* v = (vec4*)lua_touserdata(L, 1);
	lua_pushcfunction(L, l_vec_new);
	FOR_VECTOR(i, v->dim,
	           lua_pushnumber(L, v->v[i]));
	lua_call(L, v->dim, 1);
	return 1;
}

static int l_vec___add(lua_State* L)
{
	vec4* a = (vec4*)lua_touserdata(L, 1);
	vec4* b = (vec4*)l_checkvec(a->dim, L, 2);

	lua_pushcfunction(L, l_vec_new);
	FOR_VECTOR(i, a->dim,
	           lua_pushnumber(L, a->v[i] + b->v[i]));
	lua_call(L, a->dim, 1);
	return 1;
}

static int l_vec___sub(lua_State* L)
{
	vec4* a = (vec4*)lua_touserdata(L, 1);
	vec4* b = (vec4*)l_checkvec(a->dim, L, 2);

	lua_pushcfunction(L, l_vec_new);
	FOR_VECTOR(i, a->dim,
	           lua_pushnumber(L, a->v[i] - b->v[i]));
	lua_call(L, a->dim, 1);
	return 1;
}

static int l_vec___mul(lua_State* L)
{
	if (l_isanyvec(L, 1))   // v * s or v * v
	{
		vec4* a = (vec4*)lua_touserdata(L, 1);
		lua_pushcfunction(L, l_vec_new);

		if (lua_isnumber(L, 2))   // v * s -- scalar product
		{
			GLfloat s = lua_tonumber(L, 2);
			FOR_VECTOR(i, a->dim,
			           lua_pushnumber(L, a->v[i] * s));
			lua_call(L, a->dim, 1);
		}
		else     // v * v -- dot product
		{
			vec4* b = (vec4*)l_checkvec(a->dim, L, 2);
			FOR_VECTOR(i, a->dim,
			           lua_pushnumber(L, a->v[i] * b->v[i]));
		}
	}
	else if (lua_isnumber(L, 1))     // s * v
	{
		GLfloat s = lua_tonumber(L, 1);
		vec4* a = (vec4*)lua_touserdata(L, 2);
		FOR_VECTOR(i, a->dim,
		           lua_pushnumber(L, a->v[i] * s));
		lua_call(L, a->dim, 1);

	}
	return 1;
}

static int l_vec___div(lua_State* L)
{
	GLfloat s = luaL_checknumber(L, 2);
	vec4* a = (vec4*)lua_touserdata(L, 1);

	lua_pushcfunction(L, l_vec_new);
	FOR_VECTOR(i, a->dim,
	           lua_pushnumber(L, a->v[i] / s));
	lua_call(L, a->dim, 1);
	return 1;
}

// dim = 2  -->  a ^ b = det(a,b)
// dim = 3  -->  a ^ b = a x b
// dim = 4  -->  undefined.
//    TODO?: a ^ b ^ c = <wedge produdct>
//          (a1 b2 c3 - a1 b3 c2 - a2 b1 c3 + a2 b3 c1 + a3 b1 c2 - a3 b2 c1,
//           a1 b2 c4 - a1 b4 c2 - a2 b1 c4 + a2 b4 c1 + a4 b1 c2 - a4 b2 c1,
//           a1 b3 c4 - a1 b4 c3 - a3 b1 c4 + a3 b4 c1 + a4 b1 c3 - a4 b3 c1,
//           a2 b3 c4 - a2 b4 c3 - a3 b2 c4 + a3 b4 c2 + a4 b2 c3 - a4 b3 c2
static int l_vec___pow(lua_State* L)
{
	vec4* a = (vec4*)lua_touserdata(L, 1);
	vec4* b = (vec4*)l_checkvec(a->dim, L, 2);

	if (a->dim == 2)
	{
		lua_pushnumber(L, a->v[0]*b->v[1] - a->v[1]*b->v[0]);
	}
	else if (a->dim == 3)
	{
		lua_pushcfunction(L, l_vec_new);
		lua_pushnumber(L, a->v[1]*b->v[2] - a->v[2]*b->v[1]);
		lua_pushnumber(L, a->v[2]*b->v[0] - a->v[0]*b->v[2]);
		lua_pushnumber(L, a->v[0]*b->v[1] - a->v[1]*b->v[0]);
		lua_call(L, a->dim, 1);
	}
	else
	{
		return luaL_error(L, "a ^ b undefined with dim(a) = dim(b) = %d", a->dim);
	}
	return 1;
}

// permul
static int l_vec___concat(lua_State* L)
{
	int isvec_arg1 = l_isanyvec(L, 1);
	int isvec_arg2 = l_isanyvec(L, 2);

	// per element multiplication
	if (isvec_arg1 && isvec_arg2)
	{
		vec4* a = (vec4*)lua_touserdata(L, 1);
		vec4* b = (vec4*)l_checkvec(a->dim, L, 2);

		lua_pushcfunction(L, l_vec_new);
		FOR_VECTOR(i, a->dim,
		           lua_pushnumber(L, a->v[i] * b->v[i]));
		lua_call(L, a->dim, 1);
		return 1;
	}

	// string concatenation
	luaL_Buffer b;
	luaL_buffinit(L, &b);

	if (isvec_arg1)
	{
		l_vec___tostring(L);
		lua_pushvalue(L, 3);
	}
	else
	{
		// places tostring(arg2) at index 2
		lua_pushcfunction(L, l_vec___tostring);
		lua_insert(L, 2);
		lua_call(L, 1, 1);
		lua_pushvalue(L, 1);
	}

	luaL_addvalue(&b);
	lua_pushvalue(L, 2);
	luaL_addvalue(&b);
	luaL_pushresult(&b );
	return 1;
}

static int l_vec___len(lua_State* L)
{
	vec4* a = (vec4*)lua_touserdata(L, 1);
	lua_pushinteger(L, a->dim);
	return 1;
}

static int l_vec___eq(lua_State* L)
{
	if (!l_isanyvec(L, 1) || !l_isanyvec(L, 2))
	{
		lua_pushboolean(L, 0);
		return 1;
	}
	vec4* a = (vec4*)lua_touserdata(L, 1);
	vec4* b = (vec4*)lua_touserdata(L, 2);

	if (a->dim != b->dim)
	{
		lua_pushboolean(L, 0);
		return 1;
	}

	int equal = 1;
	FOR_VECTOR(i, a->dim,
	           equal = equal && (a->v[i] == b->v[i]));
	lua_pushboolean(L, equal);
	return 1;
}

static int l_vec___lt(lua_State* L)
{
	if (!l_isanyvec(L, 1) || !l_isanyvec(L, 2))
	{
		lua_pushboolean(L, 0);
		return 1;
	}
	vec4* a = (vec4*)lua_touserdata(L, 1);
	vec4* b = (vec4*)lua_touserdata(L, 2);

	if (a->dim != b->dim)
	{
		lua_pushboolean(L, a->dim < b->dim);
		return 1;
	}

	int less = 1;
	FOR_VECTOR(i, a->dim, less = less && (a->v[i] < b->v[i]));
	lua_pushboolean(L, less);
	return 1;
}

static int l_vec___le(lua_State* L)
{
	if (!l_isanyvec(L, 1) || !l_isanyvec(L, 2))
	{
		lua_pushboolean(L, 0);
		return 1;
	}
	vec4* a = (vec4*)lua_touserdata(L, 1);
	vec4* b = (vec4*)lua_touserdata(L, 2);

	if (a->dim != b->dim)
	{
		lua_pushboolean(L, a->dim < b->dim);
		return 1;
	}

	// a.x < b.x or (a.x == b.x and (a.y < b.y and ...))
	switch (a->dim)
	{
	case 4:
		if (a->v[3] != b->v[3])
		{
			lua_pushboolean(L, a->v[3] < b->v[3]);
			break;
		}
	case 3:
		if (a->v[2] != b->v[2])
		{
			lua_pushboolean(L, a->v[2] < b->v[2]);
			break;
		}
	case 2:
		if (a->v[1] != b->v[1])
		{
			lua_pushboolean(L, a->v[1] < b->v[1]);
			break;
		}
	}
	lua_pushboolean(L, a->v[0] <= b->v[0]);
	return 1;
}

// vector methods
static int l_vec_clone(lua_State* L)
{
	vec4* a = (vec4*)lua_touserdata(L, 1);
	lua_pushcfunction(L, l_vec_new);
	FOR_VECTOR(i, a->dim,
	           lua_pushnumber(L, a->v[i]));
	lua_call(L, a->dim, 1);
	return 1;
}

static int l_vec_map(lua_State* L)
{
	static const char* index_name[] = { "x", "y", "z", "w" };

	vec4* a = (vec4*)lua_touserdata(L, 1);
	if (!lua_isfunction(L, 2))
		return luaL_typerror(L, 2, "function");

	FOR_VECTOR(i, a->dim,
	           lua_pushvalue(L, 2);
	           lua_pushnumber(L, a->v[i]);
	           lua_pushstring(L, index_name[i]);
	           lua_call(L, 2, 1);
	           a->v[i] = lua_tonumber(L, -1));
	lua_settop(L, 1);
	return 1;
}

static int l_vec_unpack(lua_State* L)
{
	vec4* a = (vec4*)lua_touserdata(L, 1);
	FOR_VECTOR(i, a->dim,
	           lua_pushnumber(L, a->v[i]));
	return a->dim;
}

static int l_vec_reset(lua_State* L)
{
	vec4* a = (vec4*)lua_touserdata(L, 1);
	GLfloat b[4];
	FOR_VECTOR(i, a->dim,
	           b[i] = luaL_checknumber(L, i+2));
	memcpy(a->v, b, a->dim * sizeof(GLfloat));

	lua_settop(L, 1);
	return 1;
}

static int l_vec_len(lua_State* L)
{
	vec4* a = (vec4*)lua_touserdata(L, 1);
	GLfloat len = .0;
	FOR_VECTOR(i, a->dim,
	           len += a->v[i] * a->v[i]);
	lua_pushnumber(L, sqrt(len));
	return 1;
}

static int l_vec_len2(lua_State* L)
{
	vec4* a = (vec4*)lua_touserdata(L, 1);
	GLfloat len = .0;
	FOR_VECTOR(i, a->dim,
	           len += a->v[i] * a->v[i]);
	lua_pushnumber(L, len);
	return 1;
}

static int l_vec_dist(lua_State* L)
{
	vec4* a = (vec4*)lua_touserdata(L, 1);
	vec4* b = (vec4*)l_checkvec(a->dim, L, 2);
	GLfloat len = .0;
	GLfloat x;
	FOR_VECTOR(i, a->dim,
	           x = a->v[i] - b->v[i];
	           len += x * x);
	lua_pushnumber(L, sqrt(len));
	return 1;
}

static int l_vec_permul(lua_State* L)
{
	return l_vec___concat(L);
}

static int l_vec_normalized(lua_State* L)
{
	vec4* a = (vec4*)lua_touserdata(L, 1);
	GLfloat len = .0;
	FOR_VECTOR(i, a->dim,
	           len += a->v[i] * a->v[i]);
	len = sqrt(len);

	lua_pushcfunction(L, l_vec_new);
	FOR_VECTOR(i, a->dim,
	           lua_pushnumber(L, a->v[i] / len));
	lua_call(L, a->dim, 1);
	return 1;
}

static int l_vec_normalize_inplace(lua_State* L)
{
	vec4* a = (vec4*)lua_touserdata(L, 1);
	GLfloat len = .0;
	FOR_VECTOR(i, a->dim,
	           len += a->v[i] * a->v[i]);
	len = sqrt(len);

	FOR_VECTOR(i, a->dim,
	           a->v[i] /= len);
	return 1;
}

static int l_vec_projectOn(lua_State* L)
{
	vec4* a = (vec4*)lua_touserdata(L, 1);
	vec4* b = (vec4*)l_checkvec(a->dim, L, 2);

	GLfloat len = .0;
	GLfloat s = .0;
	FOR_VECTOR(i, a->dim,
	           len += b->v[i]*b->v[i];
	           s += a->v[i]*b->v[i]);
	s /= len;

	lua_pushcfunction(L, l_vec_new);
	FOR_VECTOR(i, a->dim,
	           lua_pushvalue(L, s * b->v[i]));
	lua_call(L, a->dim, 1);

	return 1;
}

static int l_vec_mirrorOn(lua_State* L)
{
	vec4* a = (vec4*)lua_touserdata(L, 1);
	vec4* b = (vec4*)l_checkvec(a->dim, L, 2);

	GLfloat len = .0;
	GLfloat s = .0;
	FOR_VECTOR(i, a->dim,
	           len += b->v[i]*b->v[i];
	           s += a->v[i]*b->v[i]);
	s *= 2. / len;

	lua_pushcfunction(L, l_vec_new);
	FOR_VECTOR(i, a->dim,
	           lua_pushvalue(L, s * b->v[i] - a->v[i]));
	lua_call(L, a->dim, 1);

	return 1;
}

static int l_vec_diag(lua_State* L)
{
	vec4* v = (vec4*)luaL_checkudata(L, 1, VEC_INTERNAL_NAME);
	lua_settop(L, 0);

	FOR_VECTOR(i, v->dim,
	           FOR_VECTOR(k, i,
	                      lua_pushnumber(L, .0));
	           lua_pushnumber(L, v->v[i]);
	           FOR_VECTOR(k, v->dim-i-1,
	                      lua_pushnumber(L, .0)));
	return l_mat_new(L);
}


// vector constructor
static int l_vec_new(lua_State* L)
{
	int dim = lua_gettop(L);
	vec4* v = NULL;
	switch (dim)
	{
	case 2:
		v = (vec4*)lua_newuserdata(L, sizeof(vec2));
		break;
	case 3:
		v = (vec4*)lua_newuserdata(L, sizeof(vec3));
		break;
	case 4:
		v = (vec4*)lua_newuserdata(L, sizeof(vec4));
		break;
	default:
		return luaL_error(L, "Invalid vector dimension: %d", dim);
	}
	v->dim = dim;

	for (int i = 1; i <= dim; ++i)
		v->v[i-1] = luaL_checknumber(L, i);

	if (luaL_newmetatable(L, VEC_INTERNAL_NAME))
	{
		luaL_reg meta[] =
		{
			// vector meta methods
			{"__index",           l_vec___index},
			{"__newindex",        l_vec___newindex},
			{"__tostring",        l_vec___tostring},
			{"__unm",             l_vec___unm},
			{"__add",             l_vec___add},
			{"__sub",             l_vec___sub},
			{"__mul",             l_vec___mul},
			{"__div",             l_vec___div},
			{"__pow",             l_vec___pow},
			{"__concat",          l_vec___concat},
			{"__len",             l_vec___len},
			{"__eq",              l_vec___eq},
			{"__lt",              l_vec___lt},
			{"__le",              l_vec___le},

			// vector methods
			{"clone",             l_vec_clone},
			{"map",               l_vec_map},
			{"unpack",            l_vec_unpack},
			{"reset",             l_vec_reset},
			{"len",               l_vec_len},
			{"len2",              l_vec_len2},
			{"dist",              l_vec_dist},
			{"permul",            l_vec_permul},
			{"normalized",        l_vec_normalized},
			{"normalize_inplace", l_vec_normalize_inplace},
			{"projectOn",         l_vec_projectOn},
			{"mirrorOn",          l_vec_mirrorOn},
			{"diag",              l_vec_diag},

			{NULL, NULL}
		};
		l_registerFunctions(L, -1, meta);
	}
	lua_setmetatable(L, -2);

	return 1;
}

////< MATRIX >//////////////////////////////////////////////////////////////////////
//
#define FOR_MAT_ITER(var, max, expr) do { int var = 0; \
		switch(max) {\
			case 4:  expr; ++var; \
			case 3:  expr; ++var; \
			case 2:  expr; ++var; \
			default: expr; ++var; \
		} \
	} while (0)

#define FOR_MAT(i1, i2, rows, cols, expr) \
	FOR_MAT_ITER(i1, rows, FOR_MAT_ITER(i2, cols, expr))

// matrix row proxy
typedef struct matrix_row_proxy
{
	int rowsize;
	GLfloat* row;
} matrix_row_proxy;

static int l_matrix_row_proxy___index(lua_State* L)
{
	if (!lua_isnumber(L,2))
		return luaL_error(L, "Invalid matrix column: %s", lua_tostring(L, 2));

	matrix_row_proxy* p = (matrix_row_proxy*)lua_touserdata(L, 1);
	int n = lua_tointeger(L, 2);
	if (n < 1 || n > p->rowsize)
		return luaL_error(L, "Column-index out of bounds: %d (1 <= k <= %d)", n, p->rowsize);

	lua_pushnumber(L, p->row[n-1]);
	return 1;
}

static int l_matrix_row_proxy___newindex(lua_State* L)
{
	if (!lua_isnumber(L,2))
		return luaL_error(L, "Invalid matrix column: %s", lua_tostring(L, 2));

	matrix_row_proxy* p = (matrix_row_proxy*)lua_touserdata(L, 1);
	int n = lua_tonumber(L, 2);
	if (n < 1 || n > p->rowsize)
		return luaL_error(L, "Column-index out of bounds: %d (1 <= k <= %d)", n, p->rowsize);

	GLfloat x = luaL_checknumber(L, 3);
	p->row[n-1] = x;
	return 1;
}

// matrix meta methods
static int l_mat___index(lua_State* L)
{
	mat44* m = (mat44*)lua_touserdata(L, 1);

	luaL_getmetatable(L, MAT_INTERNAL_NAME);
	lua_pushvalue(L, 2);
	lua_rawget(L, -2);
	if (!lua_isnoneornil(L, -1))
		return 1;
	if (!lua_isnumber(L, 2))
		return 0;

	int n = lua_tointeger(L, 2);
	if (n < 1 || n > m->rows)
		return luaL_error(L, "Row-Index out of bounds: %d (1 <= i <= %d)", n, m->rows);

	matrix_row_proxy* p = (matrix_row_proxy*)lua_newuserdata(L, sizeof(matrix_row_proxy));
	p->row = m->m + (n-1) * m->cols;
	p->rowsize = m->cols;

	if (luaL_newmetatable(L, MATPROXY_INTERNAL_NAME))
	{
		luaL_Reg meta[] =
		{
			{"__index",    l_matrix_row_proxy___index},
			{"__newindex", l_matrix_row_proxy___newindex},
			{NULL, NULL}
		};
		l_registerFunctions(L, -1, meta);
	}
	lua_setmetatable(L, -2);

	return 1;
}

static int l_mat___tostring(lua_State* L)
{
	mat44* m = (mat44*)lua_touserdata(L, 1);

	luaL_Buffer B;
	luaL_buffinit(L, &B);
	luaL_addstring(&B, "mat");
	luaL_addchar(&B, (char)m->rows + '0');
	luaL_addchar(&B, (char)m->cols + '0');

	luaL_addchar(&B, '(');
	FOR_MAT(i, k, m->rows, m->cols,
	        lua_pushnumber(L, m->m[i * m->cols + k]);
	        luaL_addvalue(&B);
	        luaL_addchar(&B, i+1 == m->rows && k+1 == m->cols ? ')' : ',' );
	        if (i+1 != m->rows && k+1 == m->cols)
	        luaL_addchar(&B, ' '));
	luaL_pushresult(&B);
	return 1;
}

static int l_mat___unm(lua_State* L)
{
	mat44* m = (mat44*)lua_touserdata(L, 1);

	lua_pushcfunction(L, l_mat_new);
	FOR_MAT(i, k, m->rows, m->cols,
	        lua_pushnumber(L, m->m[i * m->cols + k]));
	lua_call(L, m->rows * m->cols, 1);
	return 1;
}

static int l_mat___add(lua_State* L)
{
	mat44* a = (mat44*)lua_touserdata(L, 1);
	mat44* b = (mat44*)l_checkmat(a->rows, a->cols, L, 2);

	lua_pushcfunction(L, l_mat_new);
	FOR_MAT(i, k, a->rows, a->cols,
	        lua_pushnumber(L, a->m[i * a->cols + k] + b->m[i * b->cols + k]));
	lua_call(L, a->rows * a->cols, 1);
	return 1;
}

static int l_mat___sub(lua_State* L)
{
	mat44* a = (mat44*)lua_touserdata(L, 1);
	mat44* b = (mat44*)l_checkmat(a->rows, a->cols, L, 2);

	lua_pushcfunction(L, l_mat_new);
	FOR_MAT(i, k, a->rows, a->cols,
	        lua_pushnumber(L, a->m[i * a->cols + k] - b->m[i * b->cols + k]));
	lua_call(L, a->rows * a->cols, 1);
	return 1;
}

static int l_mat___mul(lua_State* L)
{
	int is_anymat_1 = l_isanymat(L, 1);
	if (is_anymat_1)
	{
		if (l_isanymat(L, 2))
		{
			// matrix * matrix = matrix
			mat44* a = (mat44*)lua_touserdata(L, 1);
			mat44* b = (mat44*)l_checkmat(a->cols, a->rows, L, 2);

			GLfloat res[16] = {.0,.0,.0,.0, .0,.0,.0,.0, .0,.0,.0,.0, .0,.0,.0,.0};
			FOR_MAT(i, k, a->rows, a->cols,
			        FOR_MAT_ITER(r, a->cols,
			                     res[i * a->cols + k] += a->m[i * a->cols + r] * b->m[r * b->cols + k]));

			lua_pushcfunction(L, l_mat_new);
			FOR_MAT(i, k, a->rows, b->cols,
			        lua_pushnumber(L, res[i * b->cols + k]));
			lua_call(L, a->rows * b->cols, 1);
		}
		else if (l_isanyvec(L, 2))
		{
			// matrix * vector = vector
			mat44* m = (mat44*)lua_touserdata(L, 1);
			vec4* v = (vec4*)l_checkvec(m->cols, L, 2);

			GLfloat res[4] = {.0,.0,.0,.0};
			FOR_MAT(i, k, m->rows, m->cols,
			        res[i] += m->m[i*m->cols + k] * v->v[k]);

			lua_pushcfunction(L, l_vec_new);
			FOR_VECTOR(i, v->dim,
			           lua_pushnumber(L, res[i]));
			lua_call(L, v->dim, 1);
		}
		else
		{
			// matrix * scalar = matrix
			mat44* m = (mat44*)lua_touserdata(L, 1);
			GLfloat s = luaL_checknumber(L, 2);

			lua_pushcfunction(L, l_mat_new);
			FOR_MAT(i, k, m->rows, m->cols,
			        lua_pushnumber(L, m->m[i * m->cols + k] * s));
			lua_call(L, m->rows * m->cols, 1);
		}
	}
	else
	{
		// scalar * matrix = matrix
		GLfloat s = luaL_checknumber(L, 1);
		mat44* m = (mat44*)lua_touserdata(L, 2);

		lua_pushcfunction(L, l_mat_new);
		FOR_MAT(i, k, m->rows, m->cols,
		        lua_pushnumber(L, m->m[i * m->cols + k] * s));
		lua_call(L, m->rows * m->cols, 1);
	}

	return 1;
}

static int l_mat___div(lua_State* L)
{
	mat44* m;
	GLfloat s;
	if (l_isanymat(L, 1))
	{
		m = (mat44*)lua_touserdata(L, 1);
		s = luaL_checknumber(L, 2);
	}
	else
	{
		s = luaL_checknumber(L, 1);
		m = (mat44*)lua_touserdata(L, 2);
	}

	lua_pushcfunction(L, l_mat_new);
	FOR_MAT(i, k, m->rows, m->cols,
	        lua_pushnumber(L, m->m[i * m->cols + k] / s));
	lua_call(L, m->rows * m->cols, 1);
	return 1;
}

static int l_mat___concat(lua_State* L)
{
	int ismat_arg1 = l_isanymat(L, 1);
	int ismat_arg2 = l_isanymat(L, 2);

	// per element multiplication
	if (ismat_arg1 && ismat_arg2)
	{
		mat44* a = (mat44*)lua_touserdata(L, 1);
		mat44* b = (mat44*)l_checkmat(a->rows, a->cols, L, 2);

		lua_pushcfunction(L, l_mat_new);
		FOR_MAT(i, k, a->rows, a->cols,
		        lua_pushnumber(L, a->m[i * a->cols + k] * b->m[i * b->cols + k]));
		lua_call(L, a->rows * a->cols, 1);
		return 1;
	}

	// string concatenation
	luaL_Buffer b;
	luaL_buffinit(L, &b);

	if (ismat_arg1)
	{
		l_mat___tostring(L);
		lua_pushvalue(L, 3);
	}
	else
	{
		// places tostring(arg2) at index 2
		lua_pushcfunction(L, l_mat___tostring);
		lua_insert(L, 2);
		lua_call(L, 1, 1);
		lua_pushvalue(L, 1);
	}

	luaL_addvalue(&b);
	lua_pushvalue(L, 2);
	luaL_addvalue(&b);
	luaL_pushresult(&b );
	return 1;
}

static int l_mat___len(lua_State* L)
{
	mat44* m = (mat44*)lua_touserdata(L, 1);
	lua_pushinteger(L, m->rows);
	return 1;
}

static int l_mat___eq(lua_State* L)
{
	if (!l_isanymat(L,1) && !l_isanymat(L, 2))
	{
		lua_pushboolean(L, 0);
		return 1;
	}

	mat44* a = (mat44*)lua_touserdata(L, 1);
	mat44* b = (mat44*)lua_touserdata(L, 2);

	if (a->rows != b-> rows || a->cols != b-> cols)
	{
		lua_pushboolean(L, 0);
		return 1;
	}

	int equal = 1;
	FOR_MAT(i, k, a->rows, a->cols,
	        equal = equal && (a->m[i * a->cols + k] != b->m[i * b->cols + k]));

	lua_pushboolean(L, equal);
	return 1;
}

static int l_mat___call(lua_State* L)
{
	mat44* a = (mat44*)lua_touserdata(L, 1);
	int i = luaL_checkint(L, 2);
	int k = luaL_checkint(L, 3);

	if (i < 1 || i > a->rows)
		return luaL_error(L, "Row index out of bounds: %d (1 <= row <= %d)", i, a->rows);
	if (k < 1 || k > a->cols)
		return luaL_error(L, "Column index out of bounds: %d (1 <= col <= %d)", k, a->cols);

	--i;
	--k;
	lua_pushnumber(L, a->m[i * a->cols + k]);
	return 1;
}


// matrix methods
static int l_mat_clone(lua_State* L)
{
	mat44* m = (mat44*)luaL_checkudata(L, 1, MAT_INTERNAL_NAME);

	lua_pushcfunction(L, l_mat_new);
	FOR_MAT(i, k, m->rows, m->cols,
	        lua_pushnumber(L, m->m[i * m->cols + k]));
	lua_call(L, m->rows * m->cols, 1);
	return 1;
}

static int l_mat_map(lua_State* L)
{
	mat44* m = (mat44*)luaL_checkudata(L, 1, MAT_INTERNAL_NAME);
	if (!lua_isfunction(L, 2))
		return luaL_typerror(L, 2, "function");

	lua_pushcfunction(L, l_mat_new);
	FOR_MAT(i, k, m->rows, m->cols,
	        lua_pushvalue(L, 2);
	        lua_pushnumber(L, m->m[i * m->cols + k]);
	        lua_pushinteger(L, i);
	        lua_pushinteger(L, k);
	        lua_call(L, 3, 1);
	        m->m[i * m->cols + k] = lua_tonumber(L, -1));

	lua_settop(L, 1);
	return 1;
}

static int l_mat_unpack(lua_State* L)
{
	mat44* m = (mat44*)luaL_checkudata(L, 1, MAT_INTERNAL_NAME);
	FOR_MAT(i, k, m->rows, m->cols,
	        lua_pushnumber(L, m->m[i * m->cols + k]));
	return m->rows * m->cols;
}

static int l_mat_reset(lua_State* L)
{
	mat44* m = (mat44*)luaL_checkudata(L, 1, MAT_INTERNAL_NAME);
	GLfloat res[16] = {.0,.0,.0,.0, .0,.0,.0,.0, .0,.0,.0,.0, .0,.0,.0,.0};

	FOR_MAT(i, k, m->rows, m->cols,
	        res[i * m->cols + k] = luaL_checknumber(L, i * m->cols + k));
	FOR_MAT(i, k, m->rows, m->cols,
	        m->m[i * m->cols + k] = res[i * m->cols + k]);

	lua_settop(L, 1);
	return 1;
}

static int l_mat_get(lua_State* L)
{
	mat44* m = (mat44*)luaL_checkudata(L, 1, MAT_INTERNAL_NAME);
	int i = luaL_checkint(L, 2);
	int k = luaL_checkint(L, 3);

	if (i < 1 || i > m->rows)
		return luaL_error(L, "Row index out of bounds: %d (1 <= row <= %d)", i, m->rows);
	if (k < 1 || k > m->cols)
		return luaL_error(L, "Column index out of bounds: %d (1 <= col <= %d)", k, m->cols);

	--i;
	--k;

	lua_pushnumber(L, m->m[i * m->cols + k]);
	return 1;
}

static int l_mat_set(lua_State* L)
{
	mat44* m = (mat44*)luaL_checkudata(L, 1, MAT_INTERNAL_NAME);
	int i = luaL_checkint(L, 2);
	int k = luaL_checkint(L, 3);
	GLfloat val = luaL_checknumber(L, 4);

	if (i < 1 || i > m->rows)
		return luaL_error(L, "Row index out of bounds: %d (1 <= row <= %d)", i, m->rows);
	if (k < 1 || k > m->cols)
		return luaL_error(L, "Column index out of bounds: %d (1 <= col <= %d)", k, m->cols);

	--i;
	--k;
	m->m[i * m->cols + k] = val;
	return 0;
}

static int l_mat_permul(lua_State* L)
{
	return l_mat___concat(L);
}

static int l_mat_det(lua_State* L)
{
	mat44* m = (mat44*)luaL_checkudata(L, 1, MAT_INTERNAL_NAME);
	switch (m->cols)
	{
	case 2:
		lua_pushnumber(L, m->m[0] * m->m[3] - m->m[1] * m->m[2]);
		break;
	case 3:
		lua_pushnumber(L,
		               m->m[0] * (m->m[4] * m->m[8] - m->m[5] * m->m[7])
		               + m->m[1] * (m->m[5] * m->m[6] - m->m[8] * m->m[3])
		               + m->m[2] * (m->m[3] * m->m[7] - m->m[4] * m->m[6]));
		break;
	case 4:
		// hopefully there is no error here...
		lua_pushnumber(L,
		               m->m[0] * (  m->m[5] * (m->m[10] * m->m[15] - m->m[11] * m->m[14])
		                            - m->m[6] * (m->m[15] * m->m[9]  - m->m[11] * m->m[13])
		                            + m->m[7] * (m->m[14] * m->m[9]  - m->m[10] * m->m[13]))
		               - m->m[1] * (  m->m[4] * (m->m[10] * m->m[15] - m->m[11] * m->m[14])
		                              - m->m[6] * (m->m[15] * m->m[8]  - m->m[11] * m->m[12])
		                              + m->m[7] * (m->m[14] * m->m[8]  - m->m[10] * m->m[12]))
		               + m->m[2] * (  m->m[4] * (m->m[15] * m->m[9]  - m->m[11] * m->m[13])
		                              + m->m[7] * (m->m[13] * m->m[8]  - m->m[12] * m->m[9])
		                              - m->m[5] * (m->m[15] * m->m[8]  - m->m[11] * m->m[12]))
		               - m->m[3] * (  m->m[4] * (m->m[14] * m->m[9]  - m->m[10] * m->m[13])
		                              - m->m[5] * (m->m[14] * m->m[8]  - m->m[10] * m->m[12])
		                              + m->m[6] * (m->m[13] * m->m[8]  - m->m[12] * m->m[9])));
		break;
	}
	return 1;
}

static int l_mat_transposed(lua_State* L)
{
	mat44* m = (mat44*)luaL_checkudata(L, 1, MAT_INTERNAL_NAME);

	lua_pushcfunction(L, l_mat_new);
	FOR_MAT(i, k, m->rows, m->cols,
	        lua_pushnumber(L, m->m[k * m->cols + i]));
	lua_call(L, m->rows * m->cols, 1);
	return 1;
}

static int l_mat_transpose_inplace(lua_State* L)
{
	mat44* m = (mat44*)luaL_checkudata(L, 1, MAT_INTERNAL_NAME);
	GLfloat tmp[16] = {.0,.0,.0,.0, .0,.0,.0,.0, .0,.0,.0,.0, .0,.0,.0,.0};

	FOR_MAT(i, k, m->rows, m->cols,
	        tmp[k * m->cols + i] = m->m[i * m->cols + k]);
	FOR_MAT(i, k, m->rows, m->cols,
	        m->m[i * m->cols + k] = tmp[i * m->cols + k]);
	return 1;
}

static int l_mat_trace(lua_State* L)
{
	mat44* m = (mat44*)luaL_checkudata(L, 1, MAT_INTERNAL_NAME);

	lua_pushcfunction(L, l_vec_new);
	FOR_MAT_ITER(i, m->cols,
	             lua_pushnumber(L, m->m[i * m->cols + i]));
	lua_call(L, m->cols, 1);
	return 1;
}


// matrix constructor
static int l_mat_new(lua_State* L)
{
	int dim = lua_gettop(L);
	mat44* m = NULL;
	switch (dim)
	{
	case 2*2:
		m = (mat44*)lua_newuserdata(L, sizeof(mat22));
		m->cols = m->rows = 2;
		break;
	case 3*3:
		m = (mat44*)lua_newuserdata(L, sizeof(mat33));
		m->cols = m->rows = 3;
		break;
	case 4*4:
		m = (mat44*)lua_newuserdata(L, sizeof(mat44));
		m->cols = m->rows = 4;
		break;
	default:
		return luaL_error(L, "Invalid matrix dimension: %d",(int)sqrt((double)(dim)));
	}

	for (int i = 1; i <= dim; ++i)
		m->m[i-1] = luaL_checknumber(L, i);

	if (luaL_newmetatable(L, MAT_INTERNAL_NAME))
	{
		luaL_reg meta[] =
		{
			// matrix meta methods
			{"__index",           l_mat___index},
			{"__tostring",        l_mat___tostring},
			{"__unm",             l_mat___unm},
			{"__add",             l_mat___add},
			{"__sub",             l_mat___sub},
			{"__mul",             l_mat___mul},
			{"__div",             l_mat___div},
			{"__concat",          l_mat___concat},
			{"__len",             l_mat___len},
			{"__eq",              l_mat___eq},
			{"__call",            l_mat___call},

			// matrix methods
			{"clone",             l_mat_clone},
			{"map",               l_mat_map},
			{"unpack",            l_mat_unpack},
			{"reset",             l_mat_reset},
			{"get",               l_mat_get},
			{"set",               l_mat_set},
			{"permul",            l_mat_permul},
			{"det",               l_mat_det},
			{"transposed",        l_mat_transposed},
			{"transpose_inplace", l_mat_transpose_inplace},
			{"trace",             l_mat_trace},

			{NULL, NULL}
		};
		l_registerFunctions(L, -1, meta);
	}
	lua_setmetatable(L, -2);

	return 1;
}

////< MODULE >//////////////////////////////////////////////////////////////////////

#define __normalize_vec3(r, v) do { \
	GLfloat l = sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]); \
	r[0]=v[0]/l; r[1]=v[1]/l; r[2]=v[2]/l; } while(0)
#define __cross_vec3(r, a,b) do { \
	r[0] = a[1]*b[2] - a[2]*b[1]; \
	r[1] = a[2]*b[0] - a[0]*b[2]; \
	r[2] = a[0]*b[1] - a[1]*b[0]; } while(0)

#define MAKE_MATRIX(a,b,c,d, e,f,g,h, i,j,k,l, m,n,o,p) do {\
	lua_settop(L, 0);     \
	lua_pushnumber(L, a); \
	lua_pushnumber(L, b); \
	lua_pushnumber(L, c); \
	lua_pushnumber(L, d); \
                          \
	lua_pushnumber(L, e); \
	lua_pushnumber(L, f); \
	lua_pushnumber(L, g); \
	lua_pushnumber(L, h); \
                          \
	lua_pushnumber(L, i); \
	lua_pushnumber(L, j); \
	lua_pushnumber(L, k); \
	lua_pushnumber(L, l); \
	                      \
	lua_pushnumber(L, m); \
	lua_pushnumber(L, n); \
	lua_pushnumber(L, o); \
	lua_pushnumber(L, p); \
	                      \
	l_mat_new(L); } while(0)

// transformation matrices
static int l_unit(lua_State* L)
{
	GLfloat s = luaL_optnumber(L, 1, 1.);
	MAKE_MATRIX(
	    s, .0, .0, .0,
	    .0,  s, .0, .0,
	    .0, .0,  s, .0,
	    .0, .0, .0,  s);
	return 1;
}

static int l_rotate(lua_State* L)
{
	vec3* axis = l_checkvec3(L, 1);
	GLfloat phi = luaL_checknumber(L, 2);

	GLfloat s = sin(phi);
	GLfloat c = cos(phi);
	GLfloat ic = 1. - c;

	GLfloat a[3];
	__normalize_vec3(a, axis->v);

	GLfloat x = a[0];
	GLfloat y = a[1];
	GLfloat z = a[2];

	GLfloat xx = a[0] * a[0];
	GLfloat yy = a[1] * a[1];
	GLfloat zz = a[2] * a[2];

	GLfloat xy = a[0] * a[1];
	GLfloat xz = a[0] * a[2];
	GLfloat yz = a[1] * a[2];

	MAKE_MATRIX(
	    xx*ic +   c,  xy*ic - z*s,  xz*ic + y*s, .0,
	    xy*ic + z*s,  yy*ic +   c,  yz*ic - x*s, .0,
	    xz*ic - y*s,  yz*ic + x*s,  zz*ic +   c, .0,
	    .0,           .0,           .0, 1.);
	return 1;
}

static int l_scale(lua_State* L)
{
	GLfloat sx = luaL_checknumber(L, 1);
	GLfloat sy = sx;
	GLfloat sz = sx;
	if (lua_gettop(L) == 3)
	{
		sy = luaL_checknumber(L, 2);
		sz = luaL_checknumber(L, 3);
	}
	MAKE_MATRIX(
	    sx, .0, .0, .0,
	    .0, sy, .0, .0,
	    .0, .0, sz, .0,
	    .0, .0, .0, 1.);
	return 1;
}

static int l_translate(lua_State* L)
{
	vec4* v = (vec4*)luaL_checkudata(L, 1, VEC_INTERNAL_NAME);
	MAKE_MATRIX(
	    1., .0, .0, v->v[0],
	    .0, 1., .0, v->v[1],
	    .0, .0, .1, v->dim > 2 ? v->v[2] : .0,
	    .0, .0, .0, v->dim > 3 ? v->v[3] : 1.);
	return 1;
}

// view matrices
static int l_lookAt(lua_State* L)
{
	vec3* eye    = l_checkvec3(L, 1);
	vec3* center = l_checkvec3(L, 2);
	vec3* up     = l_checkvec3(L, 3);

	GLfloat s[3], f[3], u[3];

	__normalize_vec3(u, up->v);

	// forward = normalize(center - eye)
	f[0] = center->v[0] - eye->v[0];
	f[1] = center->v[1] - eye->v[1];
	f[2] = center->v[2] - eye->v[2];
	__normalize_vec3(f, f);

	// side = normalize(forward x up)
	__cross_vec3(s, f, u);
	__normalize_vec3(s, s);

	// up = side x forward
	__cross_vec3(u, s, f);

	GLfloat *e = eye->v;
	MAKE_MATRIX(
	    s[0],  s[1],  s[2], -e[0]*s[0] - e[1]*s[1] - e[2]*s[2],
	    u[0],  u[1],  u[2], -e[0]*u[0] - e[1]*u[1] - e[2]*u[2],
	    -f[0], -f[1], -f[2],  e[0]*f[0] + e[1]*f[1] + e[2]*f[2],
	    .0,    .0,    .0,                                 1.);
	return 1;
}

// projection matrices
static int l_ortho(lua_State* L)
{
	GLfloat l = luaL_checknumber(L, 1); // left
	GLfloat r = luaL_checknumber(L, 2); // right
	GLfloat b = luaL_checknumber(L, 3); // bottom
	GLfloat t = luaL_checknumber(L, 4); // top

	if (lua_gettop(L) == 4)
	{
		MAKE_MATRIX(
		    2./(r-l),       .0, .0, (l+r)/(l-r),
		    .0, 2./(t-b), .0, (b+t)/(b-t),
		    .0,       .0, .0,         -1.,
		    .0,       .0, .0,          1.);
		return 1;
	}

	GLfloat n = luaL_checknumber(L, 5); // near
	GLfloat f = luaL_checknumber(L, 6); // far
	MAKE_MATRIX(
	    2./(r-l),       .0,       .0, (l+r)/(l-r),
	    .0, 2./(t-b),       .0, (b+t)/(b-t),
	    .0,       .0, 2./(n-f), (n+f)/(n-f),
	    .0,       .0,       .0,          1.);
	return 1;
}

static int l_perspective(lua_State* L)
{
	GLfloat fovy   = luaL_checknumber(L, 1);
	GLfloat aspect = luaL_checknumber(L, 2);

	GLfloat f = 1. / tan(fovy * M_PI / 360.);

	if (lua_gettop(L) == 2)
	{
		MAKE_MATRIX(
		    f/aspect, .0,  .0, .0,
		    .0,  f,  .0, .0,
		    .0, .0, -1., .0,
		    .0, .0, -1., .0);
		return 1;
	}

	GLfloat near   = luaL_checknumber(L, 3);
	GLfloat far    = luaL_checknumber(L, 4);
	MAKE_MATRIX(
	    f/aspect, .0,                    .0,                     .0,
	    .0,  f,                    .0,                     .0,
	    .0, .0, (far+near)/(near-far), 2.*near*far/(near-far),
	    .0, .0,                   -1.,                     .0);
	return 1;
}

static int l_frustum(lua_State* L)
{
	GLfloat l = luaL_checknumber(L, 1); // left
	GLfloat r = luaL_checknumber(L, 2); // right
	GLfloat b = luaL_checknumber(L, 3); // bottom
	GLfloat t = luaL_checknumber(L, 4); // top
	GLfloat n = luaL_checknumber(L, 5); // near
	GLfloat f = luaL_checknumber(L, 6); // far

	MAKE_MATRIX(
	    2*n/(r-l),        .0, (r+l)/(r-l),  .0,
	    .0, 2*n/(t-b), (t+b)/(t-b),  .0,
	    .0,        .0, (n+f)/(n-f), 2*f*n/(n-f),
	    .0,        .0,         -1.,  .0);
	return 1;
}

int luaopen_G4L_math(lua_State* L)
{
	luaL_reg module[] =
	{
		{"vector",  l_vec_new},
		{"matrix",  l_mat_new},
		{NULL, NULL}
	};

	luaL_reg model[] =
	{
		{"unit",        l_unit},
		{"rotate",      l_rotate},
		{"scale",       l_scale},
		{"translate",   l_translate},
		{NULL, NULL}
	};

	luaL_reg view[] =
	{
		{"lookAt",      l_lookAt},
		{NULL, NULL}
	};

	luaL_reg project[] =
	{
		{"ortho",       l_ortho},
		{"perspective", l_perspective},
		{"frustum",     l_frustum},
		{NULL, NULL}
	};

	lua_newtable(L);
	l_registerFunctions(L, -1, module);

	lua_newtable(L);
	l_registerFunctions(L, -1, model);
	lua_setfield(L, -2, "model");

	lua_newtable(L);
	l_registerFunctions(L, -1, view);
	lua_setfield(L, -2, "view");

	lua_newtable(L);
	l_registerFunctions(L, -1, project);
	lua_setfield(L, -2, "project");

	return 1;
}

