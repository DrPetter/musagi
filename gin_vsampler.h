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

	float* LoadWave(char *filename, int& slength)
	{
		FILE *file=fopen(filename, "rb");
		if(!file)
		{
			LogPrint("vsampler: File not found \"%s\"", filename);
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
			LogPrint("vsampler: Unsupported compression format (%i)", word);
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
		LogPrint("vsampler: audiostart=%i, audioend=%i", audiostart, audioend);
		LogPrint("vsampler: silence removal (%i -> %i)", numsamples, audioend-audiostart);
		numsamples=audioend-audiostart;

		float *finaldata;	
		if(samplerate!=44100) // resampling needed
		{
			int length=numsamples*44100/samplerate;
			LogPrint("vsampler: Resampling from %i (%i -> %i)", samplerate, numsamples, length);
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
			finaldata[i]=(float)((char)(finaldata[i]*127))/127;
		}
	
	//	for(int i=0;i<2000;i+=100)
	//		LogPrint("%.2f", samples[id].data[i]);
	
		LogPrint("vsampler: Loaded wave \"%s\": length=%i, %i bit, %i Hz, %i channels", filename, slength, samplebits, samplerate, channels);
		
		fclose(file);
		
		return finaldata;
	};
	
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

	void DoInterface(DUI *dui)
	{
		dui->DrawBar(1, 0, 400, 5, dui->palette[4]);
		dui->DrawBar(1, 45, 400, 5, dui->palette[4]);
		dui->DrawBar(1, 5, 400, 40, dui->palette[7]);
		dui->DrawBox(0, 1, 400, 49, dui->palette[6]);

		dui->DrawText(342, 36, dui->palette[6], "vsampler");
		dui->DrawText(342, 35, dui->palette[9], "vsampler");

		char string[16];
//		sprintf(string, "base note: %i", params.basenote);
		sprintf(string, "base note: %s", GetNoteStr(params.basenote));
		dui->DrawText(65, 20, dui->palette[6], string);
		if(dui->DoButton(40, 12, "btn_nup", "increase base note") && params.basenote<12*5)
			params.basenote++;
		if(dui->DoButton(40, 24, "btn_ndown", "decrease base note") && params.basenote>0)
			params.basenote--;
		dui->DoKnob(14, 5, params.vol, -1, "knob_vol", "volume");
//		dui->DrawLED(5, 2, 1, params.size[i]>0);
		dui->DrawLED(5, 14, 0, params.played<5);
		params.played++;
		dui->DrawText(32, 38, dui->palette[6], "load sample");
		if(dui->DoButton(8, 35, "btn_load", "load .wav sample"))
		{
			StopChannels();
			char filename[256];
			if(dui->SelectFileLoad(filename, 3))
			{
				if(params.size>0)
				{
					free(params.wavedata);
					params.size=0;
				}
				params.wavedata=LoadWave(filename, params.size);
				params.vol=1.0f;
			}
		}
		dui->DrawText(120+32, 38, dui->palette[6], "mono");
		dui->DrawLED(120, 35, 0, numchannels==1);
		if(dui->DoButton(120+8, 35, "btn_mono", "toggle polyphony"))
		{
			ReleaseAll();
			params.mono=!params.mono;
			if(params.mono)
				numchannels=1;
			else
				numchannels=16;
		}
	};

	bool Save(FILE *file)
	{
		kfwrite2(name, 64, 1, file);
		int numbytes=sizeof(vsampler_params)+64;
		for(int i=0;i<12;i++)
			numbytes+=params.size*sizeof(char);
		kfwrite2(&numbytes, 1, sizeof(int), file);
		kfwrite2(visname, 64, 1, file);
		kfwrite(&params, 1, sizeof(vsampler_params), file);
//		for(int i=0;i<12;i++)
			for(int wi=0;wi<params.size;wi++)
			{
				char sample=(char)(params.wavedata[wi]*127);
				kfwrite2(&sample, 1, sizeof(char), file);
			}
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

