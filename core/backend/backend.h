
#ifndef BACKEND_H
#define BACKEND_H

#include <common/data.h>

#define backend(x) backend_##x

void backend(force_window_close)(void);
void backend(destroy_application)(void);
int backend(is_window_open)(void);

int backend(init)(const enum window_mode window_mode_);
int backend(destroy)(void);
void backend(loop)(void);

void backend(render)(void);
void backend(loop)(void);

#endif//BACKEND_H
