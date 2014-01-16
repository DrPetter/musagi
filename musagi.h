#ifndef musagi_h
#define musagi_h

#include <windows.h>
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

struct Color
{
	float r,g,b;
};

Color ColorFromHSV(float hue, float sat, float val);

char* RPath(char* filename);

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

void** GetAllParts(int& arraysize);

void PartCopy_Reset();
int PartCopy_NumTriggers();
Trigger PartCopy_Triggers(int i);
void PartCopy_Add(Trigger trig);

float UpdateTempo();

int GetTempo();

int GetBeatLength();
void SetBeatLength(int i);

unsigned int GetTick(int mode);

DWORD GetHwnd(); // I hope this doesn't need to be here...

char* GetCurDir(int type);
void SetCurDir(int type, char* string);

char* GetMusagiDir();
void SetMusagiDir(char* path);

int GetFileVersion();
void SetFileVersion(int version);

class DUI;
DUI* GetDUI();

void EarTrigger(int note);
void EarRelease(int note);
void EarClear();
void EarUpdate();
int EarLevel();

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
//	StereoBufferP buffer;
//	int override;
};

GearStack *GetCurrentGearstack();

void midi_noteon(int channel, int note, int velocity);
void midi_noteoff(int channel, int note, int velocity);
void midi_changeprogram(int channel, int program);
int midi_allocatechannel();
void midi_freechannel(int i);
char* midi_getiname(int i);
char* midi_getpname(int i);

int GetMidiInCount();
char** GetMidiInNames();

void AbortRender();
void FinishRender();

int MapFromGearStack(GearStack* ptr);
GearStack* MapToGearStack(int i);

void SetWaveInBuffer(StereoBufferP* buffer);
StereoBufferP* GetWaveInBuffer();

#ifndef NMAX
#define NMAX 4096
#define NMAXSQRT 64
#endif
void cdft(int, int, double *, int *, double *);
void fft_convert(double *data, double *fft_buffer, int n, int direction); // direction  1=t->f  -1=f->t

#endif

