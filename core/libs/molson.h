
#ifndef MOLSON_H
#define MOLSON_H
#define molson(x) molson##x

#include <stdbool.h>

// -- Texture and Shader types.
typedef struct {
    unsigned int internal_format;
    unsigned int image_format;
    unsigned int filter_min;
    unsigned int filter_max;
    unsigned int wrap_s;
    unsigned int wrap_t;
    unsigned int height;
    unsigned int width;
    unsigned int id;
    
    const char *path;
} texture;

typedef struct {
    unsigned int fragment_shader;
    unsigned int vertex_shader;
    unsigned int ID;
} shader;

const char *molson(file_to_char)(const char *file_path);

texture molson(load_texture)(const char *file_path, bool alpha);

int molson(init_shader)(const char *vertex_path, const char *fragment_path, shader *shader);
void molson(destroy_shader)(shader *shader);
void molson(use_shader)(shader *shader);

int molson(set_vector2_f)(const char *name, float value[2], bool use_shader, shader *shader);
int molson(set_vector3_f)(const char *name, float value[3], bool use_shader, shader *shader);
int molson(set_vector4_f)(const char *name, float value[4], bool use_shader, shader *shader);
int molson(set_matrix4)(const char *name, void *value, bool use_shader, shader *shader);

int molson(set_float)(const char *name, float value, bool use_shader, shader *shader);
int molson(set_int)(const char *name, int value, bool use_shader, shader *shader);
int molson(set_bool)(const char *name, bool value, shader *shader);

// --------------------------------------------------
// -               IMPLEMENTATION                   -
// --------------------------------------------------

#endif//MOLSON_H
#ifdef MOLSON_IMPLEMENTATION

#define GLAD 0
#define GLUT 1

#define GLLOADER GLAD
#define VERBOSE true

#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

// -- Dependencies:
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#if GLLOADER == GLAD
#include <glad.h>
#elif GLLOADER == GLUT
// TODO: Add glut support(although it's deprecated and old and you were not supposed to be using it.);
#endif
// --

// -- Function destinated to load text files. Sometimes it's useful.
const char *molson(file_to_char)(const char *file_path) {
    long buffer_size = 0;
    char *buffer;
    FILE *file;
    
    file = fopen(file_path, "r");
    if (file == NULL) {
		fprintf(stderr, "[MOLSON]: FAILED :: File could not be loaded. \n");
		fclose(file);
		return NULL;
    }
    if (fseek(file, 0, SEEK_END) < 0) goto BLANK;
    buffer_size = ftell(file);
    buffer = (char *)malloc(buffer_size + 1);
    if (buffer == NULL) {
		fprintf(stderr, "[MOLSON]: FAILED :: Buffer could not be read. \n");
		return NULL;
    }
    
    if (fseek(file, 0, SEEK_SET) < 0) goto BLANK;
    fread(buffer, 1, buffer_size, file);
    if (ferror(file)) {
		fprintf(stderr, "[MOLSON]: FAILED :: File could not be read. \n");
		return NULL;
    }
    fclose(file);
    
#if VERBOSE == true
    printf("[MOLSON]: File have been loaded sucessfully. \n");
#endif
    return buffer;
    
BLANK:
    fprintf(stderr, "[INFO]: File is blank. \n");
    fclose(file);
    
    buffer = (char *)malloc(sizeof(char));
    free(buffer);
    return NULL;
}
// --

// -- Texture loading and generating.
static void init_texture(texture *texture) {
    texture->height = 0;
    texture->width = 0;
    texture->image_format = GL_RGB;
    texture->wrap_s = GL_REPEAT;
    texture->wrap_t = GL_REPEAT;
    texture->filter_min = GL_LINEAR;
    texture->filter_max = GL_LINEAR;
    
    glGenTextures(1, &texture->id);
    return;
}

static void generate_texture(unsigned int width, unsigned int height, unsigned char *data, texture *texture) {
    texture->height = height;
    texture->width = width;
    
    glBindTexture(GL_TEXTURE_2D, texture->id);
    glTexImage2D(GL_TEXTURE_2D, 0, texture->internal_format, width, height, 0, texture->image_format, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texture->wrap_s);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texture->wrap_t);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texture->filter_min);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texture->filter_max);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    return;
}

