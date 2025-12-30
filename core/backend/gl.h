

#ifndef GL_H
#define GL_H

#include <GLFW/glfw3.h>
#include <common/data.h>

#define gl(x) gl_##x

#define DEFAULT_WINDOW_WIDTH 800
#define DEFAULT_WINDOW_HEIGHT DEFAULT_WINDOW_WIDTH/4*3
#define DEFAULT_WINDOW_TITLE "prototype"

void gl(set_window_mode)(const enum window_mode window_mode_);

unsigned int gl(get_current_window_height)(void);
unsigned int gl(get_current_window_width)(void);
GLFWwindow*gl(get_current_window)(void);

int gl(is_window_minimized)(void);
int gl(is_window_focused)(void);
int gl(is_window_open)(void);

void gl(toggle_window_fullscreen)(void);
void gl(force_window_close)(void);

int gl(init)(const enum window_mode window_mode_);
void gl(destroy)(void);

void gl(begin_frame)(void);
void gl(end_frame)(void);

#endif//GL_H
