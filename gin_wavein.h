#ifndef gin_wavein_h
#define gin_wavein_h

#include "musagi.h"
#include "gear_instrument.h"

struct wavein_params
{
	float gain;
	bool dynamic;
};

class wavein_channel : gear_ichannel
{
public:
	// parameters
	wavein_params *params;
	
	wavein_channel()
	{
		params=NULL;
	};

	~wavein_channel()
	{
	};

	int RenderBuffer(StereoBufferP *buffer, int size)
	{
		StereoBufferP* wavein=GetWaveInBuffer();

		float gainv=pow(params->gain*4.0f, 2.0f);

		float volprobe=0.0f;
		
		static float plevel=1.0f;
		static float dgain=0.0f;

		for(int i=0;i<size;i++)
		{
			float sample=0.0f;

			sample=(wavein->left[i]+wavein->right[i])*dgain;
			
			volprobe+=fabs(sample);
			
			plevel+=(fabs(sample)-plevel)*0.0003f;
			if(!params->dynamic)
				plevel=0.2f;
			if(plevel<0.02f)
				plevel=0.02f;
			if(plevel>5.0f)
				plevel=5.0f;
			dgain=1.0f/plevel;

			buffer->left[i]+=sample*gainv;
		}
		
		if(volprobe>0.1f)
			active=true;
		else
			active=false;

		return 1; // produced mono output (left channel)
	};
	
	void Trigger(int tnote, float tvolume)
	{
	};
	
	void Release()
	{
	};

	void Slide(int tnote, int length)
	{
	};
};

class gin_wavein : public gear_instrument
{
private:
	wavein_params params;
	
public:
	gin_wavein()
	{
//		LogPrint("gin_wavein: init");

		strcpy(name, "wavein");
		strcpy(visname, name);

		params.gain=1.0f/4;
		params.dynamic=true;

		numchannels=1;
		for(int i=0;i<numchannels;i++)
		{
			channels[i]=(gear_ichannel*)(new wavein_channel());
			((wavein_channel*)channels[i])->params=&params;
		}
	};

	~gin_wavein()
	{
	};

	void DoInterface(DUI *dui)
	{
		dui->DrawTexturedBar(0, 0, 400, 20, dui->palette[7], dui->brushed_metal, &dui->vglare);
		dui->DrawTexturedBar(0, 20, 400, 10, dui->palette[8], dui->brushed_metal, &dui->vglare);
		dui->DrawTexturedBar(0, 30, 400, 20, dui->palette[7], dui->brushed_metal, &dui->vglare);
		dui->DrawTexturedBar(0, 3, 10, 1, dui->palette[4], dui->brushed_metal, NULL);
		dui->DrawTexturedBar(10, 3, 400, 1, dui->palette[4], dui->brushed_metal, &dui->vglare);
		dui->DrawTexturedBar(0, 5, 10, 1, dui->palette[4], dui->brushed_metal, NULL);
		dui->DrawTexturedBar(10, 5, 400, 1, dui->palette[4], dui->brushed_metal, &dui->vglare);
		dui->DrawTexturedBar(0, 44, 10, 1, dui->palette[4], dui->brushed_metal, NULL);
		dui->DrawTexturedBar(10, 44, 400, 1, dui->palette[4], dui->brushed_metal, &dui->vglare);
		dui->DrawTexturedBar(0, 46, 10, 1, dui->palette[4], dui->brushed_metal, NULL);
		dui->DrawTexturedBar(10, 46, 400, 1, dui->palette[4], dui->brushed_metal, &dui->vglare);
		dui->DrawBox(0, 1, 400, 50-1, dui->palette[6]);
		dui->DrawText(335-1, 40, dui->palette[4], "wavein");
		dui->DrawText(335+1, 40, dui->palette[4], "wavein");
		dui->DrawText(335, 40, dui->palette[6], "wavein");
		dui->DoKnob(20, 30-8, params.gain, -1, "knob_gain", "volume gain");
		dui->DrawText(15, 13, dui->palette[6], "gain");
/*		dui->DoKnob(75, 30-8, params.filter_resonance, -1, "knob_resonance", "amount of resonance at cutoff frequency");
		dui->DrawText(75-4, 12, dui->palette[6], "reso");
		dui->DoKnob(120, 30-8, params.test_envmax, -1, "knob_envmax", "scale of envelope modulation - envelope controls cutoff");
		dui->DrawText(120-14, 12, dui->palette[6], "env.mod.");
		dui->DoKnob(165, 30-8, params.test_envspeed, -1, "knob_envspeed", "speed of envelope decay");
		dui->DrawText(165-8, 12, dui->palette[6], "decay");
		dui->DoKnob(210, 30-8, params.test_envshape, -1, "knob_envshape", "envelope decay linearity");
		dui->DrawText(210-10, 12, dui->palette[6], "env.shp");*/

		dui->DrawText(100, 21, dui->palette[6], "dynamic");
		dui->DrawLED(65, 20, 0, params.dynamic);
		if(dui->DoButton(75, 20, "btn_dg", "dynamic gain"))
			params.dynamic=!params.dynamic;
/*
		dui->DrawBar(329+i*3, 10, 4, 30, dui->palette[6]);
		dui->DrawBar(330+i*3, 10, 2, 30, dui->palette[4]);
		dui->DrawBar(330+i*3, 40-(int)(params.debug_bands[i]*30), 2, (int)(params.debug_bands[i]*30), dui->palette[15]);*/
	};

	bool Save(FILE *file)
	{
		kfwrite2(name, 64, 1, file);
		int numbytes=sizeof(wavein_params)+64;
		kfwrite2(&numbytes, 1, sizeof(int), file);
		kfwrite2(visname, 64, 1, file);
		kfwrite(&params, 1, sizeof(wavein_params), file);
		return true;
	};

	bool Load(FILE *file)
	{
		int numbytes=0;
		kfread2(&numbytes, 1, sizeof(int), file);
		// crude compatibility/version test
		if(numbytes!=sizeof(wavein_params)+64)
		{
			LogPrint("ERROR: tried to load incompatible version of instrument \"%s\"", name);
			ReadAndDiscard(file, numbytes);
			return false;
		}
		kfread2(visname, 64, 1, file);
		kfread(&params, 1, sizeof(wavein_params), file);
		return true;
	};
};

#endif