texture molson(load_texture)(const char *file_path, bool alpha) {
    texture new_texture;
    init_texture(&new_texture);
    // Setting alpha format if alpha parameters is true
    if (alpha) {
		new_texture.internal_format = GL_RGBA;
		new_texture.image_format    = GL_RGBA;
    }
    int height, width, channels;
    
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(file_path, &width, &height, &channels, 0);
    if (data == NULL) {
		printf("[MOLSON]: FAILED :: Failed to load texture. \n");
		return new_texture;
    }
    generate_texture(width, height, data, &new_texture);
    stbi_image_free(data);
    
    new_texture.path = file_path;
#if VERBOSE == true
    printf("[MOLSON]: Texture had been loaded successfully. \n");
#endif
    return new_texture;
}
// --

/*
 -- OpenGL Shader loader.
    INFO: yes, here I write a similar code found in 'file_to_char' function, within this same file. I do that for some reasons:
    - Vertex and fragment shader error handling;
	- Vertex and fragment shader text formatting.
	
    Again: that is what works for me and this is, for sure, not the better approach. Any changes suggestions would be welcome.
*/
int molson(init_shader)(const char *vertex_path, const char *fragment_path, shader *shader) {
    long buffer_length = 0;
    
    char *fragment_buffer;
    char *vertex_buffer;
    FILE *fragment_code;
    FILE *vertex_code;
    
    fragment_code = fopen(fragment_path, "r");
    if (fragment_code == NULL) {
		fprintf(stderr, "[MOLSON]: FAILED :: Fragment shader file could not be loaded. \n");
		fclose(fragment_code);
		shader->ID = -1;
		return 1;
    }
    if (fseek(fragment_code, 0, SEEK_END) < 0) goto BLANK;
    
    buffer_length = ftell(fragment_code);
    fragment_buffer = (char *)malloc(buffer_length + 1);
    if (fragment_buffer == NULL) {
		fprintf(stderr, "[MOLSON]: FAILED :: Fragment shader variable could not be allocated. \n");
		fclose(fragment_code);
		shader->ID = -1;
		return 1;
    }
    if (fseek(fragment_code, 0, SEEK_SET) < 0) goto BLANK;
    fread(fragment_buffer, 1, buffer_length, fragment_code);
    if (ferror(fragment_code)) {
		fprintf(stderr, "[MOLSON]: FAILED :: Fragment shader file could not be read. \n");
		fclose(fragment_code);
		shader->ID = -1;
		return 1;
    }
    fragment_buffer[buffer_length] = '\0';
    
    vertex_code = fopen(vertex_path, "r");
    if (vertex_code == NULL) {
		fprintf(stderr, "[MOLSON]: FAILED :: Vertex shader file could not be loaded. \n");
		fclose(vertex_code);
		shader->ID = -1;
		return 1;
    }
    if (fseek(vertex_code, 0, SEEK_END) < 0) goto BLANK;
    
    buffer_length = ftell(vertex_code);
    vertex_buffer = (char *)malloc(buffer_length + 1);
    if (vertex_buffer == NULL) {
		fprintf(stderr, "[MOLSON]: FAILED :: Vertex shader variable could not be allocated. \n");
		fclose(vertex_code);
		shader->ID = -1;
		return 1;
    }
    
    if (fseek(vertex_code, 0, SEEK_SET < 0)) goto BLANK;
    fread(vertex_buffer, 1, buffer_length, vertex_code);
    if (ferror(vertex_code)) {
		fprintf(stderr, "[FAILED]: Fragment shader file could not be read. \n");
		fclose(vertex_code);
		shader->ID = -1;
		return 1;
    }
    vertex_buffer[buffer_length] = '\0';
    
    if (fragment_code) fclose(fragment_code);
    if (vertex_code)   fclose(vertex_code);
    
    char info_log[512];
    int  success;
    
    shader->vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(shader->vertex_shader, 1, (GLchar const * const *)&vertex_buffer, NULL);
    glCompileShader(shader->vertex_shader);
    glGetShaderiv(shader->vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
		glGetShaderInfoLog(shader->vertex_shader, 512, NULL, info_log);
		fprintf(stderr, "[MOLSON]: FAILED :: Vertex shader compilation failed. \n");
		fprintf(stderr, "[INFO_LOG]: %s. \n", info_log);
		return 1;
    }
#if VERBOSE == true
    printf("[MOLSON]: Vertex shader compiled successfully. \n");
#endif
    
    shader->fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(shader->fragment_shader, 1, (GLchar const * const *)&fragment_buffer, NULL);
    glCompileShader(shader->fragment_shader);
    glGetShaderiv(shader->fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
		glGetShaderInfoLog(shader->fragment_shader, 512, NULL, info_log);
		fprintf(stderr, "[MOLSON]: FAILED :: Fragment shader compilation failed. \n");
		fprintf(stderr, "[INFO_LOG]: %s. \n", info_log);
		return 1;
    }
#if VERBOSE == true
    printf("[MOLSON]: Fragment shader compiled successfully. \n");
#endif
    
    shader->ID = glCreateProgram();
    glAttachShader(shader->ID, shader->fragment_shader);
    glAttachShader(shader->ID, shader->vertex_shader);
    glLinkProgram(shader->ID);
    glGetProgramiv(shader->ID, GL_LINK_STATUS, &success);
    if (!success) {
		glGetProgramInfoLog(shader->ID, 512, NULL, info_log);
		fprintf(stderr, "[MOLSON]: FAILED :: Shader program linking failed. \n");
		fprintf(stderr, "[INFO_LOG]: %s. \n", info_log);
		return 1;
    }
#if VERBOSE == true
    printf("[MOLSON]: Shader program had been linked successfully. \n");
#endif
    return 0;
    
BLANK:
#if VERBOSE == true
    printf("[MOLSON]: Shader file is blank. \n");
#endif
    if (fragment_code)  fclose(fragment_code);
    else if (vertex_code) fclose(vertex_code);
    shader->ID = -1;
    return 0;
}

