#ifndef DPInput_h
#define DPInput_h

#define DIRECTINPUT_VERSION 0x0800

#include <windows.h>
#include <dinput.h>
//#include "F:/Progs/VCToolkit2003/DirectX SDK/include/dinput.h"

#define DPI_JRIGHT	-4
#define DPI_JLEFT	-3
#define DPI_JUP		-2
#define DPI_JDOWN	-1
/*
#define DPI_JRIGHT	256
#define DPI_JLEFT	257
#define DPI_JUP		258
#define DPI_JDOWN	259
#define DPI_JB0		260
#define DPI_JB1		261
#define DPI_JB2		262
#define DPI_JB3		263
#define DPI_JB4		264
#define DPI_JB5		265
#define DPI_JB6		266
#define DPI_JB7		267
#define DPI_JB8		268
#define DPI_JB9		269
#define DPI_JB10	270
#define DPI_JB11	271
#define DPI_JB12	272

#define DPI_JAX	0
#define DPI_JAY	1
#define DPI_JAZ	2
*/
BOOL CALLBACK EnumObjectsCallback(const DIDEVICEOBJECTINSTANCE* pdidoi, VOID* pContext);
BOOL CALLBACK EnumJoysticksCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* pContext);

struct DPJoystick
{
	float axis[8];
	GUID axistype[8];
	int num_axes;
	bool button[16];
	int num_buttons;
	float filter;
};

class DPInput
{
public:
	LPDIRECTINPUTDEVICE8	lpdiKeyboard;
	BYTE	diKeys[256];
	DPJoystick joysticks[32];
	int		num_joysticks;
	int		cur_joystick;
	int		lostjoystick[32];
//	bool	jbutton[10];
//	float	jaxis_x, jaxis_y, jaxis_z;
//	bool	analog;
//	bool	toggle_analog;

	LPDIRECTINPUTDEVICE8	lpdiJoystick[32];
	bool	poll_stop;
	bool	poll_done;
	bool	nojoystick;
	
	bool	enabled;
	
	DPInput(HWND hwnd, HINSTANCE hinst);
	~DPInput();

	void Reacquire(); // Call regularly in case the device is lost
	void Update();	
	void UpdateJoysticks();	
	bool KeyPressed(int key);
	bool IsAnalog();
	int NumJoyAxes();
	int NumJoyButtons();
	void SelectJoystick(int id);
	bool JoyButton(int index);
	float JoyAxis(int axis);
	
	void Disable();
	void Enable();
};
#endif

