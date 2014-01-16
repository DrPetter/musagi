#ifndef gin_midperc_h
#define gin_midperc_h

#include "gear_instrument.h"

struct midperc_params
{
	float vol[12];
	float fnote[12];
	int note[12];
	int played[12];
};

class midperc_channel : gear_ichannel
{
public:
	// parameters
	midperc_params *params;

	int curnote;
	int curvel;

	midperc_channel()
	{
		curnote=0;
		curvel=0;
		params=NULL;
		
		tonal=false;
	};

	~midperc_channel()
	{
	};

	int RenderBuffer(StereoBufferP *buffer, int size)
	{
		return 0; // no output
	};

	void Trigger(int tnote, float tvolume)
	{
		active=true;
		
		curnote=tnote%12;
		curvel=(int)(tvolume*params->vol[curnote]*127);
		midi_noteon(9, params->note[curnote], curvel);
		params->played[curnote]=5;
	};
	
	void Release()
	{
		active=false;
		midi_noteoff(9, params->note[curnote], curvel);
	};

	void Slide(int tnote, int length)
	{
	};
};

class gin_midperc : public gear_instrument
{
private:
	midperc_params params;

public:
	gin_midperc()
	{
		strcpy(name, "midperc");
		strcpy(visname, name);

		for(int i=0;i<12;i++)
			params.vol[i]=1.0f;
		params.note[0]=35;
		params.note[1]=38;
		params.note[2]=39;
		params.note[3]=46;
		params.note[4]=44;
		params.note[5]=42;
		params.note[6]=49;
		params.note[7]=51;
		params.note[8]=47;
		params.note[9]=48;
		params.note[10]=50;
		params.note[11]=56;
		for(int i=0;i<12;i++)
			params.fnote[i]=(float)(params.note[i]-35)/46;

		atomic=true;
		midi=true;

		numchannels=16;
		for(int i=0;i<numchannels;i++)
		{
			channels[i]=(gear_ichannel*)(new midperc_channel());
			((midperc_channel*)channels[i])->params=&params;
		}
	};

	~gin_midperc()
	{
	};

	void DoInterface(DUI *dui)
	{
		dui->DrawBar(1, 0, 400, 5, dui->palette[4]);
		dui->DrawBar(1, 45, 400, 5, dui->palette[4]);
		dui->DrawBar(1, 5, 400, 40, dui->palette[7]);
		dui->DrawBox(0, 1, 400, 49, dui->palette[6]);

		dui->DrawText(375, 8, dui->palette[6], "mid");
		dui->DrawText(375, 7, dui->palette[9], "mid");
		dui->DrawText(371, 18, dui->palette[6], "perc");
		dui->DrawText(371, 17, dui->palette[9], "perc");

		char string[16];
		int active_knob=-1;
		for(int i=0;i<12;i++)
		{
			int x=i*30;
			dui->DrawText(26+x, 28, dui->palette[10], "%i", i+1);
			sprintf(string, "knob_vol%i", i);
			dui->DoKnob(10+x, 4, params.vol[i], -1, string, "volume/velocity");
//			dui->DrawLED(5+x, 2, 1, params.size[i]>0);
			dui->DrawLED(5+x, 16, 0, params.played[i]>0);
			if(params.played[i]>0)
				params.played[i]--;
			dui->DrawText(16+x, 36, dui->palette[6], "%.2i", params.note[i]);
			sprintf(string, "knob_note%i", i);
			if(dui->DoKnob(10+x, 20, params.fnote[i], -1, string, "midi drum number"))
				active_knob=i;
			params.note[i]=35+(int)(params.fnote[i]*46);
		}
		if(active_knob>-1)
		{
			i=active_knob;
			dui->DrawText(5, 40, dui->palette[9], "%.2i %s", params.note[i], midi_getpname(params.note[i]));
		}
/*
		dui->DrawText(24, 10, dui->palette[6], "volume:");
		dui->DoKnob(64, 6, params.vol, -1, "knob_vol", "volume/velocity");
		dui->DrawText(84, 10, dui->palette[6], "%i%%", (int)(params.vol*100));

		dui->DrawText(5, 30, dui->palette[6], "instrument:");
		dui->DoKnob(64, 26, params.fprogram, -1, "knob_pr", "midi instrument/program");
		params.program=(int)(params.fprogram*127);
		dui->DrawText(84, 30, dui->palette[6], "%.3i %s", params.program+1, midi_getiname(params.program));
*/
	};

	bool Save(FILE *file)
	{
		kfwrite2(name, 64, 1, file);
		int numbytes=sizeof(midperc_params)+64;
		kfwrite2(&numbytes, 1, sizeof(int), file);
		kfwrite2(visname, 64, 1, file);
		kfwrite(&params, 1, sizeof(midperc_params), file);
		return true;
	};

	bool Load(FILE *file)
	{
		int numbytes=0;
		kfread2(&numbytes, 1, sizeof(int), file);
		kfread2(visname, 64, 1, file);
		kfread2(&params, 1, sizeof(midperc_params), file);
		return true;
	};
};

#endif

