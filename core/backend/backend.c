
#include <stdlib.h>
#include <stdio.h>

#include <backend/backend.h>
#include <common/data.h>
#include <backend/gl.h>

void backend(force_window)(void)
{
	gl(force_window_close)();
	return;
}
int backend(destroy)(void)
{
	gl(destroy)();
	return 0;
}

int backend(is_window_open)(void)
{
	return gl(is_window_open)();
}

int backend(init)(const enum window_mode window_mode_)
{
	if ((gl(init)(window_mode_))==-1)
		return -1;
	printf("backend.c::init() : backend initialized successfully.\n");
	return 0;
}

void backend(loop)(void)
{
	gl(begin_frame)();
	gl(end_frame)();
	return;
}

void backend(render)(void)
{
	return;
}
