/*
  This timer class was originally programmed by
  Richard Mosquera (burnseh),
  richardmosquera@hotmail.com

  But it has been modified a bunch by DrPetter.
*/

#ifndef __TIMER_H
#define __TIMER_H

#include <windows.h>


class CTimer
{
private:
	float mark[20];

	public:
		CTimer() {};
		~CTimer() {};

		bool init()
		{
			// retreive frequency of the timer in ticks per second
			if (!QueryPerformanceFrequency(&frequency))
			{
				return false;	// hi-res timer not supported
			}
			else
			{
				// if the hi-res timer is supported store the 
				// current value of the timer in startTime
				QueryPerformanceCounter(&startTime);
				return true;
			}

			Reset();
		}

		// returns number of seconds since it was last called 
		float GetElapsedSeconds(int index)
		{
			LARGE_INTEGER currentTime;

			// get the current value of the timer 
			QueryPerformanceCounter(&currentTime);

			// (current time - last time) gives the number of ticks that have ellapsed
			// divide that by the timer's frequence and we get time in milliseconds 
			float seconds = ((float)currentTime.QuadPart - (float)startTime.QuadPart) / (float)frequency.QuadPart;

			return seconds-mark[index];
		}

		void SetMark(int index)
		{
			LARGE_INTEGER currentTime;

			QueryPerformanceCounter(&currentTime);
			float seconds = ((float)currentTime.QuadPart - (float)startTime.QuadPart) / (float)frequency.QuadPart;

			mark[index]=seconds;
		}

		void Reset()
		{
			int i;

			QueryPerformanceCounter(&startTime);

			for(i=0;i<20;i++)
				mark[i]=0;
		}

		float GetFPS(unsigned long elapsedFrames = 1)
		{
			static LARGE_INTEGER lastTime = startTime;
			LARGE_INTEGER currentTime;

			QueryPerformanceCounter(&currentTime);

			float fps = (float)elapsedFrames * (float)frequency.QuadPart / ((float)currentTime.QuadPart - (float)lastTime.QuadPart);

			// reset the timer
			lastTime = currentTime;

			return fps;
		}
	
	private:
		LARGE_INTEGER startTime;
		LARGE_INTEGER frequency;
};

#endif // __TIMER_H
