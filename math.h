#ifndef OPENGLUA_MATH_H
#define OPENGLUA_MATH_H

#include <GL/glew.h>
#include <lua.h>

#define vec(n) \
	struct vec##n { \
		int dim; \
		GLfloat v[n]; \
	}

#define mat(n,k) \
	struct mat##n##k { \
		int rows; \
		int cols; \
		GLfloat m[n * k]; \
	}

typedef vec(2) vec2;
typedef vec(3) vec3;
typedef vec(4) vec4;
typedef mat(2,2) mat22;
typedef mat(3,3) mat33;
typedef mat(4,4) mat44;

int l_isvec(int n, lua_State* L, int idx);
void* l_checkvec(int n, lua_State* L, int idx);
#define l_isvec2(L, idx) (vec2*)(l_isvec(2, L, idx))
#define l_isvec3(L, idx) (vec3*)(l_isvec(3, L, idx))
#define l_isvec4(L, idx) (vec4*)(l_isvec(4, L, idx))
#define l_checkvec2(L, idx) (vec2*)(l_checkvec(2, L, idx))
#define l_checkvec3(L, idx) (vec3*)(l_checkvec(3, L, idx))
#define l_checkvec4(L, idx) (vec4*)(l_checkvec(4, L, idx))

int l_ismat(int n, int k, lua_State* L, int idx);
void* l_checkmat(int n, int k, lua_State* L, int idx);
#define l_ismat22(L, idx) (mat22*)(l_ismat(2,2, L, idx))
#define l_ismat33(L, idx) (mat33*)(l_ismat(3,3, L, idx))
#define l_ismat44(L, idx) (mat44*)(l_ismat(4,4, L, idx))
#define l_checkmat22(L, idx) (mat22*)(l_checkmat(2,2, L, idx))
#define l_checkmat33(L, idx) (mat33*)(l_checkmat(3,3, L, idx))
#define l_checkmat44(L, idx) (mat44*)(l_checkmat(4,4, L, idx))

int luaopen_OpenGLua_math(lua_State* L);

#endif
