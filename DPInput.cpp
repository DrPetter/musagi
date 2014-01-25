#include "DPInput.h"
#include "Log.h"
#include <SDL/SDL.h>

DPInput *ginput;

DPInput::DPInput()
{
	ginput=this;
        joystick = NULL;
        if (SDL_NumJoysticks() > 0)
            joystick = SDL_JoystickOpen(0);
};

DPInput::~DPInput()
{
};

void DPInput::Reacquire() // Call regularly in case the device is lost
{
};

void DPInput::Update()
{
        keys = SDL_GetKeyState(&numkeys);
};

bool DPInput::KeyPressed(int key)
{
//	if(diKeys[key]&0x80)
//		return true;
        //if (diKeys[key])
        if (keys[key])
            return true;
	return false;
};

float DPInput::JoyAxis(int axis)
{
    Sint16 intvalue;
    if (joystick) {
        intvalue = SDL_JoystickGetAxis(joystick, axis);
        return ((float)intvalue + 0.5) / 32767.5f;
    }
    return 0.0f;
};
