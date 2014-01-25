#ifndef musagig_h
#define musagig_h

#include <math.h>

#include "AudioStream.h"

void ZeroBuffer(float *buffer, int size)
{
	for(int i=0;i<size;i+=8)
	{
		*buffer++=0.0f;
		*buffer++=0.0f;
		*buffer++=0.0f;
		*buffer++=0.0f;
		*buffer++=0.0f;
		*buffer++=0.0f;
		*buffer++=0.0f;
		*buffer++=0.0f;
	}
}

bool kf_reading;
DWORD kf_pos;
int kf_numknobs;
int kf_curknob;
DWORD kf_fknobs[1024];
float *kf_pknobs1[1024];
float *kf_pknobs2[1024];

FILE* kfopen(char *filename, char *mode)
{
	kf_pos=0;
	kf_numknobs=0;
	kf_curknob=0;
	FILE *file=fopen(filename, mode);
	kf_reading=true;
	if(mode[0]=='w') // skip ahead to leave room for kf data
	{
		LogPrint("kfopen: skipping header");
		kf_reading=false;
		kf_pos=2*1024*4+4;
		fseek(file, kf_pos, SEEK_SET);
	}
	return file;
}

void kfclose(FILE *file)
{
	if(kf_reading)
	{
		LogPrint("kfclose: remapping knobs");
		for(int i=0;i<kf_numknobs;i++)
			KnobToMemPos(kf_pknobs1[i], kf_pknobs2[i]);
	}
	fclose(file);
}

void kfsaveknobs(FILE *file)
{
	LogPrint("kfsaveknobs: writing header at file start");
	fseek(file, 0, SEEK_SET); // put this block at the beginning of the file
	fwrite(&kf_numknobs, 1, sizeof(int), file);
	fwrite(kf_fknobs, 1024, sizeof(DWORD), file);
	fwrite(kf_pknobs1, 1024, sizeof(float*), file);
}

void kfloadknobs(FILE *file)
{
	LogPrint("kfsaveknobs: loading header");
	kfread2(&kf_numknobs, 1, sizeof(int), file);
	kfread2(kf_fknobs, 1024, sizeof(DWORD), file);
	kfread2(kf_pknobs1, 1024, sizeof(float*), file);
}

void kfwrite2(void *data, int num, int size, FILE *file)
{
	kf_pos+=num*size;
	fwrite(data, num, size, file);
}

void kfread2(void *data, int num, int size, FILE *file)
{
	kf_pos+=num*size;
	fread(data, num, size, file);
}

void kfwrite(void *data, int num, int size, FILE *file)
{
	DWORD end=(DWORD)data+num*size;
	for(DWORD m=(DWORD)data;m<end;m++)
	{
		if(KnobAtMemPos((float*)m))
		{
			LogPrint("kfwrite: knob %i detected at mem pos $%.8X, mapped to file pos $%.8X (actually $%.8X)", kf_numknobs, m, kf_pos, ftell(file));
			kf_fknobs[kf_numknobs]=kf_pos;
			kf_pknobs1[kf_numknobs]=(float*)m;
			kf_numknobs++;
		}
		kf_pos++;
	}
	fwrite(data, num, size, file);
}

void kfread(void *data, int num, int size, FILE *file)
{
	if(kf_curknob<kf_numknobs)
	{
		DWORD end=(DWORD)data+num*size;
		for(DWORD m=(DWORD)data;m<end;m++)
		{
			if(kf_pos==kf_fknobs[kf_curknob])
			{
				LogPrint("kfread: knob %i detected at file pos $%.8X (actually $%.8X), mapped to mem pos $%.8X", kf_curknob, kf_pos, ftell(file), m);
				kf_pknobs2[kf_curknob]=(float*)m;
				kf_curknob++;
			}
			kf_pos++;
		}
	}
	else
		kf_pos+=num*size;
	fread(data, num, size, file);
}

float lerp(float x, float a, float b)
{
	return a+(b-a)*x;
}

float notes[120];
char note_string[120][4];
int notepos[120];
bool gkbkeydown[120];
int tempo;
class AudioStream;
//AudioStream *gaudiostream;

