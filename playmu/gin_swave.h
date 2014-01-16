#ifndef gin_swave_h
#define gin_swave_h

#include "musagi.h"
#include "gear_instrument.h"

struct swave_params
{
	float vol[12];
	float spd[12];
	int size[12];
	int played[12];
	float *wavedata[12];
};

class swave_channel : gear_ichannel
{
public:
	// parameters
	swave_params *params;

	bool playing;
	float t;
	float ft;
	float pft;
	int waveid;

	float volume;
	int irval;
	int tick;
	
	swave_channel()
	{
		params=NULL;

		tick=0;
		volume=0.0f;
		irval=0;

		playing=false;
		t=0.0f;
		ft=0.0f;
		pft=0.0f;
		waveid=0;
	};

	~swave_channel()
	{
	};

	float WaveformSample(float t1, float t2, float *wave, int length)
	{
//		if(t2<t1) // wrapped
//			t2+=1.0f;
		t1*=length;
		t2*=length;

		if(t2-t1<1.0f) // we're playing at less than the original speed, filter accordingly
		{
			int i1=(int)(t1-0.5f);
			int i2=i1+1;
			if(i2>length-1) i2=length-1;
			float f=t1-(float)i1;
			return wave[i1]*(1.0f-f)+wave[i2]*f;
		}

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

		float vol=params->vol[waveid]*2.0f; // *2.0f to compensate for high peaks generally bringing down overall volume
		float spd=0.2f+pow(params->spd[waveid]*2.0f, 2.0f)*0.8f;
//		if(fabs(params->spd[waveid]-0.5f)<0.05f)
//			spd=1.0f;
		int isize=params->size[waveid];
		float fsize=(float)isize;

		if(vol==0.0f || isize==0)
		{
			active=false;
			return 0;
		}

		params->played[waveid]=0;		
		float *wave=params->wavedata[waveid];

		for(int si=0;si<size;si++)
		{
			t+=spd;
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

			buffer->left[si]+=sample*volume;
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
		waveid=tnote%12;
		params->played[waveid]=0;		
		
//		LogPrint("swave: triggering wave %i (%.8X, size=%i)", waveid, params->wavedata[waveid], params->size[waveid]);

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

class gin_swave : public gear_instrument
{
private:
	swave_params params;

public:
	gin_swave()
	{
//		LogPrint("gin_swave: init");

		strcpy(name, "swave");
		strcpy(visname, name);

		for(int i=0;i<12;i++)
		{
			params.vol[i]=0.0f;
			params.spd[i]=0.5f;
			params.size[i]=0;
			params.played[i]=5;
			params.wavedata[i]=NULL;
		}

		params.vol[0]=1.0f;
		params.spd[0]=0.5f;
		params.size[0]=5000;
		params.wavedata[0]=(float*)malloc(params.size[0]*sizeof(float));
		for(int i=0;i<params.size[0];i++)
		{
			if(i<params.size[0]-2000)
				params.wavedata[0][i]=(float)sin((float)i*0.05f);
			else
				params.wavedata[0][i]=(float)sin((float)i*0.05f)*(float)(params.size[0]-i)/2000;
			if(params.wavedata[0][i]>1.0f)
				params.wavedata[0][i]=1.0f;
			if(params.wavedata[0][i]<-1.0f)
				params.wavedata[0][i]=-1.0f;
//			params.wavedata[0][i]=(float)((char)(params.wavedata[0][i]*127))/127;
		}

		atomic=true;
		
		numchannels=32;
		for(int i=0;i<numchannels;i++)
		{
			channels[i]=(gear_ichannel*)(new swave_channel());
			((swave_channel*)channels[i])->params=&params;
		}
	};

	~gin_swave()
	{
		for(int i=0;i<12;i++)
			if(params.size[i]>0)
				free(params.wavedata[i]);
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
/*		if(numbytes!=sizeof(swave_params))
		{
			LogPrint("ERROR: tried to load incompatible version of instrument \"%s\"", name);
			char junk[4096];
			fread(junk, 1, sizeof(numbytes), file);
			return false;
		}*/
		kfread2(visname, 64, 1, file);
		kfread(&params, 1, sizeof(swave_params), file);
		for(int i=0;i<12;i++)
			if(params.size[i]>0)
			{
				params.wavedata[i]=(float*)malloc(params.size[i]*sizeof(float));
				for(int wi=0;wi<params.size[i];wi++)
				{
//					char sample;
//					kfread2(&sample, 1, sizeof(char), file);
//					params.wavedata[i][wi]=(float)sample/127;
					short sample;
					kfread2(&sample, 1, sizeof(short), file);
					params.wavedata[i][wi]=(float)sample/32767;
				}
			}
		return true;
	};
};

#endif

