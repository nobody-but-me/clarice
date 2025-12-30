
#include <stdio.h>
//#include <common/data.h>
#include <backend/backend.h>

int main(int argc,char**argv){
	if((backend(init)(windowed_mode))==-1)
		return -1;
	while(backend(is_window_open)())
		backend(loop)();
	backend(destroy)();
	return 0;
}