int partcopy_numtriggers=0;
Trigger partcopy_triggers[1024];

#include "part.h"

Part *current_part;

void ReadAndDiscard(FILE *file, int numbytes)
{
	int crap[256];
	while(numbytes>0)
	{
		int nbytes=numbytes;
		if(nbytes>256)
			nbytes=256;
		fread(crap, nbytes, 1, file);
		numbytes-=256;
	}
}

float GetNoteFrequency(int note)
{
	return notes[note];
}

int GetNotePos(int note)
{
	return notepos[note];
}

char* GetNoteStr(int note)
{
	return note_string[note];
}

bool NoteIsBlack(int note)
{
	return (note_string[note][1]=='#');
}

bool IsKeyDown(int i)
{
	return gkbkeydown[i];
}

void SetKeyDown(int i, bool state)
{
	gkbkeydown[i]=state;
}

#include "part.h"
//class Part;

void SetCurrentPart(void *part)
{
	if(current_part!=NULL && current_part!=part)
		current_part->gearstack->instrument->ReleaseAll();
	current_part=(Part*)part;
}

void PartCopy_Reset()
{
	partcopy_numtriggers=0;
}

int PartCopy_NumTriggers()
{
	return partcopy_numtriggers;
}

Trigger PartCopy_Triggers(int i)
{
	return partcopy_triggers[i];
}

void PartCopy_Add(Trigger trig)
{
	if(partcopy_numtriggers>1023)
		return;
	partcopy_triggers[partcopy_numtriggers++]=trig;
}

int GetTempo()
{
	return tempo;
}

void InitGlobals()
{
	// Init notes
	float noteref[12];
	for(int i=0;i<12;i++)
	{
	    noteref[i]=440.0f/32*(float)pow(2.0f, (float)i/12);
	    note_string[i][2]='0';
	    note_string[i][3]='\0';
	}
    note_string[0][0]='C';
    note_string[0][1]='-';
    note_string[1][0]='C';
    note_string[1][1]='#';
    note_string[2][0]='D';
    note_string[2][1]='-';
    note_string[3][0]='D';
    note_string[3][1]='#';
    note_string[4][0]='E';
    note_string[4][1]='-';
    note_string[5][0]='F';
    note_string[5][1]='-';
    note_string[6][0]='F';
    note_string[6][1]='#';
    note_string[7][0]='G';
    note_string[7][1]='-';
    note_string[8][0]='G';
    note_string[8][1]='#';
    note_string[9][0]='A';
    note_string[9][1]='-';
    note_string[10][0]='A';
    note_string[10][1]='#';
    note_string[11][0]='B';
    note_string[11][1]='-';
    notes[0]=noteref[3];
    notes[1]=noteref[4];
    notes[2]=noteref[5];
    notes[3]=noteref[6];
    notes[4]=noteref[7];
    notes[5]=noteref[8];
    notes[6]=noteref[9];
    notes[7]=noteref[10];
    notes[8]=noteref[11];
    notes[9]=noteref[0]*2;
    notes[10]=noteref[1]*2;
    notes[11]=noteref[2]*2;
    notepos[0]=0;
    notepos[1]=1;
    notepos[2]=2;
    notepos[3]=3;
    notepos[4]=4;
    notepos[5]=6;
    notepos[6]=7;
    notepos[7]=8;
    notepos[8]=9;
    notepos[9]=10;
    notepos[10]=11;
    notepos[11]=12;
	for(int i=12;i<120;i++)
	{
		int sri=i%12;
	    notes[i]=notes[i-12]*2;
	    notepos[i]=notepos[i-12]+14;
//	    LogPrint("Note %.3i: %.2f Hz", i, notes[i]);
	    note_string[i][0]=note_string[sri][0];
	    note_string[i][1]=note_string[sri][1];
	    note_string[i][2]='0'+i/12;
	    note_string[i][3]='\0';
	}

	for(int i=0;i<120;i++)
		gkbkeydown[i]=false;
	
//	tempo=44;
};

#endif

