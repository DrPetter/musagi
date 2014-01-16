#include "DPInput.h"
#include "Log.h"

LPDIRECTINPUT8			lpdi;
DPInput *ginput;
DPJoystick *current_joystick;
int current_joystick_index;

BOOL CALLBACK EnumObjectsCallback(const DIDEVICEOBJECTINSTANCE* pdidoi, VOID* pContext)
{
	HWND hDlg=(HWND)pContext;
	
	// For axes that are returned, set the DIPROP_RANGE property for the
	// enumerated axis in order to scale min/max values.
	if(pdidoi->dwType&DIDFT_AXIS)
	{
		DIPROPRANGE diprg; 
		diprg.diph.dwSize       = sizeof(DIPROPRANGE); 
		diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER); 
		diprg.diph.dwHow        = DIPH_BYID; 
		diprg.diph.dwObj        = pdidoi->dwType; // Specify the enumerated axis
		diprg.lMin              = -1000; 
		diprg.lMax              = +1000; 
		
		current_joystick->axistype[current_joystick->num_axes]=pdidoi->guidType;
		current_joystick->num_axes++;
		
		// Set the range for the axis
		if(FAILED(ginput->lpdiJoystick[current_joystick_index]->SetProperty(DIPROP_RANGE, &diprg.diph)))
			return DIENUM_STOP;
	}
	return DIENUM_CONTINUE;
};

BOOL CALLBACK EnumJoysticksCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* pContext)
{
	HRESULT hr;

	LogPrint("Joystick callback");

	hr=lpdi->CreateDevice(pdidInstance->guidInstance, &ginput->lpdiJoystick[ginput->num_joysticks], NULL);
	if(FAILED(hr)) 
		return DIENUM_STOP;
//		return DIENUM_CONTINUE;

	ginput->nojoystick=false;
	ginput->num_joysticks++;

	LogPrint("Added joystick %i", ginput->num_joysticks-1);

	return DIENUM_CONTINUE;
//	return DIENUM_STOP;
};

DWORD WINAPI PollThread(LPVOID arg)
{
	LogPrint("Starting poll thread");
	while(!ginput->poll_stop)
	{
		Sleep(10);
		if(!ginput->poll_done)
		{
			for(int i=0;i<ginput->num_joysticks;i++)
			{
//				LogPrint("Polling joystick %i", i);
				ginput->lpdiJoystick[i]->Poll();
			}
			ginput->poll_done=true;
		}
	}
	ginput->poll_done=true;
	return 1;
}

DPInput::DPInput(HWND hwnd, HINSTANCE hinst)
{
	LogPrint("Init DPInput");
	ginput=this;

	enabled=true;
	
	DirectInput8Create(hinst, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&lpdi, NULL);
	// Init keyboard
	lpdi->CreateDevice(GUID_SysKeyboard, &lpdiKeyboard, NULL);
	lpdiKeyboard->SetCooperativeLevel(hwnd, DISCL_NONEXCLUSIVE|DISCL_FOREGROUND);
	lpdiKeyboard->SetDataFormat(&c_dfDIKeyboard);
	// Init joystick
	LogPrint("Enumerating joysticks...");
	num_joysticks=0;
	nojoystick=true;
	lpdi->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumJoysticksCallback, NULL, DIEDFL_ATTACHEDONLY);
	if(num_joysticks>32) // I don't want dynamic allocation, but I also don't want to crash...
		num_joysticks=32;
	for(int i=0;i<num_joysticks;i++)
	{
		DPJoystick *cjoy=&joysticks[i];
		cjoy->num_axes=0;
		cjoy->num_buttons=4; // TEMP
//		LogPrint("-------- %i buttons", cjoy->num_buttons);
		HRESULT hr;
		if(FAILED(hr=lpdiJoystick[i]->SetDataFormat(&c_dfDIJoystick2)))
			LogPrint("Error - failed to set data format");
		if(FAILED(hr=lpdiJoystick[i]->SetCooperativeLevel(hwnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND)))
			LogPrint("Error - failed to set coop level");
	//		g_diDevCaps.dwSize=sizeof(DIDEVCAPS);
	//		if(FAILED(hr=lpdiJoystick->GetCapabilities(&g_diDevCaps))) MessageBox(NULL, "Fail 3", "Error", 0);
		current_joystick=cjoy;
		current_joystick_index=i;
//		LogPrint("-------- %i buttons", current_joystick->num_buttons);
		if(FAILED(hr=lpdiJoystick[i]->EnumObjects(EnumObjectsCallback, (VOID*)hwnd, DIDFT_AXIS)))
			LogPrint("Error - failed to enumerate axes");
	//					     Joystick->EnumObjects(EnumObjectsCallback, (VOID*)hwnd, DIDFT_ALL)))
	//		if(FAILED(hr=g_pJoystick->SetDataFormat(&c_dfDIJoystick2)))
	//			return hr;
		lpdiJoystick[i]->Acquire();
		LogPrint("Joystick %i initialized - %i axes", i, cjoy->num_axes);
	}
	LogPrint("A total of %i joysticks were found", num_joysticks);
