#define _WIN32_WINDOWS 0xBAD
#define _WIN32_WINNT 0x0400

#include <windows.h>
#include <gl\gl.h>
#include <gl\glu.h>
#include <gl\glext.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "Log.h"

HWND		hWndMain=NULL;
HINSTANCE	hInstanceMain;

#define DWORD unsigned long

#include "Texture.h"

#include "glkit_global.h"
/*
// vsync stuff
typedef BOOL	(APIENTRY * PFNWGLSWAPINTERVALEXT)(int interval);
typedef int		(APIENTRY * PFNWGLGETSWAPINTERVALEXT)();
PFNWGLSWAPINTERVALEXT		wglSwapIntervalEXT = NULL;
PFNWGLGETSWAPINTERVALEXT	wglGetSwapIntervalEXT = NULL;
#define LOAD_EXTENSION(name) *((void**)&name) = wglGetProcAddress(#name)
bool	vsync_control_enabled;
*/

HDC		hDC=NULL;
HGLRC	hRC=NULL;
LRESULT	CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

bool	keys[256];
bool	active=true;
//bool    fullscreen;

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

bool glkit_halfres=false;

bool glkitHalfRes()
{
	return glkit_halfres;
}

void glkitSetHalfRes(bool value)
{
	glkit_halfres=value;
}

glkit_mouse glkmouse;
char glkkey;

HCURSOR cursor_arrow;

int paint_requests; // to detect if the main loop is running/rendering or if we need to handle WM_PAINT

void glkPreInit();
void glkInit(char* cmd);
void glkFree();
void glkRenderFrame(bool disabled);
bool glkCalcFrame();

HWND glkitGetHwnd()
{
	return hWndMain;
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
	if(glkit_halfres)
		return glkit_width/2;
	else
		return glkit_width;
}

int glkitGetHeight()
{
	if(glkit_halfres)
		return glkit_height/2;
	else
		return glkit_height;
}

void glkitShowMouse(bool on)
{
	if(on)
		ShowCursor(TRUE);
	else
		ShowCursor(FALSE);
}

void glkitInternalRender(bool disabled) // disabled==true means no input should be handled
{
	glDisable(GL_SCISSOR_TEST);
	glClear(GL_COLOR_BUFFER_BIT);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glkRenderFrame(disabled);

	if(abort_render)
	{
		LogPrint("abort_render in glkit");
		abort_render=false;
		return;
	}

	glDisable(GL_BLEND);
	glFlush();

	SwapBuffers(hDC);
}

