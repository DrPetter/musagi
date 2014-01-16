#ifndef glkit_global_h
#define glkit_global_h

struct glkit_mouse
{
	int glk_mousex, glk_mousey;
//	int glk_mousep1x, glk_mousep1y;
//	int glk_mousep2x, glk_mousep2y;
	int glk_cmx, glk_cmy;
	bool glk_mouseleft, glk_mouseright, glk_mousemiddle;
	int glk_mouseleftclick, glk_mouserightclick, glk_mousemiddleclick;
	bool glk_mousedoubleclick;
	int glk_mousewheel;
};

bool glkitHalfRes();

void glkitSetHalfRes(bool value);

void glkitShowMouse(bool on);

HWND glkitGetHwnd();

char glkitGetKey();

void glkitResetKey();

int glkitGetWidth();

int glkitGetHeight();

#endif
