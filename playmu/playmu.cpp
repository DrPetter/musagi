#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "Log.h"

#include "musagi.h"
#include "musagig.h"

#include "playmu.h"

#include "playmu_sound.h"

float playmu_sysvolume=1.0f;

bool audio_enabled=true;

void playmu_disable()
{
	audio_enabled=false;
}

#include "audiostream.h"
#include "pa_callback.h"

#include "gin_protobass.h"
#include "gin_xnes.h"
#include "gin_chip.h"
#include "gin_swave.h"
#include "gin_vsmp.h"
#include "gin_operator.h"

#include "gef_gapan.h"

#include "part.h"

#include "song.h"

#include "part_song_cpp.h"

/*
void LogPrint(char* string) // dummy
{
}
*/
AudioStream *audiostream;

GearStack gearstacks[128];
int maxstacks=128;

GearStack* MapToGearStack(int i)
{
	if(i==-1)
		return NULL;
	return &gearstacks[i];
}

Part *mungoparts[1024];
int maxparts=1024;

Song *thesong;

//int log_on=0;

float ftempo=0.5f;
float master_volume=0.5f;

int stacklist[128];
int stacklistnum=0;
/*
int GetTick()
{
	return audiostream->GetTick(1);
}
*/
unsigned int GetTick(int mode)
{
	return audiostream->GetTick(mode);
}

float UpdateTempo()
{
	int fractempo=1945+(int)(pow(1.0f-ftempo, 2.0f)*6000);
	float bpm=44100.0f/(32*320*fractempo/1600)*60;
	bpm=(float)((int)bpm);
	float rounded_tempo=(1.0f/(bpm/60))*44100.0f/32*1600/320;
	tempo=(int)rounded_tempo;
	return bpm;
}


void Reset();

int errorcount=0;

#include "main_fileops.h"


//----------------------------------------

int KnobRecState(float *value)
{
	return thesong->KnobRecState(value);
}

bool KnobAtMemPos(float *pos)
{
	return thesong->KnobAtMemPos(pos);
}

void KnobToMemPos(float *oldpos, float *newpos)
{
	thesong->KnobToMemPos(oldpos, newpos);
}

bool note_playing=false;

void Reset()
{
	LogPrint("init instruments");

	stacklistnum=0;

	for(int i=0;i<maxstacks;i++)
	{
		gearstacks[i].instrument=NULL;
		gearstacks[i].num_effects=0;
		gearstacks[i].winshow=false;
		gearstacks[i].winpop=false;
		gearstacks[i].snappedright=false;
		gearstacks[i].winx=-999;
		gearstacks[i].winy=0;
	}

	for(int i=0;i<maxparts;i++)
		mungoparts[i]=NULL;

//	current_part=NULL;
//	fcurstack=0;

	thesong=new Song();
//	thesong->scrollfollow=scrollfollow;

//	curfilename[0]='\0';

	ftempo=0.5f;
	master_volume=0.5f;

//	for(int i=0;i<maxstacks;i++)
//		if(gearstacks[i].instrument!=NULL)
//			audiostream->AddGearStack(&gearstacks[i]);

	LogPrint("main: set song");

	audiostream->SetSong(thesong);
}

void playmu_fadeout(int milliseconds)
{
	audiostream->delta_volume=-audiostream->default_volume/(44100/128)/((float)milliseconds/1000);
}

void playmu_songvolume(int percent)
{
	audiostream->default_volume=audiostream->loaded_volume*(float)percent/100;
	audiostream->master_volume=audiostream->default_volume;
//	audiostream->master_volume=pow(master_volume, 2.0f)*0.5f;
}

void playmu_mastervolume(int percent)
{
	playmu_sysvolume=(float)percent/100;
}

bool playmu_loadsong(char *filename)
{
	LogPrint("->stop");
	thesong->Stop();
	LogPrint("LoadSong()");
	return LoadSong(filename);
}

void playmu_playsong(int fadein_milliseconds)
{
	if(fadein_milliseconds!=0)
	{
		audiostream->master_volume=0.0f;
		audiostream->delta_volume=audiostream->default_volume/(44100/128)/((float)fadein_milliseconds/1000);
	}
	thesong->PlayButton();
}

void playmu_stopsong()
{
	thesong->Stop();
}

void playmu_init()
{
	srand(time(NULL));
	
//	LogStart("playmu_log.txt");

	InitGlobals();
	tempo=44;

	// sound effects
	for(int i=0;i<2048;i++)
		playmu_soundbank[i]=NULL;
	for(int i=0;i<64;i++)
		playmu_sesounds[i].sound=NULL;
	playmu_soundbuffer.size=0;
	playmu_soundbuffer.left=NULL;
	playmu_soundbuffer.right=NULL;
	
	playmu_sysvolume=1.0f;

	LogPrint("init audiostream");
	audiostream=new AudioStream();

	Reset();
	
	LogPrint("main: start stream");
	audiostream->StartStream(1024, &pa_callback);
}

void playmu_close()
{
	delete audiostream;

	free(playmu_soundbuffer.left);
	free(playmu_soundbuffer.right);

	for(int i=0;i<maxstacks;i++)
		if(gearstacks[i].instrument!=NULL)
		{
			delete gearstacks[i].instrument;
			for(int j=0;j<gearstacks[i].num_effects;j++)
				if(gearstacks[i].effects[j]!=NULL)
					delete gearstacks[i].effects[j];
		}
	for(int i=0;i<maxparts;i++)
		if(mungoparts[i]!=NULL)
			delete mungoparts[i];

	delete thesong;

	for(int i=0;i<2048;i++)
		free(playmu_soundbank[i]);

	LogPrint("cleanup done");
}

