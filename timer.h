/*
  This timer class was originally programmed by
  Richard Mosquera (burnseh),
  richardmosquera@hotmail.com

  But it has been modified a bunch by DrPetter.
*/

#ifndef __TIMER_H
#define __TIMER_H

//#include <windows.h>
#include <SDL/SDL.h>

// Not sure if SDL_GetTicks is quite accurate enough,
// but it's better than nothing.

class CTimer
{
private:
	float mark[20];

	public:
		CTimer() {};
		~CTimer() {};

		bool init()
		{
                        frequency = 1000; // millisecond accuracy
			// retreive frequency of the timer in ticks per second
			/*if (!QueryPerformanceFrequency(&frequency))
			{
				return false;	// hi-res timer not supported
			}
			else
			{
				// if the hi-res timer is supported store the 
				// current value of the timer in startTime
				QueryPerformanceCounter(&startTime);
				return true;
			}*/
                        startTime = SDL_GetTicks();

			Reset();
		}

		// returns number of seconds since it was last called 
		float GetElapsedSeconds(int index)
		{
			Uint32 currentTime;

			// get the current value of the timer 
			//QueryPerformanceCounter(&currentTime);
                        currentTime = SDL_GetTicks();

			// (current time - last time) gives the number of ticks that have ellapsed
			// divide that by the timer's frequence and we get time in milliseconds 
			float seconds = ((float)currentTime - (float)startTime) / (float)frequency;

			return seconds-mark[index];
		}

		void SetMark(int index)
		{
			Uint32 currentTime;

			//QueryPerformanceCounter(&currentTime);
                        currentTime = SDL_GetTicks();
			float seconds = ((float)currentTime - (float)startTime) / (float)frequency;

			mark[index]=seconds;
		}

		void Reset()
		{
			int i;

			//QueryPerformanceCounter(&startTime);
                        startTime = SDL_GetTicks();

			for(i=0;i<20;i++)
				mark[i]=0;
		}

		float GetFPS(unsigned long elapsedFrames = 1)
		{
			static Uint32 lastTime = startTime;
			Uint32 currentTime;
                        currentTime = SDL_GetTicks();
			//QueryPerformanceCounter(&currentTime);

			float fps = (float)elapsedFrames * (float)frequency / ((float)currentTime - (float)lastTime);

			// reset the timer
			lastTime = currentTime;

			return fps;
		}
	
	private:
		Uint32 startTime;
		Uint32 frequency;
};

#endif // __TIMER_H