//	LogPrint("-------- %i buttons", current_joystick->num_buttons);
	lpdiKeyboard->Acquire();

	poll_stop=false;
	poll_done=false;
	
	for(int i=0;i<num_joysticks;i++)
	{
		lostjoystick[i]=0;
		for(int j=0;j<8;j++)
			joysticks[i].axis[j]=0.0f;
		for(int j=0;j<16;j++)
			joysticks[i].button[j]=false;
		joysticks[i].filter=1.0f;
//		LogPrint("Joystick %i (%.8X) has %i axes", i, &joysticks[i], joysticks[i].num_axes);
	}
	cur_joystick=0;

//	LogPrint("-------- %i buttons", current_joystick->num_buttons);
	
//	joystick_present=false;
	
	DWORD threadid;
	if(!nojoystick)
		CreateThread(0, 0, PollThread, NULL, 0, &threadid);

//	for(int i=0;i<num_joysticks;i++)
//		LogPrint("Joystick %i (%.8X) has %i axes", i, &joysticks[i], joysticks[i].num_axes);
};

DPInput::~DPInput()
{
	LogPrint("Destroy DPInput");
	for(int i=0;i<num_joysticks;i++)
	{
		poll_stop=true;
		while(!poll_done) // Wait for poll thread to close
		{
			Sleep(5);
		}
		lpdiJoystick[i]->Unacquire();
		lpdiJoystick[i]->Release();
	}
	lpdiKeyboard->Unacquire();
	lpdiKeyboard->Release();
	lpdi->Release();
};

void DPInput::Reacquire() // Call regularly in case the device is lost
{
	lpdiKeyboard->Acquire();
	for(int i=0;i<num_joysticks;i++)
		lpdiJoystick[i]->Acquire();
};

void DPInput::Update()
{
	enabled=true;

	if(lpdiKeyboard->GetDeviceState(256, (LPVOID)&diKeys)!=DI_OK)
	{
		lpdiKeyboard->Acquire();
		// Mark all keys as released if state could not be retrieved
		for(int i=0;i<256;i++)
			diKeys[i]=0x00;
	}

//	if(nojoystick)
//		return;

	UpdateJoysticks();
};

