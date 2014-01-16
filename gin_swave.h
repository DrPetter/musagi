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
		
		tonal=false;
	};

	~swave_channel()
	{
	};

	float WaveformSample(float t1, float t2, float *wave, int length)
	{
		t1*=length;
		t2*=length;

		if(t1>length-1.0f) t1=length-1.0f;
		if(t2>length-1.0f) t2=length-1.0f;

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
		i2=i1;
//		if(i2>length-1) i2=length-1; // bounds check
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
			ws+=wave[wi];
			wsamples++;
		}
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
		if(fabs(spd-1.0f)<0.03f) // snap to original speed
		{
			spd=1.0f;
			ft=-1.0f;
		}
		int isize=params->size[waveid];
		float fsize=(float)isize;

		if(vol==0.0f || isize==0)
		{
			active=false;
			return 0;
		}

		params->played[waveid]=0;		
		float *wave=params->wavedata[waveid];
		float finalvol=vol*volume;

		// optimized paths
		if(spd==1.0f) // fastest, just plain data copy and volume scaling
		{
			int ipos=(int)t;
			int numsamples=(isize-1)-ipos;
			if(numsamples>size) // don't go beyond requested buffersize
				numsamples=size;
			else
				active=false; // will render the last sample within this buffer
			for(int si=0;si<numsamples;si++)
				buffer->left[si]+=wave[ipos++]*finalvol;
			tick+=numsamples;
			t=(float)ipos;
		}
		else // time scaling
		{
			if(ft==-1.0f)
				ft=t/fsize;
			int numsamples=(isize-1)-(int)t;
			if(numsamples>size) // don't go beyond requested buffersize
				numsamples=size;
			else
				active=false; // will render the last sample within this buffer
			float stepsize=spd/fsize;
			for(int si=0;si<numsamples;si++)
			{
//				t+=spd;
				pft=ft;
				ft+=stepsize;
//				ft=t/fsize; // this could be removed with a precalced step size and adjustment of t after the loop
				buffer->left[si]+=WaveformSample(pft, ft, wave, isize)*finalvol;
			}
			t+=spd*numsamples;
			tick+=numsamples;
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

	float* LoadWave(char *filename, int& slength)
	{
		FILE *file=fopen(filename, "rb");
		if(!file)
		{
			LogPrint("swave: File not found \"%s\"", filename);
			return NULL;
		}
	
		int channels=0;
		int samplebits=0;
		int samplerate=0;
		
		char string[256];
		int word=0;
		int dword=0;
		int chunksize=0;
		
		fread(string, 4, 1, file); // "RIFF"
		fread(&dword, 1, 4, file); // remaining file size
		fread(string, 4, 1, file); // "WAVE"
	
		fread(string, 4, 1, file); // "fmt "
		fread(&chunksize, 1, 4, file); // chunk size
		fread(&word, 1, 2, file); // compression code
		if(word!=1)
		{
			LogPrint("swave: Unsupported compression format (%i)", word);
			return NULL; // not PCM
		}
		fread(&channels, 1, 2, file); // channels
		fread(&samplerate, 1, 4, file); // sample rate
		fread(&dword, 1, 4, file); // bytes/sec (ignore)
		fread(&word, 1, 2, file); // block align (ignore)
		fread(&samplebits, 1, 2, file); // bits per sample
		for(int i=0;i<chunksize-16;i+=2) // extra format bytes
			fread(&word, 1, 2, file);
	
		fread(string, 4, 1, file); // "data"
	//	string[4]='\0';
	//	LogPrint("\"%s\"", string);
		fread(&chunksize, 1, 4, file); // chunk size
	//	LogPrint("Chunksize=%i", chunksize);
	
		// read sample data	
		float peak=1.0f/65535; // not zero...
		int samplesize=samplebits/8*channels;
		int numsamples=chunksize/samplesize;
	//	LogPrint("Reading %i samples, bits=%i, channels=%i", numsamples, samplebits, channels);
		float *data=(float*)malloc(numsamples*sizeof(float));
		int audiostart=-1;
		int audioend=0;
		for(int i=0;i<numsamples;i++)
		{
			float value=0.0f;
			if(samplebits==8) // 8 bit unsigned
			{
				unsigned char byte=0;
				fread(&byte, 1, 1, file);
				if(channels==2)
				{
					int a=(int)byte;
					fread(&byte, 1, 1, file);
					byte=(unsigned char)((a+byte)/2);
				}
				value=(float)(byte-128)/0x80;
			}
			if(samplebits==16) // 16 bit signed
			{
				short word=0;
				fread(&word, 1, 2, file);
				if(channels==2)
				{
					int a=(int)word;
					fread(&word, 1, 2, file);
					word=(short)((a+word)/2);
				}
				value=(float)word/0x8000;
			}
			if(value!=0.0f)
			{
				audioend=i+1;
				if(audiostart==-1)
					audiostart=i;
			}
			if(audiostart!=-1)
				data[i-audiostart]=value;
			if(fabs(value)>peak)
				peak=fabs(value);
		}
		LogPrint("swave: audiostart=%i, audioend=%i", audiostart, audioend);
		LogPrint("swave: silence removal (%i -> %i)", numsamples, audioend-audiostart);
		numsamples=audioend-audiostart;

		float *finaldata;	
		if(samplerate!=44100) // resampling needed
		{
			int length=numsamples*44100/samplerate;
			LogPrint("swave: Resampling from %i (%i -> %i)", samplerate, numsamples, length);
			finaldata=(float*)malloc(length*sizeof(float));
			slength=length;
			for(int i=0;i<length;i++)
			{
				float fpos=(float)i/(length-1)*(numsamples-1);
				int index=(int)(fpos-0.5f);
				float f=fpos-(float)index;
				if(index<0) index=0;
				if(index>numsamples-2) index=numsamples-2;
				float s1=data[index];
				float s2=data[index+1];
				float value=s1*(1.0f-f)+s2*f;
				finaldata[i]=value;
			}
			free(data);
		}
		else // matching samplerate
		{
			finaldata=data;
			slength=numsamples;
		}
	
		// normalize & quantize
		for(int i=0;i<slength;i++)
		{
			finaldata[i]*=0.99f/peak;
//			finaldata[i]=(float)((char)(finaldata[i]*127))/127;
		}
	
	//	for(int i=0;i<2000;i+=100)
	//		LogPrint("%.2f", samples[id].data[i]);
	
		LogPrint("swave: Loaded wave \"%s\": length=%i, %i bit, %i Hz, %i channels", filename, slength, samplebits, samplerate, channels);
		
		fclose(file);
		
		return finaldata;
	};
	
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

	void DoInterface(DUI *dui)
	{
		dui->DrawBar(1, 0, 400, 5, dui->palette[4]);
		dui->DrawBar(1, 45, 400, 5, dui->palette[4]);
		dui->DrawBar(1, 5, 400, 40, dui->palette[7]);
		dui->DrawBox(0, 1, 400, 49, dui->palette[6]);

		dui->DrawText(362, 36, dui->palette[6], "swave");
		dui->DrawText(362, 35, dui->palette[9], "swave");

		char string[16];
		for(int i=0;i<12;i++)
		{
			int x=i*30;
			dui->DrawText(26+x, 26, dui->palette[6], "%i", i+1);
			sprintf(string, "knob_vol%i", i);
			dui->DoKnob(10+x, 2, params.vol[i], -1, string, "volume");
			dui->DrawLED(3+x, 2, 1, params.size[i]>0);
			dui->DrawLED(3+x, 10, 0, params.played[i]<5);
			// kind of nasty to have the same calc here as in render()... but I don't want to change the parameter struct
			bool originalspeed=false;
			float spd=0.2f+pow(params.spd[i]*2.0f, 2.0f)*0.8f;
			if(fabs(spd-1.0f)<0.02f) // snap to original speed
				originalspeed=true;
			dui->DrawLED(3+x, 18, 0, !originalspeed);
			//
			params.played[i]++;
			sprintf(string, "knob_spd%i", i);
			dui->DoKnob(10+x, 18, params.spd[i], -1, string, "playback speed (up is original speed)");
			sprintf(string, "btn_load%i", i);
			if(dui->DoButton(8+x, 35, string, "load .wav sample"))
			{
				StopChannels();
				if(params.size[i]>0)
				{
					free(params.wavedata[i]);
					params.size[i]=0;
					params.vol[i]=0.0f;
					params.spd[i]=0.5f;
				}
				char filename[256];
				if(dui->SelectFileLoad(filename, 3))
				{
					params.wavedata[i]=LoadWave(filename, params.size[i]);
					params.vol[i]=1.0f;
				}
			}
		}

/*		// Poll joystick for button triggers
		for(int si=0;si<12;si++)
		{
			if(dui->joyclick[si])
			{
				Trigger(si, 1.0f);
//				LogPrint("swave: joybutton trigger %i!", si);
//				for(int i=0;i<numchannels;i++)
//				{
//					if(!((swave_channel*)channels[i])->active)
//					{
//						channels[i]->Trigger(si, 1.0f);
//						LogPrint("swave: triggering channel %i", i);
//						break;
//					}
//				}
			}
		}*/
	};

	bool Save(FILE *file)
	{
		kfwrite2(name, 64, 1, file);
		int numbytes=sizeof(swave_params)+64;
		for(int i=0;i<12;i++)
			numbytes+=params.size[i]*sizeof(char);
		kfwrite2(&numbytes, 1, sizeof(int), file);
		kfwrite2(visname, 64, 1, file);
		kfwrite(&params, 1, sizeof(swave_params), file);
		for(int i=0;i<12;i++)
			for(int wi=0;wi<params.size[i];wi++)
			{
//				char sample=(char)(params.wavedata[i][wi]*127);
//				kfwrite2(&sample, 1, sizeof(char), file);
				short sample=(short)(params.wavedata[i][wi]*32767);
				kfwrite2(&sample, 1, sizeof(short), file);
			}
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

