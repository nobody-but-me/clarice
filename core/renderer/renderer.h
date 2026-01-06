
#ifndef RENDERER_H
#define RENDERER_H

#include <libs/molson.h>
#include <common/data.h>

#define RECT_VERTEX_DATA {				\
		0.0f, 1.0f, 1.0f, 0.0f, 1.0f,	\
		1.0f, 0.0f, 1.0f, 1.0f, 0.0f,	\
		0.0f, 0.0f, 1.0f, 0.0f, 0.0f,	\
										\
		0.0f, 1.0f, 1.0f, 0.0f, 1.0f,	\
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f,	\
		1.0f, 0.0f, 1.0f, 1.0f, 0.0f,	\
	}
#define renderer(x) renderer_##x

void renderer(get_quad_vao)(unsigned int*vao_);
void renderer(get_quad_vbo)(unsigned int*vbo_);

int renderer(init_globals)(void);

int renderer(init_rect)(object*object_,texture*texture_,const char*name_);

int renderer(set_object_transform)(object*object_);
int renderer(render_object)(object*object_);

void renderer(init)(void);

#endif//RENDERER_H