void DPInput::UpdateJoysticks()
{
	if(!poll_done)
		return;
	poll_done=false;

//	LogPrint("Updating joystick data");

//	for(int i=0;i<num_joysticks;i++)
//		LogPrint("Joystick %i (%.8X) has %i axes", i, &joysticks[i], joysticks[i].num_axes);

	for(int i=0;i<num_joysticks;i++)
	{
		DIJOYSTATE2 js;
		if(lpdiJoystick[i]->GetDeviceState(sizeof(DIJOYSTATE2), &js)!=DI_OK)
		{
			lostjoystick[i]++;
			if(lostjoystick[i]>4)
			{
//				LogPrint("GetDeviceState for joystick %i failed, reaquiring", i);
				lpdiJoystick[i]->Acquire();
				for(int j=0;j<16;j++)
					joysticks[i].button[j]=false; // Mark all joystick buttons as released if state could not be retrieved
			}
		}
		else
		{
			lostjoystick[i]=0;
//			LogPrint("Updating data for joystick %i", i);
			DPJoystick *cjoy=&joysticks[i];
			// D-pad (derive from analog axes 0 and 1)
			cjoy->button[DPI_JRIGHT+4]=(js.lX>500);
			cjoy->button[DPI_JLEFT+4]=(js.lX<-500);
			cjoy->button[DPI_JDOWN+4]=(js.lY>500);
			cjoy->button[DPI_JUP+4]=(js.lY<-500);
			// Filtered axes
//			LogPrint("Joystick %i (%.8X) has %i axes", i, cjoy, cjoy->num_axes);
			for(int j=0;j<cjoy->num_axes;j++)
			{
//				LogPrint("Joystick %i, axis %i", i, j);
//				LogPrint("Joystick %i, axis %i = %.4f", i, j, cjoy->axis[j]);
				GUID type=cjoy->axistype[j];
				if(type==GUID_XAxis)
					cjoy->axis[j]+=((float)js.lX/1000-cjoy->axis[j])*cjoy->filter;
				if(type==GUID_YAxis)
					cjoy->axis[j]+=((float)js.lY/1000-cjoy->axis[j])*cjoy->filter;
				if(type==GUID_ZAxis)
					cjoy->axis[j]+=((float)js.lZ/1000-cjoy->axis[j])*cjoy->filter;
				if(type==GUID_RxAxis)
					cjoy->axis[j]+=((float)js.lRx/1000-cjoy->axis[j])*cjoy->filter;
				if(type==GUID_RyAxis)
					cjoy->axis[j]+=((float)js.lRy/1000-cjoy->axis[j])*cjoy->filter;
				if(type==GUID_RzAxis)
					cjoy->axis[j]+=((float)js.lRz/1000-cjoy->axis[j])*cjoy->filter;
			}
//			jaxis_y+=((float)js.lY/1000-jaxis_y)*joystick_filter;
//			jaxis_z+=((float)js.lZ/1000-jaxis_z)*joystick_filter;
//			for(int j=0;j<4;j++)
//				cjoy->button[j]=false; // D-pad not pressed

			for(int j=0;j<cjoy->num_buttons;j++)
//			for(int j=0;j<10;j++)
				cjoy->button[j+4]=((js.rgbButtons[j]&0x80)==0x80);

/*			jbutton[DPI_JB1-256]=((js.rgbButtons[1]&0x80)==0x80);
			jbutton[DPI_JB2-256]=((js.rgbButtons[2]&0x80)==0x80);
			jbutton[DPI_JB3-256]=((js.rgbButtons[3]&0x80)==0x80);
			jbutton[DPI_JB4-256]=((js.rgbButtons[4]&0x80)==0x80);
			jbutton[DPI_JB5-256]=((js.rgbButtons[5]&0x80)==0x80);*/
		}
	}
};

bool DPInput::KeyPressed(int key)
{
	if(!enabled)
		return false;
/*	if(key>=256) // Joystick button
	{
		if(nojoystick)
			return false;
		return joysticks[cur_joystick].button[key-256];
	}*/
	if(diKeys[key]&0x80)
		return true;
	return false;
};

bool DPInput::JoyButton(int index)
{
	if(!enabled)
		return false;
//	LogPrint("dinput: checking button %i (numbuttons=%i)", index, joysticks[cur_joystick].num_buttons);
	index+=4; // Translate to actual button mapping (to allow for d-pad buttons)

//	if(nojoystick || index<0 || index>=joysticks[cur_joystick].num_buttons+4)
//		return false;

/*	if(joysticks[cur_joystick].button[index])
		LogPrint("dinput: joybutton %i=true", index);
	else
		LogPrint("dinput: joybutton %i=false", index);*/
	return joysticks[cur_joystick].button[index];
};
/*
bool DPInput::IsAnalog()
{
	return analog;
};
*/
int DPInput::NumJoyAxes()
{
	if(nojoystick)
		return 0;
	return joysticks[cur_joystick].num_axes;
};

int DPInput::NumJoyButtons()
{
	if(nojoystick)
		return 0;
	return joysticks[cur_joystick].num_buttons;
};

void DPInput::SelectJoystick(int id)
{
	if(id<0) id=0;
	if(id>=num_joysticks) id=num_joysticks-1;
	cur_joystick=id;
};

float DPInput::JoyAxis(int axis)
{
/*	if(axis==DPI_JAX)
		return jaxis_x;
	if(axis==DPI_JAY)
		return jaxis_y;
	if(axis==DPI_JAZ)
		return jaxis_z;*/
	if(nojoystick || axis<0 || axis>=joysticks[cur_joystick].num_axes)
		return 0.0f;
	return joysticks[cur_joystick].axis[axis];
};

void DPInput::Disable()
{
	enabled=false;
};

void DPInput::Enable()
{
	enabled=true;
};