void molson(destroy_shader)(shader *shader) {
    glDeleteShader(shader->fragment_shader);
    glDeleteShader(shader->vertex_shader);
    glDeleteProgram(shader->ID);
#if VERBOSE == true
    printf("[MOLSON]: Shaders deleted successfully. \n");
#endif
    return;
}
// --

// -- OpenGL shader use and uniforms setter.
void molson(use_shader)(shader *shader) {
    glUseProgram(shader->ID);
    return;
}

// -- TODO: find a workaround so generics are not used here.
int molson(set_matrix4)(const char *name, void *value, bool use_shader, shader *shader) {
    if (value == NULL) return 1;
    if (use_shader) {
	glUseProgram(shader->ID);
    }
    glUniformMatrix4fv(glGetUniformLocation(shader->ID, name), 1, false, (float *)value);
    return 0;
}
// --

int molson(set_vector2_f)(const char *name, float value[2], bool use_shader, shader *shader) {
    if (value == NULL) return 1;
    if (use_shader) {
	glUseProgram(shader->ID);
    }
    glUniform2fv(glGetUniformLocation(shader->ID, name), 1, value);
    return 0;
}
int molson(set_vector3_f)(const char *name, float value[3], bool use_shader, shader *shader) {
    if (value == NULL) return 1;
    if (use_shader) {
	glUseProgram(shader->ID);
    }
    glUniform3fv(glGetUniformLocation(shader->ID, name), 1, value);
    return 0;
}
int molson(set_vector4_f)(const char *name, float value[4], bool use_shader, shader *shader) {
    if (value == NULL) return 1;
    if (use_shader) {
	glUseProgram(shader->ID);
    }
    glUniform4fv(glGetUniformLocation(shader->ID, name), 1, value);
    return 0;
}

int molson(set_float)(const char *name, float value, bool use_shader, shader *shader) {
    if (value == NULL) return -1;
    if (use_shader) {
	glUseProgram(shader->ID);
    }
    glUniform1f(glGetUniformLocation(shader->ID, name), value);
    return 0;
}
int molson(set_int)(const char *name, int value, bool use_shader, shader *shader) {
    if (value == NULL) return -1;
    if (use_shader) {
	glUseProgram(shader->ID);
    }
    glUniform1i(glGetUniformLocation(shader->ID, name), value);
    return 0;
}
int molson(set_bool)(const char *name, bool value, shader *shader) {
    if (value == NULL) return -1;
    glUniform1i(glGetUniformLocation(shader->ID, name), value);
    return 0;
}
// --

#endif




