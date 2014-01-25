#ifndef AudioStream_h
#define AudioStream_h

class AudioStream;

#include <stdio.h>
#include <portaudio.h>
#include "timer.h"
#include "musagi.h"
#include "part.h"
#include "song.h"

//static int pa_callback(void*, void*, unsigned long, PaTimestamp, void*);
//int pa_callback(void*, void*, unsigned long, PaTimestamp, void*);

struct PPart
{
	Part *part;
	int start;
	bool expired;
};

class AudioStream
{
public:
	PortAudioStream *stream;

	StereoBufferP buffer;
	
	GearStack **gearstacks;
	int num_gearstacks;
	GearStack **newlist;
	
	PPart *parts;
	int numparts;
	
	Song *song;

	int globaltick;

	bool metronome_on;	
	float metronome_vol;
	int metronome;
	int metrocount;
	int blipcount;
	float blipvol;
	float blipangle;
	
	float master_volume;

	// for file output
	FILE *foutput;
	bool file_output;
	int file_stereosampleswritten;
	int foutstream_datasize;

	// for performance measurement		
	CTimer *perftimer;
	float dtime;
	int dtime_num;
	float cpu_usage;
	
	float peak_left;
	float peak_right;
	int clip_left;
	int clip_right;

//public:
	AudioStream()
	{
		LogPrint("AudioStream: init");
		perftimer=new CTimer();
		perftimer->init();
		dtime=0;
		dtime_num=0;
		cpu_usage=0.0f;
		
		globaltick=0;
		metronome=0;
		blipcount=0;
		metronome_on=true;
		metronome_vol=0.5f;
	
		buffer.left=NULL;
		buffer.right=NULL;
		buffer.size=0;
		
		peak_left=0.0f;
		peak_right=0.0f;
		clip_left=false;
		clip_right=false;
		
		master_volume=0.25f;
		
		gearstacks=(GearStack**)malloc(512*sizeof(GearStack*));
		num_gearstacks=0;
//		newlist=NULL;
		newlist=(GearStack**)malloc(512*sizeof(GearStack*));
		
		song=NULL;
		
		parts=(PPart*)malloc(1024*sizeof(PPart));
		numparts=0;

		file_output=false;

		// init portaudio
		Pa_Initialize();
		stream=NULL;
		LogPrint("AudioStream: init done");
	};

	~AudioStream()
	{
		if(stream!=NULL)
			StopStream();
		// close portaudio
	    Pa_Terminate();

		delete perftimer;
		
		free(buffer.left);
		free(buffer.right);
		
		free(gearstacks);
		free(newlist);
		free(parts);
	};
	
	int GetTick()
	{
		return globaltick;
	};
	
	bool StartStream(int buffersize, int (*pa_callback)(void*, void*, unsigned long, PaTimestamp, void*))
	{
		LogPrint("AudioStream: start stream");
		if(stream!=NULL) // already streaming
			return false;

		if(buffer.size!=buffersize)
		{
			buffer.size=buffersize;
			free(buffer.left);
			free(buffer.right);
			buffer.left=(float*)malloc(buffer.size*sizeof(float));
			buffer.right=(float*)malloc(buffer.size*sizeof(float));
		}
		
		Pa_OpenDefaultStream(
				&stream,
				0,				// input channels
				2,				// output channels
				paFloat32,		// 32 bit floating point output
				44100,
				buffer.size,		// frames per buffer
				0,				// number of buffers, if zero then use default minimum
				pa_callback,
				this);
	    Pa_StartStream(stream);
	    
	    return true;
	};
	
	bool StopStream()
	{
		if(stream==NULL)
			return false;

	    Pa_StopStream(stream);
	    Pa_CloseStream(stream);
	    stream=NULL;
	    
	    if(buffer.size>0)
	    {
			free(buffer.left);
			free(buffer.right);
			buffer.left=NULL;
			buffer.right=NULL;
			buffer.size=0;
		}

	    return true;
	};