void ReSizeGLScene(GLsizei width, GLsizei height)
{
	if(height==0) height=1;

	glkit_width=width;
	glkit_height=height;

	glViewport(0,0,width,height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if(glkit_halfres)
		glOrtho(0,width/2, height/2,0, 1, -1);
	else
		glOrtho(0,width, height,0, 1, -1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

int InitGL(void)
{
	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);
	glClearColor(0.5f, 0.5f, 0.5f, 0);
	glClearDepth(1.0f);
	glDepthMask(0);
	glDisable(GL_DEPTH_TEST);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	glDisable(GL_CULL_FACE);

/*	LOAD_EXTENSION(wglSwapIntervalEXT);
	LOAD_EXTENSION(wglGetSwapIntervalEXT);
	if( wglSwapIntervalEXT==NULL ||
		wglGetSwapIntervalEXT==NULL)
			vsync_control_enabled=false;
	else
		vsync_control_enabled=true;
	if(vsync_control_enabled)
		wglSwapIntervalEXT(1);
*/
	return TRUE;
}

void KillGLWindow(void)
{
/*	if(glkit_fullscreen)
	{
		ChangeDisplaySettings(NULL,0);
		ShowCursor(TRUE);
	}
*/
	if(hRC)
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
	UnregisterClass("OpenGL",hInstanceMain);
}

BOOL CreateGLWindow(char* title, int width, int height, int bits, bool fullscreen, int refresh)
{
	LogPrint("CreateGLWindow: start");

	GLuint		PixelFormat;			// Holds The Results After Searching For A Match
	WNDCLASS	wc;						// Windows Class Structure
	DWORD		dwExStyle;				// Window Extended Style
	DWORD		dwStyle;				// Window Style
	RECT		WindowRect;				// Grabs Rectangle Upper Left / Lower Right Values
	WindowRect.left=(long)glkit_winx;	// Left Value
	WindowRect.right=(long)glkit_winx+(long)width;		// Set Right Value To Requested Width
	WindowRect.top=(long)glkit_winy;		// Top Value
	WindowRect.bottom=(long)glkit_winy+(long)height;		// Set Bottom Value To Requested Height

//	fullscreen=fullscreenflag;			// Set The Global Fullscreen Flag

	hInstanceMain		= GetModuleHandle(NULL);				// Grab An Instance For Our Window
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;	// Redraw On Size, And Own DC For Window.
	wc.lpfnWndProc		= (WNDPROC) WndProc;					// WndProc Handles Messages
	wc.cbClsExtra		= 0;									// No Extra Window Data
	wc.cbWndExtra		= 0;									// No Extra Window Data
	wc.hInstance		= hInstanceMain;						// Set The Instance
	wc.hIcon			= LoadIcon(NULL, IDI_WINLOGO);			// Load The Default Icon
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);			// Load The Arrow Pointer
	wc.hbrBackground	= NULL;									// No Background Required For GL
	wc.lpszMenuName		= NULL;									// We Don't Want A Menu
	wc.lpszClassName	= "OpenGL";								// Set The Class Name

	LogPrint("CreateGLWindow: register class");

	RegisterClass(&wc);

/*	if(fullscreen)												// Attempt Fullscreen Mode?
	{
		DEVMODE dmScreenSettings;								// Device Mode
		memset(&dmScreenSettings,0,sizeof(dmScreenSettings));	// Makes Sure Memory's Cleared
		dmScreenSettings.dmSize=sizeof(dmScreenSettings);		// Size Of The Devmode Structure
		dmScreenSettings.dmDisplayFrequency	= refresh;
		dmScreenSettings.dmPelsWidth	= width;				// Selected Screen Width
		dmScreenSettings.dmPelsHeight	= height;				// Selected Screen Height
		dmScreenSettings.dmBitsPerPel	= bits;					// Selected Bits Per Pixel
		dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT|DM_DISPLAYFREQUENCY;

		// Try To Set Selected Mode And Get Results.  NOTE: CDS_FULLSCREEN Gets Rid Of Start Bar.
		if(ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
			fullscreen=FALSE;		// Windowed Mode Selected.  Fullscreen = FALSE
	}

	if(fullscreen)												// Are We Still In Fullscreen Mode?
	{
		dwExStyle=WS_EX_APPWINDOW;								// Window Extended Style
		dwStyle=WS_POPUP;										// Windows Style
		ShowCursor(FALSE);										// Hide Mouse Pointer
	}
	else
	{*/
	dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;			// Window Extended Style
	dwStyle=WS_OVERLAPPEDWINDOW;
//		ShowCursor(FALSE);										// Show Mouse Pointer
//	}

	LogPrint("CreateGLWindow: adjustwindowrect");

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);		// Adjust Window To True Requested Size

	LogPrint("CreateGLWindow: create window");

	// Create The Window
	if(!(hWndMain=CreateWindowEx(	dwExStyle,							// Extended Style For The Window
								"OpenGL",							// Class Name
								title,								// Window Title
								dwStyle |							// Defined Window Style
								WS_CLIPSIBLINGS |					// Required Window Style
								WS_CLIPCHILDREN,					// Required Window Style
								glkit_winx, glkit_winy,				// Window Position
								WindowRect.right-WindowRect.left,	// Calculate Window Width
								WindowRect.bottom-WindowRect.top,	// Calculate Window Height
								NULL,								// No Parent Window
								NULL,								// No Menu
								hInstanceMain,							// Instance
								NULL)))								// Dont Pass Anything To WM_CREATE
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Could not create window","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	static	PIXELFORMATDESCRIPTOR pfd=				// pfd Tells Windows How We Want Things To Be
	{
		sizeof(PIXELFORMATDESCRIPTOR),				// Size Of This Pixel Format Descriptor
		1,											// Version Number
		PFD_DRAW_TO_WINDOW |						// Format Must Support Window
		PFD_SUPPORT_OPENGL |						// Format Must Support OpenGL
		PFD_DOUBLEBUFFER,							// Must Support Double Buffering
		PFD_TYPE_RGBA,								// Request An RGBA Format
		bits,										// Select Our Color Depth
		0, 0, 0, 0, 0, 0,							// Color Bits Ignored
		0,											// No Alpha Buffer
		0,											// Shift Bit Ignored
		0,											// No Accumulation Buffer
		0, 0, 0, 0,									// Accumulation Bits Ignored
		16,											// 16Bit Z-Buffer (Depth Buffer)
		0,											// No Stencil Buffer
		0,											// No Auxiliary Buffer
		PFD_MAIN_PLANE,								// Main Drawing Layer
		0,											// Reserved
		0, 0, 0										// Layer Masks Ignored
	};

	LogPrint("CreateGLWindow: dc stuff");

	hDC=GetDC(hWndMain);
	if(hDC==NULL)
		LogPrint("#### CreateGLWindow: failed to get DC");
	PixelFormat=ChoosePixelFormat(hDC, &pfd);
	if(PixelFormat==0)
		LogPrint("#### CreateGLWindow: failed to choose pixel format");
	if(SetPixelFormat(hDC, PixelFormat, &pfd)==FALSE)
		LogPrint("#### CreateGLWindow: failed to set pixel format");
	hRC=wglCreateContext(hDC);
	wglMakeCurrent(hDC, hRC);
	ShowWindow(hWndMain, SW_SHOW);						// Show The Window
	SetForegroundWindow(hWndMain);						// Slightly Higher Priority
	SetFocus(hWndMain);									// Sets Keyboard Focus To The Window
	ReSizeGLScene(width, height);					// Set Up Our Perspective GL Screen

	if(glkit_winmax)
		ShowWindow(hWndMain, SW_MAXIMIZE);

	LogPrint("CreateGLWindow: init gl");
	if(!InitGL())									// Initialize Our Newly Created GL Window
	{
		LogPrint("CreateGLWindow: gl failed");
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"OpenGL initialization failed","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	LogPrint("CreateGLWindow: done");

	return TRUE;									// Success
}

LRESULT CALLBACK WndProc(	HWND	hWnd,			// Handle For This Window
							UINT	uMsg,			// Message For This Window
							WPARAM	wParam,			// Additional Message Information
							LPARAM	lParam)			// Additional Message Information
{
	RECT rect;

	switch (uMsg)									// Check For Windows Messages
	{
	case WM_ACTIVATE:							// Watch For Window Activate Message
		if (!HIWORD(wParam))					// Check Minimization State
			active=TRUE;						// Program Is Active
		else
			active=FALSE;						// Program Is No Longer Active
		return 0;								// Return To The Message Loop

	case WM_SYSCOMMAND:							// Intercept System Commands
		switch (wParam)							// Check System Calls
		{
		case SC_SCREENSAVE:					// Screensaver Trying To Start?
		case SC_MONITORPOWER:				// Monitor Trying To Enter Powersave?
			return 0;							// Prevent From Happening
		}
		break;									// Exit

	case WM_CLOSE:								// Did We Receive A Close Message?
		glkit_close=true;
		PostQuitMessage(0);						// Send A Quit Message
		return 0;								// Jump Back

	case WM_PAINT:
		paint_requests++;
		if(paint_requests>2)
		{
//			LogPrint("WM_PAINT %i", paint_requests);
			glkitInternalRender(true); // trigger redraws when there's a file requester open holding up the message queue... uhm
		}
		break; // default handling

	case WM_TIMER:
		GetClientRect(hWndMain, &rect);
		InvalidateRect(hWnd, &rect, FALSE);
//		Repaint();
		return 0;

	case WM_SETCURSOR:
		if(glkit_fullscreen || glkit_fullwindow)
		{
			SetCursor(NULL);
			return 0;
		}
		else
			SetCursor(cursor_arrow);
		break;

	case WM_SIZE:								// Resize The Window
		glkit_resized=true;
		glkit_width=LOWORD(lParam);
		glkit_height=HIWORD(lParam);
		ReSizeGLScene(glkit_width, glkit_height);  // LoWord=Width, HiWord=Height
		return 0;								// Jump Back

	case WM_MOVE:								// Move The Window
		glkit_winx=LOWORD(lParam)-GetSystemMetrics(SM_CXSIZEFRAME);
		glkit_winy=HIWORD(lParam)-GetSystemMetrics(SM_CYSIZEFRAME)-GetSystemMetrics(SM_CYCAPTION);
		return 0;								// Jump Back

	case WM_MOUSELEAVE: // mouse left the window - release all buttons
		glkmouse.glk_mouseleft=false;
		glkmouse.glk_mouseright=false;
		glkmouse.glk_mousemiddle=false;
		glkit_tracking=false;
		return 0;
	case WM_MOUSEMOVE:
		if(!glkit_tracking)
		{
			glkit_tracking=true;
			TRACKMOUSEEVENT tme;
			tme.cbSize=sizeof(TRACKMOUSEEVENT);
			tme.dwFlags=TME_LEAVE;
			tme.hwndTrack=hWndMain;
			tme.dwHoverTime=HOVER_DEFAULT;
			TrackMouseEvent(&tme);
		}
		int curwidth, curheight;
		GetClientRect(hWndMain, &rect);
		curwidth=rect.right-rect.left;
		curheight=rect.bottom-rect.top;
		glkmouse.glk_mousex=(int)((float)LOWORD(lParam)/curwidth*glkit_width);
		glkmouse.glk_mousey=(int)((float)HIWORD(lParam)/curheight*glkit_height);
		if(glkit_halfres)
		{
			glkmouse.glk_mousex/=2;
			glkmouse.glk_mousey/=2;
		}
		return 0;
	case WM_MOUSEWHEEL:
		if((wParam>>16)&0x7FFF)
		{
			if((wParam>>16)&0x8000)
				glkmouse.glk_mousewheel=-1;
			else
				glkmouse.glk_mousewheel=1;
		}
		return 0;
	case WM_LBUTTONDBLCLK:
		glkmouse.glk_mousedoubleclick=true;
		return 0;
	case WM_LBUTTONDOWN:
		glkmouse.glk_mouseleft=true;
		glkmouse.glk_mouseleftclick=5;
		GetClientRect(hWndMain, &rect);
		curwidth=rect.right-rect.left;
		curheight=rect.bottom-rect.top;
		glkmouse.glk_cmx=(int)((float)LOWORD(lParam)/curwidth*glkit_width);
		glkmouse.glk_cmy=(int)((float)HIWORD(lParam)/curheight*glkit_height);
//		mouse_leftclick=true;
		if(glkit_halfres)
		{
			glkmouse.glk_cmx/=2;
			glkmouse.glk_cmy/=2;
		}
		return 0;
	case WM_LBUTTONUP:
		glkmouse.glk_mouseleft=false;
		return 0;
	case WM_RBUTTONDOWN:
		glkmouse.glk_mouseright=true;
		glkmouse.glk_mouserightclick=5;
		GetClientRect(hWndMain, &rect);
		curwidth=rect.right-rect.left;
		curheight=rect.bottom-rect.top;
		glkmouse.glk_cmx=(int)((float)LOWORD(lParam)/curwidth*glkit_width);
		glkmouse.glk_cmy=(int)((float)HIWORD(lParam)/curheight*glkit_height);
//		mouse_rightclick=true;
		if(glkit_halfres)
		{
			glkmouse.glk_cmx/=2;
			glkmouse.glk_cmy/=2;
		}
		return 0;
	case WM_RBUTTONUP:
		glkmouse.glk_mouseright=false;
		return 0;
	case WM_MBUTTONDOWN:
		glkmouse.glk_mousemiddle=true;
		glkmouse.glk_mousemiddleclick=5;
		GetClientRect(hWndMain, &rect);
		curwidth=rect.right-rect.left;
		curheight=rect.bottom-rect.top;
		glkmouse.glk_cmx=(int)((float)LOWORD(lParam)/curwidth*glkit_width);
		glkmouse.glk_cmy=(int)((float)HIWORD(lParam)/curheight*glkit_height);
//		mouse_middleclick=true;
		if(glkit_halfres)
		{
			glkmouse.glk_cmx/=2;
			glkmouse.glk_cmy/=2;
		}
		return 0;
	case WM_MBUTTONUP:
		glkmouse.glk_mousemiddle=false;
		return 0;

	case WM_CHAR:
		glkkey=(char)wParam;
		return 0;
	}

	// Pass All Unhandled Messages To DefWindowProc
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}

int WINAPI WinMain(	HINSTANCE	hInstance,
					HINSTANCE	hPrevInstance,
					LPSTR		lpCmdLine,
					int			nCmdShow)
{
	MSG msg;

	cursor_arrow=LoadCursor(NULL, IDC_ARROW);

	glkkey=0;

	glkit_width=950;
	glkit_height=650;
	glkit_winx=0;
	glkit_winy=0;
	glkit_winmax=1;
	glkit_refresh=75;
	glkit_fullscreen=false;
	glkit_fullwindow=false;

	glkmouse.glk_mousex=0;
	glkmouse.glk_mousey=0;
	glkmouse.glk_cmx=0;
	glkmouse.glk_cmy=0;
	glkmouse.glk_mouseleft=false;
	glkmouse.glk_mouseright=false;
	glkmouse.glk_mousemiddle=false;
	glkmouse.glk_mouseleftclick=false;
	glkmouse.glk_mouserightclick=false;
	glkmouse.glk_mousemiddleclick=false;
	glkmouse.glk_mousedoubleclick=false;
	glkmouse.glk_mousewheel=0;

	glkPreInit(); // log output is started here (if activated in config.txt)

	if(glkit_winmax)
	{
		glkit_width=950;
		glkit_height=650;
		glkit_winx=0;
		glkit_winy=0;
	}

	LogPrint("CreateGLWindow()");

	if(!CreateGLWindow("musagi 0.23", glkit_width, glkit_height, 32, glkit_fullscreen, glkit_refresh))
		return 0;

	LARGE_INTEGER startTime;
	LARGE_INTEGER frequency;
	bool glkit_timeravailable;

	LogPrint("init performance counter");

	if(!QueryPerformanceFrequency(&frequency))
		glkit_timeravailable=false;
	else
	{
		glkit_timeravailable=true;
		QueryPerformanceCounter(&startTime);
	}

	LogPrint("glkInit()");

	glkInit(lpCmdLine);

	LogPrint("init midi timer");

	UINT_PTR timerid=9347; // some fun id
	timerid=SetTimer(hWndMain, timerid, 50, NULL);

	LogPrint("enter message loop ------------ USAGE START");

	bool done=false;
	while(!done)
	{
/*		mouse_leftclick=false;
		mouse_rightclick=false;
		mouse_middleclick=false;
		mouse_doubleclick=false;

		mouse_px=mouse_x;
		mouse_py=mouse_y;*/

		Sleep(5);

		glkit_resized=false;
		glkmouse.glk_mousewheel=0;
		glkmouse.glk_mousedoubleclick=false;
		while(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			TranslateMessage(&msg);
			if(!GetMessage(&msg, NULL, 0, 0))
				break;
			DispatchMessage(&msg);
		}


		if(glkmouse.glk_mouseleft && glkmouse.glk_mouseleftclick==5) glkmouse.glk_mouseleftclick=0;
		if(glkmouse.glk_mouseright && glkmouse.glk_mouserightclick==5) glkmouse.glk_mouserightclick=0;
		if(glkmouse.glk_mousemiddle && glkmouse.glk_mousemiddleclick==5) glkmouse.glk_mousemiddleclick=0;
		if(glkmouse.glk_mouseleftclick>0) glkmouse.glk_mouseleft=true;
		if(glkmouse.glk_mouserightclick>0) glkmouse.glk_mouseright=true;
		if(glkmouse.glk_mousemiddleclick>0) glkmouse.glk_mousemiddle=true;

//		mouse_dx=mouse_x-mouse_px;
//		mouse_dy=mouse_y-mouse_py;

		if(!glkCalcFrame())
		{
		    done=true;
		    break;
		}

//		if(!glkit_fullscreen)
//		{
			// Limit framerate using whatever timing is available
/*			if(glkit_timeravailable)
			{
				float elapsed=0.0f;
				while(elapsed<0.98f/glkit_refresh) // 0.98 to avoid missing vsync due to being anal about exact framerate
				{
					LARGE_INTEGER currentTime;
					QueryPerformanceCounter(&currentTime);
					elapsed=((float)currentTime.QuadPart-(float)startTime.QuadPart)/(float)frequency.QuadPart;
				}
				QueryPerformanceCounter(&startTime);
			}
			else
				Sleep(1000/glkit_refresh); // Useless crappy timing*/
//		}

		if(glkit_render)
		{
			glkitInternalRender(false);
			paint_requests=0;
		}

		if(glkmouse.glk_mouseleftclick==1) glkmouse.glk_mouseleft=false;
		if(glkmouse.glk_mouserightclick==1) glkmouse.glk_mouseright=false;
		if(glkmouse.glk_mousemiddleclick==1) glkmouse.glk_mousemiddle=false;
		if(glkmouse.glk_mouseleftclick>0) glkmouse.glk_mouseleftclick--;
		if(glkmouse.glk_mouserightclick>0) glkmouse.glk_mouserightclick--;
		if(glkmouse.glk_mousemiddleclick>0) glkmouse.glk_mousemiddleclick--;
	}

	LogPrint("leave message loop ------------ USAGE END");

	KillTimer(hWndMain, timerid);

	glkit_winmax=IsZoomed(hWndMain);

	glkFree();

	KillGLWindow();

	LogPrint("end of WinMain() -> exit program");

	return (msg.wParam);
}

