
#ifndef DATA_H
#define DATA_H

#include <libs/molson.h>
#include <cglm/cglm.h>

enum  object_type
{
	quad_object
};

typedef struct
{
	unsigned int m_indices;
	mat4 m_transform;
	int m_initialized;
	
	texture *m_texture;
	enum object_type m_type;
	
	int z_index;
	vec2 position;
	vec3 rotation;
	vec3 color;
	vec2 scale;
	
	const char *name;
	
	void (*set_initialized)(int new_initialized_,struct object*object_);
	void (*set_transform)(mat4 new_transform_,struct object*object_);
	void (*set_indices)(unsigned int new_indices_,struct object*object_);
	void (*set_texture)(texture new_texture_,struct object*object_);
	void (*set_type)(enum object_type new_type_,struct object*object_);
	
	void (*get_initialized)(int*initialized_,struct object*object_);
	void (*get_transform)(mat4*transform_,struct object*object_);
	void (*get_texture)(texture*texture_,struct object*object_);
	void (*get_indices)(unsigned int*indices_,struct object*object_);
	void (*get_type)(enum object_type*type_,struct object*object_);
		
} object;

enum window_mode
{
	fullscreen_mode,
	windowed_mode
};

#endif//DATA_H
