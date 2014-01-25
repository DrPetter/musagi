#ifndef DPInput_h
#define DPInput_h

#include <SDL/SDL.h>

class DPInput
{
private:
        Uint8 *keys;
        int numkeys;
        SDL_Joystick *joystick;
public:
	DPInput();//SDL_Screen *screen);
	~DPInput();

	void Reacquire(); // Call regularly in case the device is lost
	void Update();	
	bool KeyPressed(int key);
        float JoyAxis(int axis);
};
#endif

