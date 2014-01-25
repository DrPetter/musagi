//#define _WIN32_WINDOWS 0xBAD
//#define _WIN32_WINNT 0x0400

//#include <windows.h>
#include <SDL/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "Log.h"

//HWND		hWndMain=NULL;
//HINSTANCE	hInstanceMain;

#define DWORD unsigned long

#include "Texture.h"

#include "glkit_global.h"

//HDC		hDC=NULL;
//HGLRC	hRC=NULL;
//LRESULT	CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
SDL_Surface *screen;

bool	keys[256];
bool	active=true;

int glkit_width;
int glkit_height;
int glkit_refresh;
bool glkit_fullscreen;
bool glkit_fullwindow;
bool glkit_close=false;
bool glkit_tracking=false;
bool glkit_resized=false;

int glkit_winx;
int glkit_winy;
int glkit_winmax;
bool glkit_render=true;

glkit_mouse glkmouse;
char glkkey;

//HCURSOR cursor_arrow;

void glkPreInit();
void glkInit();
void glkFree();
void glkRenderFrame();
bool glkCalcFrame();

/*HWND glkitGetHwnd()
{
	return hWndMain;
}*/
SDL_Surface *glkitGetScreen() {
    return screen;
}

char glkitGetKey()
{
	return glkkey;
}

void glkitResetKey()
{
	glkkey=0;
}

int glkitGetWidth()
{
	return glkit_width;
}

int glkitGetHeight()
{
	return glkit_height;
}

void glkitShowMouse(bool on)
{
	if(on)
                SDL_ShowCursor(SDL_ENABLE);
		//ShowCursor(TRUE);
	else
                SDL_ShowCursor(SDL_DISABLE);
		//ShowCursor(FALSE);
}

void glkitInternalRender()
{
	glDisable(GL_SCISSOR_TEST);
	glClear(GL_COLOR_BUFFER_BIT);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);				
	glEnable(GL_BLEND);
	
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glkRenderFrame();
	
	glDisable(GL_BLEND);
	glFlush();

        SDL_GL_SwapBuffers();
}

