#ifndef gin_midinst_h
#define gin_midinst_h

#include "gear_instrument.h"

struct midinst_params
{
	float vol;
	int channel;
	float fprogram;
	int program;
	float foctave;
	int octave;
	void* self;
};

class midinst_channel : gear_ichannel
{
public:
	// parameters
	midinst_params *params;

	int curnote;
	int curvel;
//	int curprogram;

	midinst_channel()
	{
		curnote=0;
		curvel=0;
		params=NULL;
	};

	~midinst_channel()
	{
	};

	int RenderBuffer(StereoBufferP *buffer, int size)
	{
		return 0; // no output
	};

	void Trigger(int tnote, float tvolume)
	{
		active=true;

		params->channel=midi_allocatechannel(params->channel, params->program, params->self);
		
//		if(curprogram!=params->program)
//			midi_changeprogram(params->channel, params->program);
//		curprogram=params->program;
		
		curnote=tnote+params->octave*12;
		if(curnote<0) curnote=0;
		if(curnote>127) curnote=127;
		curvel=(int)(tvolume*params->vol*127);
		midi_noteon(params->channel, curnote, curvel);
	};
	
	void Release()
	{
		active=false;
		midi_noteoff(params->channel, curnote, curvel);
	};

	void Slide(int tnote, int length)
	{
	};
};

class gin_midinst : public gear_instrument
{
private:
	midinst_params params;

public:
	gin_midinst()
	{
//		LogPrint("gin_midinst: init");

		strcpy(name, "midinst");
		strcpy(visname, name);

		params.vol=1.0f;
		params.self=this;
		params.channel=midi_allocatechannel(0, 0, params.self);
		params.fprogram=0.0f;
		params.program=0;
		params.foctave=0.5f;
		params.octave=0;

		midi=true;

		numchannels=16;
		for(int i=0;i<numchannels;i++)
		{
			channels[i]=(gear_ichannel*)(new midinst_channel());
			((midinst_channel*)channels[i])->params=&params;
		}
	};

	~gin_midinst()
	{
		midi_freechannel(params.channel);
	};

	void DoInterface(DUI *dui)
	{
		dui->DrawBar(1, 0, 400, 5, dui->palette[4]);
		dui->DrawBar(1, 45, 400, 5, dui->palette[4]);
		dui->DrawBar(1, 5, 400, 40, dui->palette[7]);
		dui->DrawBox(0, 1, 400, 49, dui->palette[6]);

		dui->DrawText(342, 36, dui->palette[6], "midinst");
		dui->DrawText(342, 35, dui->palette[9], "midinst");

		dui->DrawText(24, 10, dui->palette[6], "volume:");
		dui->DoKnob(64, 6, params.vol, -1, "knob_vol", "volume/velocity");
		dui->DrawText(84, 10, dui->palette[10], "%i%%", (int)(params.vol*100));

		dui->DrawText(100+24, 10, dui->palette[6], "octave:");
		dui->DoKnob(100+64, 6, params.foctave, -1, "knob_oct", "octave offset");
		params.octave=(int)((params.foctave-0.5f)*6);
		if(params.octave==0)
			dui->DrawText(100+84, 10, dui->palette[10], " 0");
		if(params.octave>0)
			dui->DrawText(100+84, 10, dui->palette[10], "+%i", params.octave);
		if(params.octave<0)
			dui->DrawText(100+84, 10, dui->palette[10], "%i", params.octave);

		dui->DrawText(5, 30, dui->palette[6], "instrument:");
		dui->DoKnob(64, 26, params.fprogram, -1, "knob_pr", "midi instrument/program");
		params.program=(int)(params.fprogram*127);
		dui->DrawText(84, 30, dui->palette[9], "%.3i %s", params.program+1, midi_getiname(params.program));
	};

	bool Save(FILE *file)
	{
		kfwrite2(name, 64, 1, file);
		int numbytes=sizeof(midinst_params)+64;
		kfwrite2(&numbytes, 1, sizeof(int), file);
		kfwrite2(visname, 64, 1, file);
		kfwrite(&params, 1, sizeof(midinst_params)-4, file);
		return true;
	};

	bool Load(FILE *file)
	{
		int numbytes=0;
		kfread2(&numbytes, 1, sizeof(int), file);
		kfread2(visname, 64, 1, file);
		kfread2(&params, 1, sizeof(midinst_params)-4, file);
		params.self=this;
		return true;
	};
};

#endif