	// pause to make sure callback function has realized that a variable changed (not needed?)
/*	void WaitForCallback()
	{
		Sleep(1);
	};
*/	
	void StartFileOutput(char *filename)
	{
		foutput=fopen(filename, "wb");
		// write wav header
		char string[32];
		unsigned int dword=0;
		unsigned short word=0;
		fwrite("RIFF", 4, 1, foutput); // "RIFF"
		dword=0;
		fwrite(&dword, 1, 4, foutput); // remaining file size
		fwrite("WAVE", 4, 1, foutput); // "WAVE"
	
		fwrite("fmt ", 4, 1, foutput); // "fmt "
		dword=16;
		fwrite(&dword, 1, 4, foutput); // chunk size
		word=1;
		fwrite(&word, 1, 2, foutput); // compression code
		word=2;
		fwrite(&word, 1, 2, foutput); // channels
		dword=44100;
		fwrite(&dword, 1, 4, foutput); // sample rate
		dword=44100*4;
		fwrite(&dword, 1, 4, foutput); // bytes/sec
		word=4;
		fwrite(&word, 1, 2, foutput); // block align
		word=16;
		fwrite(&word, 1, 2, foutput); // bits per sample
	
		fwrite("data", 4, 1, foutput); // "data"
		dword=0;
		foutstream_datasize=ftell(foutput);
		fwrite(&dword, 1, 4, foutput); // chunk size
	
		// sample data	

		file_stereosampleswritten=0;
		file_output=true;
	};
	
	void StopFileOutput()
	{
		if(file_output)
		{
			file_output=false;

//			WaitForCallback();

			// seek back to header and write size info
			fseek(foutput, 4, SEEK_SET);
			unsigned int dword=0;
			dword=foutstream_datasize-4+file_stereosampleswritten*4;
			fwrite(&dword, 1, 4, foutput); // remaining file size
			fseek(foutput, foutstream_datasize, SEEK_SET);
			dword=file_stereosampleswritten*4;
			fwrite(&dword, 1, 4, foutput); // chunk size (data)
			fclose(foutput);
		}
	};
	
	void AddGearStack(GearStack *new_stack)
	{
		LogPrint("AudioStream: add gearstack, instrument [%.8X]", new_stack->instrument);
		gearstacks[num_gearstacks]=new_stack;
		num_gearstacks++;
	};

	void RemoveGearStack(GearStack *expired_stack)
	{
		LogPrint("AudioStream: remove gearstack %.8X", expired_stack);
		// create new array to avoid messing up the old one while it's in use
//		if(newlist!=NULL)
//			free(newlist);
//		LogPrint("as: allocated new list");
		int ni=0;
		int ei=0;
		for(int i=0;i<num_gearstacks;i++)
			if(gearstacks[i]!=expired_stack)
				newlist[ni++]=gearstacks[i];
			else
				gearstacks[i]=NULL;
		num_gearstacks--;
		for(int i=0;i<num_gearstacks;i++)
			gearstacks[i]=newlist[i];
//		gearstacks=newstacks;
//		LogPrint("as: filled new list");
	};
/*
	void FlushGearstackList() // bleh, damn threading/callback issues
	{
		if(newlist!=NULL)
		{
			LogPrint("AudioStream: reassigning gearstack list");
			GearStack **oldlist=gearstacks;
			num_gearstacks--;
			gearstacks=newlist;
//			LogPrint("as: reassigned old list");
			free(oldlist);
			newlist=NULL;
//			LogPrint("as: released old list");
			for(int i=0;i<num_gearstacks;i++)
				LogPrint("AudioStream: %i: %.8X", i, gearstacks[i]);
		}
	};
*/	
	void ResetClipMarkers()
	{
		clip_left=0;
		clip_right=0;
	};

	void AddPart(Part *part)
	{
		RemovePart(part); // in case it's already in... ?
		
		numparts++;
		parts[numparts-1].part=part;
		parts[numparts-1].start=globaltick;
		parts[numparts-1].expired=false;
//		LogPrint("audiostream: added part %i (%.8X) at tick %i", numparts-1, part, globaltick);
	};

	void RemovePart(Part *part)
	{
		bool found=false;
                int i;
		for(i=0;i<numparts;i++)
			if(parts[i].part==part)
			{
				found=true;
				break;
			}
		if(found)
		{
//			LogPrint("audiostream: removed part %i (%.8X)", i, parts[i].part);
			for(int j=i;j<numparts-1;j++)
				parts[j]=parts[j+1];
			numparts--;
		}
	};

	void RemoveAllParts()
	{
		numparts=0;
	};

	void Flush()
	{
		RemoveAllParts();
		num_gearstacks=0;
	};
	
	void SetSong(Song *s)
	{
		song=s;
	};
};

#endif

