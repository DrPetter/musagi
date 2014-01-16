#ifndef gin_vsampler_h
#define gin_vsampler_h

#include "gear_instrument.h"

struct vsampler_params
{
	float vol;
	bool mono;
	int basenote;
	int size;
	int played;
	float *wavedata;
};

class vsampler_channel : gear_ichannel
{
public:
	// parameters
	vsampler_params *params;

	bool playing;
	float t;
	float ft;
	float pft;
	float speed;

	float volume;
	int irval;
	int tick;
	
	vsampler_channel()
	{
		params=NULL;

		tick=0;
		volume=0.0f;
		irval=0;

		playing=false;
		t=0.0f;
		ft=0.0f;
		pft=0.0f;
	};

	~vsampler_channel()
	{
	};

	float WaveformSample(float t1, float t2, float *wave, int length)
	{
//		if(t2<t1) // wrapped
//			t2+=1.0f;
		t1*=length;
		t2*=length;
		int i1=(int)(t1-0.5f);
		int i2=(int)(t2-0.5f);
		if(i1==i2) // both sample points on same wave sample
			return wave[i1];
		float fi1=(float)i1;
		float fi2=(float)i2;
		float p1=1.0f-(t1-fi1);
		float p2=t2-fi2;
		float ws=0.0f;
		int wsamples=0;
		for(int i=i1+1;i<i2;i++) // add all intermediate whole sample values
		{
			int wi=i;
//			if(wi>=length)
//				wi-=length;
			ws+=wave[wi];
			wsamples++;
		}
//		if(i2>=length)
//			i2-=length;
//		i1+=256-length;
//		i2+=256-length;
		if(wsamples>0)
			return (ws+wave[i1]*p1+wave[i2]*p2)/((float)wsamples+p1+p2);
		else
			return (wave[i1]*p1+wave[i2]*p2)/(p1+p2);
	};

	int RenderBuffer(StereoBufferP *buffer, int size)
	{
		if(!active)
			return 0; // no output

		float vol=params->vol*2.0f; // *2.0f to compensate for high peaks bringing down overall volume
//		float spd=0.2f+pow(params->spd[waveid]*2.0f, 2.0f)*0.8f;
//		if(fabs(params->spd[waveid]-0.5f)<0.05f)
//			spd=1.0f;
//		float speed=1.0f/period;
		int isize=params->size;
		float fsize=(float)isize;

		if(vol==0.0f || isize==0)
		{
			active=false;
			return 0;
		}

		params->played=0;		
		float *wave=params->wavedata;

		for(int si=0;si<size;si++)
		{
			t+=speed;
			if(t>=fsize)
			{
				active=false;
				break;
			}
			pft=ft;
			ft=t/fsize;

			float sample=0.0f;

//			sample+=(float)(rnd(16)-8)/8*vol;
			sample+=WaveformSample(pft, ft, wave, isize)*vol;

			buffer->left[si]+=sample*volume*2.0f;
			tick++;
		}

		return 1; // produced mono output (left channel)
	};

	void Trigger(int tnote, float tvolume)
	{
		active=true;
		volume=tvolume;

		playing=true;
		t=0.0f;
		ft=0.0f;
		pft=0.0f;
		speed=GetNoteFrequency(tnote)/GetNoteFrequency(params->basenote);
//		waveid=tnote%12;
//		params->played[waveid]=0;		
		
//		LogPrint("vsampler: triggering wave %i (%.8X, size=%i)", waveid, params->wavedata[waveid], params->size[waveid]);

		tick=0;
	};
	
	void Release()
	{
//		for(int i=0;i<1;i++)
//			channels[i].decay=true;
	};

	void Slide(int tnote, int length)
	{
	};
};

class gin_vsampler : public gear_instrument
{
private:
	vsampler_params params;

public:
	gin_vsampler()
	{
//		LogPrint("gin_vsampler: init");

		strcpy(name, "vsampler");
		strcpy(visname, name);

		params.played=5;
		params.wavedata=NULL;

		params.mono=true;
		params.vol=1.0f;
		params.size=5000;
		params.wavedata=(float*)malloc(params.size*sizeof(float));
		params.basenote=12*3;
		float frequency=GetNoteFrequency(params.basenote)/44100.0f;
		for(int i=0;i<params.size;i++)
		{
			if(i<params.size-2000)
				params.wavedata[i]=(float)sin((float)i*frequency*2*PI);
			else
				params.wavedata[i]=(float)sin((float)i*frequency*2*PI)*(float)(params.size-i)/2000;
			if(params.wavedata[i]>1.0f)
				params.wavedata[i]=1.0f;
			if(params.wavedata[i]<-1.0f)
				params.wavedata[i]=-1.0f;
			params.wavedata[i]=(float)((char)(params.wavedata[i]*127))/127;
		}

//		atomic=false;
		
		numchannels=16;
		for(int i=0;i<numchannels;i++)
		{
			channels[i]=(gear_ichannel*)(new vsampler_channel());
			((vsampler_channel*)channels[i])->params=&params;
		}
		numchannels=1;
	};

	~gin_vsampler()
	{
		if(params.size>0)
			free(params.wavedata);
	};

	bool Save(FILE *file)
	{
		return true;
	};

	bool Load(FILE *file)
	{
		int numbytes=0;
		kfread2(&numbytes, 1, sizeof(int), file);
		// crude compatibility/version test
/*		if(numbytes!=sizeof(vsampler_params))
		{
			LogPrint("ERROR: tried to load incompatible version of instrument \"%s\"", name);
			char junk[4096];
			fread(junk, 1, sizeof(numbytes), file);
			return false;
		}*/
		kfread2(visname, 64, 1, file);
		kfread2(&params, 1, sizeof(vsampler_params), file);
//		for(int i=0;i<12;i++)
//			if(params.size[i]>0)
//			{
				params.wavedata=(float*)malloc(params.size*sizeof(float));
				for(int wi=0;wi<params.size;wi++)
				{
					char sample;
					kfread2(&sample, 1, sizeof(char), file);
					params.wavedata[wi]=(float)sample/127;
				}
//			}
		if(params.mono)
			numchannels=1;
		else
			numchannels=16;
		return true;
	};
};

#endif

