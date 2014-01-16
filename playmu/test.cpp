#include <windows.h> // to get Sleep() for this example
#include <stdio.h>

#include "playmu/playmu.h"

void main()
{
	playmu_sound testeffect; // handle for sound effect

	printf("playmu test\n");
	
	SetEnvironmentVariable("PA_MIN_LATENCY_MSEC", "75"); // portaudio checks this to determine latency settings, leave it out if you're not on windows or if you want to ensure non-crackly sound even on poor sound cards
	
	playmu_init();

	playmu_mastervolume(100); // percent of maximum (well, above 100 is possible but not recommended)

	bool result=true; // error checking is possible, yay (but I can't be bothered in this demo)
	result=playmu_loadsound(testeffect, "column.wav", 1); // 1 = how many simultaneous playback channels are allowed for this sound
	result=playmu_loadsong("ninjamedeko2.smu"); // replaces any previously loaded song (can only handle one at a time)

	playmu_playsong(0); // parameter is fade-in time in milliseconds
	playmu_songvolume(80); // percent of original volume

	printf("press enter to quit");
	getchar();
	
	// ----
	
	playmu_playsound(testeffect, 50); // 50 = sound volume in percent of original

	printf("fading out...");
	playmu_fadeout(3000); // milliseconds, again (only affects music volume)
	
	Sleep(3500); // sleep a little extra (500 ms) to allow for stream latency

	playmu_close();
}