GLvoid ReSizeGLScene(GLsizei width, GLsizei height)
{
	if(height==0) height=1;

	glkit_width=width;
	glkit_height=height;

	glViewport(0,0,width,height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0,width, height,0, 1, -1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

int InitGL(GLvoid)
{
	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);
	glClearColor(0.6f, 0.6f, 0.6f, 0);
	glClearDepth(1.0f);
	glDepthMask(0);
	glDisable(GL_DEPTH_TEST);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	glDisable(GL_CULL_FACE);

	return true;
}

GLvoid KillGLWindow(GLvoid)
{
	/*if(hRC)
	{
		LogPrint("delete OpenGL context");
		wglMakeCurrent(NULL,NULL);
		wglDeleteContext(hRC);
		hRC=NULL;
	}

	if(hDC)
	{
		LogPrint("release DC");
		ReleaseDC(hWndMain,hDC);
	}

	if(hWndMain)
	{
		LogPrint("destroy main window");
		DestroyWindow(hWndMain);
	}

	LogPrint("unregister main class");
	UnregisterClass("OpenGL",hInstanceMain);*/
        SDL_Quit();
}

bool CreateGLWindow(char* title, int width, int height, int bits, bool fullscreen, int refresh)
{
    Uint32 flags = 0;
    if (fullscreen)
        flags |= SDL_FULLSCREEN;
    flags |= SDL_OPENGL;
    flags |= SDL_GL_DOUBLEBUFFER;
    flags |= SDL_HWPALETTE;
    flags |= SDL_RESIZABLE;

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    screen = SDL_SetVideoMode(width, height, bits, flags);

    if (!screen)
        return false;

    SDL_WM_SetCaption(title, NULL); // title, icon

    InitGL();

    ReSizeGLScene(width,height);

    return true;
}

bool HandleEvent(SDL_Event event)
{
	switch (event.type)									// Check For Windows Messages
	{
	case SDL_ACTIVEEVENT:							// Watch For Window Activate Message
		if (event.active.gain)					// Check Minimization State
			active=true;						// Program Is Active
		else
			active=false;						// Program Is No Longer Active
		return 0;								// Return To The Message Loop

        // We could perhaps do something with SDL_SYSWMEVENT here, but it looks
        // like it might get hairy, so I'm ignoring it for now.
        /*
	case WM_SYSCOMMAND:							// Intercept System Commands
		switch (wParam)							// Check System Calls
		{
		case SC_SCREENSAVE:					// Screensaver Trying To Start?
		case SC_MONITORPOWER:				// Monitor Trying To Enter Powersave?
			return 0;							// Prevent From Happening
		}
		break;									// Exit
        */
	case SDL_QUIT:								// Did We Receive A Close Message?
		glkit_close=true;
		//PostQuitMessage(0);						// Send A Quit Message
		return 0;								// Jump Back

        // No idea what to do with this
	/*case WM_SETCURSOR:
		if(glkit_fullscreen || glkit_fullwindow)
		{
			SetCursor(NULL);
			return 0;
		}
		else
			SetCursor(cursor_arrow);
		break;
        */
	case SDL_VIDEORESIZE:								// Resize The Window
		glkit_resized=true;
		glkit_width=event.resize.w;
		glkit_height=event.resize.h;
		ReSizeGLScene(glkit_width, glkit_height);  // LoWord=Width, HiWord=Height
		return 0;								// Jump Back

        // Not sure if we can de anything with this
	/*case WM_MOVE:								// Move The Window
		glkit_winx=LOWORD(lParam)-GetSystemMetrics(SM_CXSIZEFRAME);
		glkit_winy=HIWORD(lParam)-GetSystemMetrics(SM_CYSIZEFRAME)-GetSystemMetrics(SM_CYCAPTION);
		return 0;								// Jump Back
        */
        // Not sure if we can do this in SDL either
        /*
	case WM_MOUSELEAVE: // mouse left the window - release all buttons
		glkmouse.glk_mouseleft=false;
		glkmouse.glk_mouseright=false;
		glkmouse.glk_mousemiddle=false;
		glkit_tracking=false;
		return 0;
        */
	case SDL_MOUSEMOTION:
                // What's this?
		/*if(!glkit_tracking)
		{
			glkit_tracking=true;
			TRACKMOUSEEVENT tme;
			tme.cbSize=sizeof(TRACKMOUSEEVENT);
			tme.dwFlags=TME_LEAVE;
			tme.hwndTrack=hWndMain;
			tme.dwHoverTime=HOVER_DEFAULT;
			TrackMouseEvent(&tme);
		}*/
		int curwidth, curheight;
                SDL_Rect rect;
		//GetClientRect(hWndMain, &rect);
		curwidth=screen->w;//rect.right-rect.left;
		curheight=screen->h;//rect.bottom-rect.top;
		glkmouse.glk_mousex=(int)((float)event.motion.x/curwidth*glkit_width);
		glkmouse.glk_mousey=(int)((float)event.motion.y/curheight*glkit_height);
		return 0;
        // Moving this code to case SDL_MOUSEBUTTONDOWN
	/*case WM_MOUSEWHEEL:
		if((wParam>>16)&0x7FFF)
		{
		}
		return 0;*/
	/*case WM_LBUTTONDBLCLK:
		glkmouse.glk_mousedoubleclick=true;
		return 0;*/
	case SDL_MOUSEBUTTONDOWN://WM_LBUTTONDOWN:
            switch(event.button.button) {
            case SDL_BUTTON_LEFT:
		glkmouse.glk_mouseleft=true;
		//GetClientRect(hWndMain, &rect);
		curwidth=screen->w;//rect.right-rect.left;
		curheight=screen->h;//rect.bottom-rect.top;
		glkmouse.glk_cmx=(int)((float)event.button.x/curwidth*glkit_width);
		glkmouse.glk_cmy=(int)((float)event.button.y/curheight*glkit_height);
		return 0;
            case SDL_BUTTON_RIGHT:
		glkmouse.glk_mouseright=true;
		curwidth=screen->w;
		curheight=screen->h;
		glkmouse.glk_cmx=(int)((float)event.button.x/curwidth*glkit_width);
		glkmouse.glk_cmy=(int)((float)event.button.y/curheight*glkit_height);
		return 0;
            case SDL_BUTTON_MIDDLE:
		glkmouse.glk_mousemiddle=true;
		curwidth=screen->w;
		curheight=screen->h;
		glkmouse.glk_cmx=(int)((float)event.button.x/curwidth*glkit_width);
		glkmouse.glk_cmy=(int)((float)event.button.y/curheight*glkit_height);
		return 0;
            case SDL_BUTTON_WHEELUP:
                glkmouse.glk_mousewheel=-1;
                return 0;
            case SDL_BUTTON_WHEELDOWN:
                glkmouse.glk_mousewheel=1;
                return 0;
            }
        case SDL_MOUSEBUTTONUP:
            switch(event.button.button) {
            case SDL_BUTTON_LEFT:
		glkmouse.glk_mouseleft=false;
		return 0;
            case SDL_BUTTON_RIGHT:
		glkmouse.glk_mouseright=false;
		return 0;
            case SDL_BUTTON_MIDDLE:
		glkmouse.glk_mousemiddle=false;
		return 0;
            }
	case SDL_KEYDOWN:
                if ((event.key.keysym.unicode & 0xFF80) == 0) {
                    glkkey=event.key.keysym.unicode & 0x7F;
                }
                else {
                    glkkey=0; // this is kinda not right, but what can I do?
                }
		return 0;
	}

	// Pass All Unhandled Messages To DefWindowProc
	return 1; // DefWindowProc(hWnd,uMsg,wParam,lParam);
}

int main(int argc, char **argv)
{
        SDL_Event event;

	//cursor_arrow=LoadCursor(NULL, IDC_ARROW);
	
	glkkey=0;

	glkit_width=950;
	glkit_height=650;
	glkit_winx=0;
	glkit_winy=0;
	glkit_winmax=1;
	glkit_refresh=75;
	glkit_fullscreen=false;
	glkit_fullwindow=false;
	
	glkPreInit();

	if(glkit_winmax)
	{
		glkit_width=950;
		glkit_height=650;
		glkit_winx=0;
		glkit_winy=0;
	}

        SDL_Init(SDL_INIT_VIDEO);

	if(!CreateGLWindow("musagi 0.1", glkit_width, glkit_height, 32, glkit_fullscreen, glkit_refresh))
		return 0;

	Uint32 startTime;
	Uint32 frequency;
	bool glkit_timeravailable;

	/*if(!QueryPerformanceFrequency(&frequency))
		glkit_timeravailable=false;
	else
	{
		glkit_timeravailable=true;
		QueryPerformanceCounter(&startTime);
	}*/
        glkit_timeravailable=true;
        frequency = 1000;
        startTime = SDL_GetTicks();

	glkInit();

	bool done=false;
	while(!done)
	{
		SDL_Delay(5);//Sleep(5);

		glkit_resized=false;
		glkmouse.glk_mousewheel=0;
                while (SDL_PollEvent(&event)) {
                    HandleEvent(event);
                }

		if(!glkCalcFrame())
		{
		    done=true;
		    break;
		}

		if(glkit_render)
			glkitInternalRender();
	}

	//glkit_winmax=IsZoomed(hWndMain);
	
	glkFree();

	KillGLWindow();

	LogPrint("end of WinMain() -> exit program");

	return 0; // (msg.wParam);
}

