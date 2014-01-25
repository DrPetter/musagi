#ifndef musagi_h
#define musagi_h

#include <math.h>
#include <stdio.h>

#define rnd(n) (rand()%(n+1))

#define PI 3.14159265f

struct Trigger
{
	int note;
	int start;
	int duration;
	float volume;
	int slidenote;
	int slidelength;
	bool selected; // for gui
};

struct StereoBufferP
{
	float *left;
	float *right;
	bool mono; // means there's identical data in left and right channel, a performance hint for effects etc
	int size;
	bool undefined;
};

FILE* kfopen(char *filename, char *mode);
void kfclose(FILE *file);
void kfsaveknobs(FILE *file);
void kfloadknobs(FILE *file);
void kfwrite2(void *data, int num, int size, FILE *file);
void kfread2(void *data, int num, int size, FILE *file);
void kfwrite(void *data, int num, int size, FILE *file);
void kfread(void *data, int num, int size, FILE *file);

bool KnobAtMemPos(float *pos);
void KnobToMemPos(float *oldpos, float *newpos);

int KnobRecState(float *value);

void ReadAndDiscard(FILE *file, int numbytes);

void ZeroBuffer(float *buffer, int size);

float lerp(float x, float a, float b);

float GetNoteFrequency(int note);

int GetNotePos(int note);

char* GetNoteStr(int note);

bool NoteIsBlack(int note);

bool IsKeyDown(int i);

void SetKeyDown(int i, bool state);

//#include "Part.h"

//class Part;

void SetCurrentPart(void *part);

void PartCopy_Reset();
int PartCopy_NumTriggers();
Trigger PartCopy_Triggers(int i);
void PartCopy_Add(Trigger trig);

int GetTempo();

int GetTick();

//HWND GetHwnd();

struct Color
{
	float r,g,b;
};

#include "Log.h"
#include "gear_instrument.h"
#include "gear_effect.h"

struct GearStack
{
	gear_instrument *instrument;
	gear_effect *effects[32];
	int num_effects;
	int winx, winy;
	bool winshow;
	bool winpop;
	bool snappedright;
};

GearStack *GetCurrentGearstack();

#endif

