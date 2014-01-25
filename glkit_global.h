#ifndef glkit_global_h
#define glkit_global_h

#include <SDL/SDL.h>

struct glkit_mouse
{
	int glk_mousex, glk_mousey;
	int glk_cmx, glk_cmy;
	bool glk_mouseleft, glk_mouseright, glk_mousemiddle;
	bool glk_mousedoubleclick;
	int glk_mousewheel;
};

void glkitShowMouse(bool on);

SDL_Surface *glkitGetScreen();

char glkitGetKey();

void glkitResetKey();

int glkitGetWidth();

int glkitGetHeight();

#endif
