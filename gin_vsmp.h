#ifndef gin_vsmp_h
#define gin_vsmp_h

#include "musagi.h"
#include "gear_instrument.h"

struct vsmppattern
{
	int offset[32];
	int end;
	int pos;
};

struct vsmp_params
{
	vsmppattern pattern[1];
	float vol[1];
//	float duty[1];
//	float detune[1];
//	float vislen;
//	int vislenupdated;
	
	float envelope[2][256];
	float envmid[2];
	float envloop[2];
	float envpos[2];

	float speed;
	bool loop;

	float slide_delay;
	float slide_speed;
	float vibrato_depth;
	float vibrato_speed;

	float envspd_attack;
	float envspd_decay;

	float patscrolly;
	
	bool mono;
	float nslide;
	
	bool dpcm;
	bool filter;
	int mode;

	float wavepos;
	float loop_start;
	float loop_end;
	int basenote;
	float fbasenote;
	bool waveloop;
	int size;
	float *wavedata;
};

class vsmp_channel : gear_ichannel
{
public:
	// parameters
	vsmp_params *params;

	// internal data
	struct vsmpchannel
	{
		float period;
	//	int note;
		float noteperiod;
		bool playing;
		bool duestop;
//		int t;
		float t;
		float ft;
		float pft;
		float duty;
		int st;
		float envpos;
		float denvpos;
		bool decay;
		int iwlen;
		int patpos;
	};
	vsmpchannel channels[1];
	
	float scale[49];
	
	float volume;
	int irval;
	int tick;
	int pt;
	float psample;
	
	int numslide;
	double dslide;
	float vibphase;
	
	int dpcmlevel;
	float dpcmphase;
	
	float fy;
	float fdy;
	int fltout;
	bool dead;

	// auto-arpeggio (from polyphony on mono instrument)
	int numarpnotes;
	int autoarp[8];
	int autoarp_delay;

	bool mono_released;

	vsmp_channel()
	{
		params=NULL;

		tick=0;
		volume=0.0f;
		irval=0;

		fy=0.0f;
		fdy=0.0f;
		dslide=0.0f;
		vibphase=0.0f;
		psample=0.0f;
		
		for(int i=0;i<1;i++)
		{
			channels[i].playing=false;
			channels[i].duty=0.5f;
			channels[i].pft=0.0f;
			channels[i].ft=0.0f;
			channels[i].iwlen=170;
		}

		float factor=1.0f;
		scale[24]=1.0f;
		for(int i=1;i<=24;i++)
		{
			factor*=98.0f/92.5f;
			scale[24-i]=factor;
			scale[24+i]=1.0f/factor;
		}
		
		numarpnotes=0;
		for(int i=0;i<8;i++)
			autoarp[i]=-1;
		autoarp_delay=0;

		mono_released=true;
	};

	~vsmp_channel()
	{
	};

/*	float SquareSample(float t1, float t2, float duty)
	{
		if(t1<duty && t2<duty) // both sample points in first duty
			return -1.0f;
		if(t1>duty && t2>duty) // both sample points in second duty
			return 1.0f;
		bool inverted=false;
		if(t2<t1) // wrapped
		{
			if(duty>t1) // short spike between t1 and t2
			{
				float n1=duty-t1;
				float p=1.0f-duty;
				float n2=t2;
				return p/(n1+p+n2)*2.0f-1.0f;
			}
			t2+=1.0f;
			duty=1.0f;
			inverted=true;
		}
		float p1=duty-t1;
		if(p1<0.0f) p1=0.0f;
		float p2=t2-duty;
		if(p2<0.0f) p2=0.0f;
		if(!inverted)
			return p2/(p2+p1)*2.0f-1.0f;
		else
			return p1/(p2+p1)*2.0f-1.0f;
	};
*/
	float WaveformSample(float t1, float t2, float *wave, int length, float tls, float tle)
	{
		if(t2<t1) // wrapped
			t2+=tle-tls;
				
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
		int ile=(int)(tle*length);
		if(ile>length)
			ile=length;
		int ils=(int)(tls*length);
		int ild=ile-ils;
		float ws=0.0f;
		int wsamples=0;
		for(int i=i1+1;i<i2;i++) // add all intermediate whole sample values
		{
			int wi=i;
			if(wi>=ile)
				wi-=ild;
			ws+=wave[wi];
			wsamples++;
		}
		if(i2>=ile) i2-=ild;
		
		if(i1>length-1) i1=length-1;
		if(i2>length-1) i2=length-1;

		if(wsamples>0)
			return (ws+wave[i1]*p1+wave[i2]*p2)/((float)wsamples+p1+p2);
		else
			return (wave[i1]*p1+wave[i2]*p2)/(p1+p2);
	};

	int RenderBuffer(StereoBufferP *buffer, int size)
	{
		if(!active)
			return 0; // no output
			
		double slidespd=1.0-pow(params->slide_speed*2.0f-1.0f, 3.0f)*0.001;
		if(fabs(slidespd-1.0)<0.000002)
			slidespd=1.0;
		float envspd_a=pow(params->envspd_attack, 2.0f)*0.0005f;
		float envspd_d=pow(params->envspd_decay, 2.0f)*0.0005f;
		int islidetrig=(int)(pow(params->slide_delay, 2.0f)*80000.0f);
//		int iptperiod=(int)(pow(1.1f-params->speed, 2.0f)*44100.0f);
		int iptperiod=(int)(pow(0.9f-params->speed*0.65f, 3.0f)*22050.0f);
		float vibspeed=pow(params->vibrato_speed+0.2f, 2.0f)*0.003f;
		float vibdep=pow(params->vibrato_depth, 3.0f);

		for(int si=0;si<size;si++)
		{
			if(params->size==0) // no sample data
			{
				active=false;
				break;
			}
				
			int iperiod[1];
//			float ft[4];
			bool ptrig=false;
			pt++;
			if(pt>=iptperiod)
			{
				pt=0;
				ptrig=true;
			}
//			int deadch=0;
			float volenv[1];
			float dutenv[7];
			if(numslide>0)
				numslide--;

			if(numarpnotes>0)
				autoarp_delay++; // time passed since trigger, only arpeggiate on near-simultaneous notes

			if(dead)
			{
				fltout++;
				if(fltout>500)
				{
					active=false;
					break;
				}
			}
			else
				fltout=0;

			if(numslide>0)
			{
				channels[0].noteperiod=(float)((double)channels[0].noteperiod*dslide);
				channels[0].period=(float)((double)channels[0].period*dslide);
			}
			if(tick>=islidetrig)
			{
				if(vibdep>0.0f)
				{
					vibphase+=vibspeed;
					if(vibphase>=2*PI)
						vibphase-=2*PI;
//					slidespd+=sin(vibphase)*(double)vibdep*0.01*vibspeed;
					slidespd+=cos(vibphase)*(double)vibdep*0.01*vibspeed;
				}

				channels[0].noteperiod=(float)((double)channels[0].noteperiod*slidespd);
				channels[0].period=(float)((double)channels[0].period*slidespd);

				if(channels[0].period<0.01f) // max 100x recorded speed
					channels[0].period=0.01f;
				if(channels[0].period>100.0f) // min 1/100x recorded speed
					channels[0].period=100.0f;
				if(channels[0].noteperiod<0.01f)
					channels[0].noteperiod=0.01f;
				if(channels[0].noteperiod>100.0f)
					channels[0].noteperiod=100.0f;
			}
			
			if(ptrig)
			{
				channels[0].patpos++;
				if(numarpnotes>1) // auto-arpeggio
				{
					if(channels[0].patpos>=numarpnotes)
					{
						if(params->loop)
							channels[0].patpos=0; // loop
						else
							channels[0].patpos--; // hold at the end position
					}
//					if(!channels[0].playing)
//						channels[0].t=0.0f; // reset phase if we're coming from silence
					if(channels[0].patpos>=0)
					{
						channels[0].playing=true;
						channels[0].period=GetNoteFrequency(params->basenote)/GetNoteFrequency(autoarp[channels[0].patpos]);
//						channels[0].period=44100.0f/GetNoteFrequency(autoarp[channels[0].patpos]);
					}
				}
				else
				{
					if(channels[0].patpos>params->pattern[0].end)
					{
						if(params->loop)
							channels[0].patpos=0; // loop
						else
							channels[0].patpos--; // hold at the end position
					}
//					if(!channels[0].playing)
//						channels[0].t=0.0f; // reset phase if we're coming from silence
					int offset=params->pattern[0].offset[channels[0].patpos];
					if(offset==999)
					{
						if(channels[0].playing)
							channels[0].duestop=true;
					}
					else
					{
						channels[0].playing=true;
						channels[0].period=channels[0].noteperiod*scale[offset+24];
					}
					params->pattern[0].pos=channels[0].patpos;
				}
			}

			// envelope
			if(!channels[0].decay)
			{
				channels[0].envpos+=envspd_a;
				if(channels[0].envpos>params->envmid[0])
					channels[0].envpos-=params->envmid[0]-params->envloop[0];
				channels[0].denvpos+=envspd_a;
				if(channels[0].denvpos>params->envmid[1])
					channels[0].denvpos-=params->envmid[1]-params->envloop[1];
			}
			else
			{
				channels[0].envpos+=envspd_d;
				if(channels[0].envpos>=1.0f)
				{
					channels[0].envpos=1.0f;
					channels[0].playing=false;
					dead=true;
//					deadch++;
				}
				channels[0].denvpos+=envspd_d;
				if(channels[0].denvpos>=1.0f)
					channels[0].denvpos=1.0f;
			}
			params->envpos[0]=channels[0].envpos; // for visualization
			params->envpos[1]=channels[0].denvpos; // for visualization
			if(channels[0].envpos<1.0f)
			{
				int index=(int)(channels[0].envpos*170);
				if(index>169)
					index=169;
				volenv[0]=params->envelope[0][index];
			}
			else
				volenv[0]=0.0f;
			// duty envelope
/*			int index=(int)(channels[0].denvpos*170);
			if(index>169)
				index=169;
			dutenv[0]=1.0f-(1.0f-params->envelope[1][index])*(1.0f-params->duty[0]);
*/
//			channels[0].t+=1.0f/channels[0].period;
			channels[0].t+=1.0f/channels[0].period/params->size;
//			channels[0].t+=1.0f/speed;

			if(channels[0].t>=params->loop_end)
			{
				if(params->waveloop)
					channels[0].t-=params->loop_end-params->loop_start;
				else
				{
//				{
					channels[0].t=params->loop_end;
					channels[0].playing=false;
					channels[0].decay=true;
//					active=false;
//					break;
//				}
				}
			}

//			if(deadch==1)
//				active=false;

			int isize=params->size;
			float fsize=(float)isize;
			channels[0].pft=channels[0].ft;
//			channels[0].ft=channels[0].t/fsize;
			channels[0].ft=channels[0].t;

			float sample=0.0f;
			float wavesample=WaveformSample(channels[0].pft, channels[0].ft, params->wavedata, isize, params->loop_start, params->loop_end);

			if((wavesample<0.0f && psample>0.0f) || (wavesample>0.0f && psample<0.0f)) // zero-crossing, latch variables and maybe do note end
			{
				if(channels[0].duestop)
				{
					channels[0].playing=false;
					channels[0].duestop=false;
				}
				// latch stuff
//				channels[0].duty=dutenv[0];
//				channels[0].iwlen=170-(int)(dutenv[0]*170);
//				if(channels[0].iwlen<2)
//					channels[0].iwlen=2;
//				params->vislen=dutenv[0];
//				params->vislenupdated=0;
			}
			psample=wavesample;

			if(channels[0].playing && params->vol[0]>0.0f)
			{
				if(!params->dpcm)
					sample+=wavesample*volenv[0]*params->vol[0];
				else
				{
					float curlevel=(float)dpcmlevel/16;
					dpcmphase+=103.412543f/channels[0].period;
					if(dpcmphase>=1.0f)
					{
						dpcmphase-=1.0f;
						float target=wavesample*volenv[0]*params->vol[0];
						if(target>curlevel)
							dpcmlevel++;
						else
							dpcmlevel--;
						if(dpcmlevel>15) dpcmlevel=15;
						if(dpcmlevel<-16) dpcmlevel=-16;
						curlevel=(float)dpcmlevel/16;
					}
					sample+=curlevel;
				}
				params->wavepos=channels[0].ft;
//				params->envpos[2]=(float)(170-channels[0].iwlen)/170.0f+channels[0].ft*(float)channels[0].iwlen/170.0f;
			}

			if(params->filter)
			{
//				fdy+=(sample-(fy+fdy*6))*(float)channels[0].iwlen*0.5f/channels[0].period;
//				fdy+=(sample-(fy+fdy*6))*50/channels[0].period;
				if(fy>2.0f) fy=2.0f; // ?
				if(fy<-2.0f) fy=-2.0f; // ?
				fdy+=(sample-(fy+fdy*3))*0.05f;
				if(fdy>0.5f) fy=0.5f;
				if(fdy<-0.5f) fdy=-0.5f;
				fy+=fdy;
				sample=fy;
			}
			else
			{
				fy=sample;
				fdy=0.0f;
			}

			// noise
//			if(channels[3].playing && params->vol[3]>0.0f)
//				sample+=WaveformSample(channels[3].pft, channels[3].ft, noisewave, 16)*volenv[3]*params->vol[3];

			buffer->left[si]+=sample*volume;
			tick++;
		}

		return 1; // produced mono output (left channel)
	};

	void Trigger(int tnote, float tvolume)
	{
		active=true;
		volume=tvolume;

		float newperiod=GetNoteFrequency(params->basenote)/GetNoteFrequency(tnote);

		int nch=0;
		for(nch=0;nch<1;nch++)
			if(params->vol[nch]>0.0f)
				break;
		if(params->mono && params->nslide>0.01f && nch<4 && channels[nch].noteperiod>0.0f && !mono_released)
		{
			numslide=(int)(params->nslide*20000.0f);
			dslide=1.0+log(newperiod/channels[nch].noteperiod)/(double)numslide;
			numslide+=1;
		}
		else
		{
			dslide=1.0;
			numslide=0;
		}

		for(int i=0;i<1;i++)
		{
			if(numslide==0)
				channels[i].noteperiod=newperiod;
//				channels[i].noteperiod=44100.0f/GetNoteFrequency(tnote);
			if(numslide==0 || channels[i].decay)
			{
				if(channels[i].decay) // don't reset wave phase if we're interrupting a previous note (basically hack to allow smooth slide transitions)
				{
					channels[i].t=0.0f;
					channels[i].ft=0.0f;
					channels[i].pft=0.0f;
				}
				channels[i].st=0;
				channels[i].envpos=0.0f;
				channels[i].denvpos=0.0f;
				channels[i].decay=false;
				channels[i].duestop=false;
				channels[i].patpos=-1;
			}
			channels[i].duestop=false;
		}
		vibphase=0.0f;
		dpcmlevel=31; // start high to get popping noise...
		dpcmphase=0.0f;
		fltout=0;
//		fy=0.0f;
//		fdy=0.0f;
		pt=100000;
		tick=0;
		dead=false;

//		speed=newspeed;

		if(params->mono)
		{
			if(autoarp_delay<100) // build auto-arpeggio
			{
				autoarp[numarpnotes]=tnote;
				numarpnotes++;
				for(int i=1;i<numarpnotes;i++) // sort notes in ascending order
					if(autoarp[i]<autoarp[i-1])
					{
						int t=autoarp[i];
						autoarp[i]=autoarp[i-1];
						autoarp[i-1]=t;
						i=0;
					}
			}
			else // new trigger while playing auto-arpeggio - kill arpeggio
			{
				numarpnotes=0;
				autoarp_delay=0;

				autoarp[numarpnotes]=tnote;
				numarpnotes++;
			}
		}

		mono_released=false;
	};
	
	void Release()
	{
		for(int i=0;i<1;i++)
			channels[i].decay=true;

		numarpnotes=0;
		autoarp_delay=0;

		mono_released=true; // for autoslide
	};

	void Slide(int tnote, int length)
	{
		if(!active)
			return;

		int nch=0;
		for(nch=0;nch<1;nch++)
			if(params->vol[nch]>0.0f)
				break;
		if(channels[nch].noteperiod<=0.0f)
			return;

		numslide=length;
		float newperiod=GetNoteFrequency(params->basenote)/GetNoteFrequency(tnote);
		dslide=1.0+log(newperiod/channels[nch].noteperiod)/(double)numslide;
		numslide+=1;
	};
};

class gin_vsmp : public gear_instrument
{
private:
	vsmp_params params;
	int pmx;
	bool envline_active;
	int envline_x1, envline_y1, envline_x2, envline_y2;

	float* LoadWave(char *filename, int& slength)
	{
		FILE *file=fopen(filename, "rb");
		if(!file)
		{
			LogPrint("vsmp: File not found \"%s\"", filename);
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
			LogPrint("vsmp: Unsupported compression format (%i)", word);
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
		float peak=1.0f/65535.0f; // not zero...
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
//		LogPrint("vsmp: audiostart=%i, audioend=%i", audiostart, audioend);
//		LogPrint("vsmp: silence removal (%i -> %i)", numsamples, audioend-audiostart);
		numsamples=audioend-audiostart;

		float *finaldata;	
		if(samplerate!=44100) // resampling needed
		{
			int length=numsamples*44100/samplerate;
//			LogPrint("vsmp: Resampling from %i (%i -> %i)", samplerate, numsamples, length);
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
	
		// normalize
		for(int i=0;i<slength;i++)
		{
			finaldata[i]*=0.99f/peak;
//			finaldata[i]=(float)((char)(finaldata[i]*127))/127;
		}
	
	//	for(int i=0;i<2000;i+=100)
	//		LogPrint("%.2f", samples[id].data[i]);
	
//		LogPrint("vsmp: Loaded wave \"%s\": length=%i, %i bit, %i Hz, %i channels", filename, slength, samplebits, samplerate, channels);
		
		fclose(file);
		
		return finaldata;
	};
	
public:
	gin_vsmp()
	{
//		LogPrint("gin_vsmp: init");

		strcpy(name, "vsmp");
		strcpy(visname, name);

		pmx=0;
		envline_active=false;

		params.mode=7;
		for(int pi=0;pi<32;pi++)
			params.pattern[0].offset[pi]=999;
		params.pattern[0].offset[0]=0;
		params.pattern[0].offset[1]=0;
		params.pattern[0].offset[2]=0;
		params.pattern[0].end=2;
		params.pattern[0].pos=-1;
		params.vol[0]=0.0f;
//		params.duty[0]=0.0f;
//		params.detune[0]=0.5f;
		for(int i=0;i<2;i++)
		{
			for(int ei=0;ei<256;ei++)
			{
				if(i==0)
				{
					if(ei<32)
						params.envelope[i][ei]=1.0f;
					else if(ei<=170)
						params.envelope[i][ei]=1.0f-(float)(ei-32)/138;
					else
						params.envelope[i][ei]=0.0f;
				}
//				if(i==1)
//					params.envelope[i][ei]=0.0f;
/*				if(i==2)
				{
					params.envelope[i][ei]=1.0f;
					if(ei>230)
						params.envelope[i][ei]=0.0f;
					if(ei>240)
						params.envelope[i][ei]=1.0f;
					if(ei>245)
						params.envelope[i][ei]=-1.0f;
				}*/
			}
			params.envmid[i]=0.1f;
			params.envloop[i]=0.0f;
			params.envpos[i]=0.0f;
		}
		params.vol[0]=1.0f;
//		params.vislen=0.0f;

		params.speed=0.75f;
		params.loop=true;
	
		params.slide_delay=0.0f;
		params.slide_speed=0.5f;
		params.vibrato_depth=0.0f;
		params.vibrato_speed=0.35f;

		params.envspd_attack=1.0f;
		params.envspd_decay=1.0f;
		
		params.patscrolly=0.5f;

		params.mono=false;
		params.nslide=0.0f;
		params.dpcm=false;
		params.filter=false;

		params.waveloop=false;
		params.wavepos=0.0f;		
		params.size=0;
		params.loop_start=0.0f;
		params.loop_end=0.0f;
		params.basenote=12*3;
		params.fbasenote=(float)(params.basenote-12*1)/(12*7);

		numchannels=16;
		for(int i=0;i<numchannels;i++)
		{
			channels[i]=(gear_ichannel*)(new vsmp_channel());
			((vsmp_channel*)channels[i])->params=&params;
		}
	};

	~gin_vsmp()
	{
		if(params.size>0)
			free(params.wavedata);
	};

	void DoInterface(DUI *dui)
	{
		dui->DrawBar(0, 0, 400, 50, dui->palette[4]);
		dui->DrawBar(2, 30, 393, 17, dui->palette[6]);
		dui->DrawBox(2, 30, 393, 17, dui->palette[8]);
//		dui->DrawBox(20, 30, 45, 10, dui->palette[8]);
//		dui->DrawBar(21, 29, 43, 10, dui->palette[4]);
		dui->DrawBox(20, 30, 60, 10, dui->palette[8]);
		dui->DrawBar(21, 29, 58, 10, dui->palette[4]);
		dui->DrawBox(87, 30, 67, 10, dui->palette[8]);
		dui->DrawBar(88, 29, 65, 10, dui->palette[4]);
		dui->DrawBox(0, 1, 400, 49, dui->palette[6]);

		dui->DrawText(369, 40, dui->palette[6], "vsmp");
		dui->DrawText(371, 40, dui->palette[6], "vsmp");
		dui->DrawText(370, 40, dui->palette[9], "vsmp");

//		dui->DrawText(3, 33, dui->palette[6], "vol");
//		dui->DoKnob(2, 12, params.vol[0], -1, "knob_vol1", "volume");
		dui->DrawText(3, 20, dui->palette[6], "vol");
		dui->DoKnob(3, 30, params.vol[0], -1, "knob_vol1", "volume");

//		dui->DoKnob(158, 12, params.fbasenote, -1, "knob_nbase", "base note of the recorded sample (remember shift for knob precision)");
//		params.basenote=(int)(params.fbasenote*12*7+12*1);
//		dui->DrawText(166, 33, dui->palette[9], GetNoteStr(params.basenote));
		dui->DoKnob(155, 30, params.fbasenote, -1, "knob_nbase", "base note of the recorded sample (remember shift for knob precision)");
		params.basenote=(int)(params.fbasenote*12*7+12*1);
//		params.basenote=12*4;
		dui->DrawText(172, 33, dui->palette[9], GetNoteStr(params.basenote));

//		dui->DrawText(68, 5, dui->palette[6], "offset");
//		dui->DoKnob(50, 2, params.duty[0], -1, "knob_dut1", "waveform offset");

		dui->DrawLED(162, 2, 0, params.mode==0);
		if(dui->DoButton(170, 2, "btn_params.mode1", "arpeggio"))
			params.mode=0;

		dui->DrawLED(127, 2, 0, params.mode==5);
		if(dui->DoButton(135, 2, "btn_params.mode6", "volume envelope"))
			params.mode=5;
//		dui->DrawLED(127, 2+12, 0, params.mode==6);
//		if(dui->DoButton(135, 2+12, "btn_params.mode7", "offset envelope"))
//			params.mode=6;

		dui->DrawLED(127, 2+12, 0, params.mode==7);
		if(dui->DoButton(135, 2+12, "btn_params.mode8", "waveform view"))
			params.mode=7;

		dui->DrawText(123, 2+25, dui->palette[6], "DPCM");
		dui->DrawLED(92, 2+24, 1, params.dpcm);
		if(dui->DoButton(100, 2+24, "btn_dpcm", "tries to mimic the delta-pcm channel of the NES"))
			params.dpcm=!params.dpcm;

		dui->DrawText(56, 2+25, dui->palette[6], "FLT");
		dui->DrawLED(25, 2+24, 1, params.filter);
		if(dui->DoButton(33, 2+24, "btn_flt", "filter waveform"))
			params.filter=!params.filter;

		dui->DrawText(56, 2+13, dui->palette[6], "loop");
		dui->DrawLED(25, 2+12, 0, params.waveloop);
		if(dui->DoButton(33, 2+12, "btn_wloop", "toggle wave loop"))
			params.waveloop=!params.waveloop;

		dui->DrawText(56, 3, dui->palette[6], "load wav");
		if(dui->DoButton(33, 2, "btn_load", "load .wav sample"))
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
//				params.loop_end=(float)params.size-1;
				params.loop_end=1.0f;
				params.loop_start=0.0f;
//				if(params.loop_end<params.loop_start)
//					params.loop_end=params.loop_start;
//				params.vol=1.0f;
			}
		}

		dui->DrawLED(367, 2, 0, params.mode==4);
		if(dui->DoButton(377, 2, "btn_params.mode5", "general options"))
			params.mode=4;

		dui->DrawBox(192, 1, 172, 49, dui->palette[6]);

		if(params.mode==4)
		{
			dui->DrawBar(193, 2, 170, 47, dui->palette[14]);

			dui->DrawText(195+16, 5, dui->palette[6], "del");
			dui->DoKnob(195, 1, params.slide_delay, -1, "knob_sd", "delay before slide and vibrato starts");
			dui->DrawText(195+16, 5+16, dui->palette[6], "ss");
			dui->DoKnob(195, 1+16, params.slide_speed, -1, "knob_ss", "slide speed and direction");

			dui->DrawText(230+16, 5, dui->palette[6], "vd");
			dui->DoKnob(230, 1, params.vibrato_depth, -1, "knob_vd", "vibrato depth");
			dui->DrawText(230+16, 5+16, dui->palette[6], "vs");
			dui->DoKnob(230, 1+16, params.vibrato_speed, -1, "knob_vs", "vibrato speed");

			dui->DrawText(305+16, 5+16, dui->palette[6], "attack");
			dui->DoKnob(305, 1+16, params.envspd_attack, -1, "knob_ats", "envelope attack speed");
			dui->DrawText(305+16, 5+32, dui->palette[6], "decay");
			dui->DoKnob(305, 1+32, params.envspd_decay, -1, "knob_dcs", "envelope decay speed");

			dui->DrawText(328+16, 5, dui->palette[6], "sld");
			dui->DoKnob(328, 1, params.nslide, -1, "knob_nslide", "note-to-note slide length");

			dui->DrawText(195+16, 5+32, dui->palette[6], "arp spd");
			dui->DoKnob(195, 1+32, params.speed, -1, "knob_ps", "arpeggio speed");
			dui->DrawText(235+32, 5+22, dui->palette[6], "a.loop");
			dui->DrawLED(265, 5+32, 0, params.loop);
			if(dui->DoButton(265+8, 4+32, "btn_alp", "toggle arpeggio loop"))
				params.loop=!params.loop;

			dui->DrawText(265+30, 5, dui->palette[6], "mono");
			dui->DrawLED(265, 4, 0, numchannels==1);
			if(dui->DoButton(265+8, 4, "btn_mono", "toggle polyphony - select mono to enable note-to-note slide"))
			{
				ReleaseAll();
				params.mono=!params.mono;
				if(params.mono)
					numchannels=1;
				else
					numchannels=16;
			}
		}
		if(params.mode==0)
		{
			int topwiny=dui->gui_winy;
			float dummy=0.0f;
			dui->StartScrollArea(193, 1, 170, 48, dummy, params.patscrolly, 170, 49*3, dui->palette[14], "vsmpscroll");
			{
				for(int i=0;i<160/5;i++)
				{
					int sx=i*5;
					if(params.pattern[params.mode].offset[i]==999)
						dui->DrawBar(sx, 0, 5, 49*3, dui->palette[6]);
				}
				for(int i=0;i<49;i++)
				{
					if(i==24)
						dui->DrawBar(0, i*3, 170, 3, dui->palette[7]);
					else if(abs(i-24)%12==0)
						dui->DrawBar(0, i*3, 170, 3, dui->palette[4]);
					else if(abs(i-24)%3==0)
						dui->DrawBar(0, i*3+1, 170, 1, dui->palette[4]);
				}
				if(params.pattern[params.mode].pos>-1)
					dui->DrawBar(params.pattern[params.mode].pos*5+1, 0, 3, 49*3, dui->palette[4]);
				dui->DrawBar(params.pattern[params.mode].end*5+3, 0, 1, 49*3, dui->palette[9]);
				int mx=(dui->mousex-dui->gui_winx)/5;
				int my=(dui->mousey-dui->gui_winy)/3;
				if(mx>155/5 || dui->mousey<topwiny || dui->mousey>topwiny+48)
					mx=-1;
				for(int i=0;i<160/5;i++)
				{
					int sx=i*5;
					if(params.pattern[params.mode].offset[i]<999)
					{
						int sy=(24-params.pattern[params.mode].offset[i])*3;
						dui->DrawBar(sx, sy, 4, 3, dui->palette[3]);
					}
					if(mx==i)
						dui->DrawBar(sx, my*3, 4, 3, dui->palette[9]);
				}
				if(mx>-1)
				{
					if(dui->lmbclick || dui->rmbclick)
						dui->mouseregion=45;
					if(dui->mouseregion==45)
					{
						if(dui->shift_down && dui->lmbdown) // place end point
							params.pattern[params.mode].end=mx;
						else
						{
							if(dui->lmbdown) // place offset
								params.pattern[params.mode].offset[mx]=24-my;
							if(dui->rmbdown) // delete offset
								params.pattern[params.mode].offset[mx]=999;
						}
					}
				}
			}
			dui->EndScrollArea();
		}
		if(params.mode==5)
		{
			int topwiny=dui->gui_winy;
			float dummy=0.0f;
			dui->StartScrollArea(193, 1, 170, 48, dummy, dummy, 170, 48, dui->palette[14], "vsmpscroll2");
			{
				int ecolor=9;
				if(params.mode==6)
					ecolor=3;
				int eoffset=0;
/*				if(params.mode==7) // vsmp wave
				{
					eoffset=256-170;
					if(params.vislenupdated>5)
						params.vislen=params.duty[0];
					params.vislenupdated++;
					dui->DrawBar((int)(params.vislen*170), 0, 171-(int)(params.vislen*170), 50, dui->palette[15]);
				}
				else
				{*/
					dui->DrawBar((int)(params.envmid[params.mode-5]*170), 0, 2, 50, dui->palette[6]);
					dui->DrawBar((int)(params.envloop[params.mode-5]*170), 0, 2, 50, dui->palette[7]);
//				}
				dui->DrawBar((int)(params.envpos[params.mode-5]*170), 0, 2, 50, dui->palette[9]);
/*				if(params.mode==7)
				{
					for(int i=0;i<170;i++)
						dui->DrawBar(i, 42-(int)((params.envelope[params.mode-5][i+eoffset]*0.5f+0.5f)*38), 1, 2, dui->palette[9]);
				}
				else
				{*/
					for(int i=0;i<170;i++)
						dui->DrawBar(i, 42-(int)(params.envelope[params.mode-5][i+eoffset]*38), 1, 2, dui->palette[ecolor]);
//				}
				int mx=dui->mousex-dui->gui_winx;
				int my=dui->mousey-dui->gui_winy;
				if(mx>=0 && mx<=170 && dui->mousey>=topwiny && dui->mousey<topwiny+48)
				{
					if(dui->lmbclick || dui->rmbclick)
						dui->mouseregion=45;
					if(dui->mouseregion==45)
					{
						if(dui->ctrl_down && dui->lmbdown && !envline_active) // start interpolation line
						{
							envline_active=true;
							envline_x1=mx;
							envline_y1=my;
						}
						if(!dui->shift_down && !dui->ctrl_down && dui->lmbdown) // draw
						{
							int x1=pmx;
							int x2=mx;
							if(x1>x2)
							{
								int tx=x1;
								x1=x2;
								x2=tx;
							}
							if(x1==x2)
								x2++;
							for(int x=x1;x<x2;x++)
							{
								if(x<0 || x>=170)
									continue;
								params.envelope[params.mode-5][x+eoffset]=(float)(42-my)/38;
/*								if(params.mode==7)
								{
									params.envelope[params.mode-5][x+eoffset]=params.envelope[params.mode-5][x+eoffset]*2.0f-1.0f;
									if(params.envelope[params.mode-5][x+eoffset]>1.0f) params.envelope[params.mode-5][x+eoffset]=1.0f;
									if(params.envelope[params.mode-5][x+eoffset]<-1.0f) params.envelope[params.mode-5][x+eoffset]=-1.0f;
								}
								else
								{*/
									if(params.envelope[params.mode-5][x+eoffset]>1.0f) params.envelope[params.mode-5][x+eoffset]=1.0f;
									if(params.envelope[params.mode-5][x+eoffset]<0.0f) params.envelope[params.mode-5][x+eoffset]=0.0f;
//								}
							}
						}
//						if(params.mode!=7) // place midpoint or loop point
//						{
							if(dui->shift_down && dui->lmbdown)
								params.envloop[params.mode-5]=(float)mx/170;
							if(dui->rmbdown)
								params.envmid[params.mode-5]=(float)mx/170;
							if(params.envloop[params.mode-5]>params.envmid[params.mode-5])
								params.envloop[params.mode-5]=params.envmid[params.mode-5];
//						}
					}
				}
				if(envline_active)
				{
					if(dui->ctrl_down && dui->lmbdown) // specify interpolation line
					{
						envline_x2=mx;
						envline_y2=my;
					}
					if(!dui->lmbdown) // apply interpolation line
					{
						int x1=envline_x1;
						int x2=envline_x2;
						float y1=(float)(42-envline_y1)/38;
						float y2=(float)(42-envline_y2)/38;
						if(x1>x2)
						{
							int tx=x1;
							x1=x2;
							x2=tx;
							float ty=y1;
							y1=y2;
							y2=ty;
						}
						if(x1==x2)
							x2++;
						for(int x=x1;x<x2;x++)
						{
							if(x<0 || x>=170)
								continue;
							float f=(float)(x-x1)/(x2-x1);
							params.envelope[params.mode-5][x+eoffset]=y1*(1.0f-f)+y2*f;
/*							if(params.mode==7)
							{
								params.envelope[params.mode-5][x+eoffset]=params.envelope[params.mode-5][x+eoffset]*2.0f-1.0f;
								if(params.envelope[params.mode-5][x+eoffset]>1.0f) params.envelope[params.mode-5][x+eoffset]=1.0f;
								if(params.envelope[params.mode-5][x+eoffset]<-1.0f) params.envelope[params.mode-5][x+eoffset]=-1.0f;
							}
							else
							{*/
								if(params.envelope[params.mode-5][x+eoffset]>1.0f) params.envelope[params.mode-5][x+eoffset]=1.0f;
								if(params.envelope[params.mode-5][x+eoffset]<0.0f) params.envelope[params.mode-5][x+eoffset]=0.0f;
//							}
//							if(params.envelope[params.mode-5][x+eoffset]>1.0f) params.envelope[params.mode-5][x+eoffset]=1.0f;
//							if(params.envelope[params.mode-5][x+eoffset]<0.0f) params.envelope[params.mode-5][x+eoffset]=0.0f;
						}
						envline_active=false;
					}
					dui->DrawLine(envline_x1, envline_y1, envline_x2, envline_y2, 2, dui->palette[11]);
				}
				pmx=mx;
			}
			dui->EndScrollArea();
		}
		if(params.mode==7) // waveform view
		{
			int topwiny=dui->gui_winy;
			float dummy=0.0f;
			dui->StartScrollArea(193, 1, 170, 48, dummy, dummy, 170, 48, dui->palette[14], "vsmpscroll2");
			{
				int ecolor=9;
				if(params.mode==6)
					ecolor=3;
				int eoffset=0;

				dui->DrawBar(0, 0, 171, 50, dui->palette[14]);

				if(params.size>0)
				{
					int px=0;
					for(int x=0;x<170;x++)
					{
						int sx=x*params.size/170;
						if(sx>params.size) sx=params.size;
						float maxv=params.wavedata[px];
						float minv=maxv;
						float penergy=0.0f;
						float nenergy=0.0f;
						int n=1;
						int step=1+params.size/1700; // keep it below 10 steps per pixel
						for(int i=px+1;i<sx;i+=step)
						{
							float v=params.wavedata[i];
							if(v>maxv) maxv=v;
							if(v<minv) minv=v;
							if(v>0.0f)
								penergy+=v;
							else
								nenergy-=v;
							n++;
						}
						px=sx;
						penergy=pow(penergy/n, 0.3f);
						nenergy=pow(nenergy/n, 0.3f);
//						float span=maxv-minv;
						// 4,5,15
						dui->DrawBar(x, 25-(int)(maxv*25), 1, (int)((maxv-minv)*25), dui->palette[4]);
						if(penergy+nenergy>1.0f/30)
							dui->DrawBar(x, 25-(int)(penergy*maxv*25), 1, (int)((nenergy+penergy)*(maxv-minv)*25), dui->palette[5]);
						penergy=pow(penergy, 3.0f);
						nenergy=pow(nenergy, 3.0f);
//						if(penergy+nenergy>1.0f/30)
						int h=(int)((nenergy+penergy)*(maxv-minv)*25);
						if(h<1) h=1;
						dui->DrawBar(x, 25-(int)(penergy*maxv*25), 1, h, dui->palette[7]);
					}

					if(params.waveloop)
					{
						dui->DrawBar((int)(params.loop_start*170)-1, 0, 2, 50, dui->palette[6]);
						dui->DrawBar((int)(params.loop_end*170)-1, 0, 2, 50, dui->palette[10]);
					}
					dui->DrawBar((int)(params.wavepos*170)-1, 0, 2, 50, dui->palette[9]);
				}

/*				if(params.mode==7) // vsmp wave
				{
					eoffset=256-170;
					if(params.vislenupdated>5)
						params.vislen=params.duty[0];
					params.vislenupdated++;
					dui->DrawBar((int)(params.vislen*170), 0, 171-(int)(params.vislen*170), 50, dui->palette[15]);
				}


				dui->DrawBar((int)(params.envmid[params.mode-5]*170), 0, 2, 50, dui->palette[6]);
				dui->DrawBar((int)(params.envloop[params.mode-5]*170), 0, 2, 50, dui->palette[7]);


				dui->DrawBar((int)(params.envpos[params.mode-5]*170), 0, 2, 50, dui->palette[9]);


				for(int i=0;i<170;i++)
					dui->DrawBar(i, 42-(int)(params.envelope[params.mode-5][i+eoffset]*38), 1, 2, dui->palette[ecolor]);

*/
				int mx=dui->mousex-dui->gui_winx;
				int my=dui->mousey-dui->gui_winy;
				if(dui->mousey>=topwiny && dui->mousey<topwiny+48)
				{
					if(mx>=0 && mx<=170 && (dui->lmbclick || dui->rmbclick))
					{
						dui->mouseregion=45;
						pmx=-100;
					}
					if(dui->mouseregion==45)
					{
						bool lschanged=false;
						bool lechanged=false;
						if(mx<0) mx=0;
						if(mx>170) mx=170;
						if(dui->lmbdown)
						{
							if(mx!=pmx)
							{
								params.loop_start=(float)mx/170;
								lschanged=true;
							}
						}
						if(dui->rmbdown)
						{
							if(mx!=pmx)
							{
								params.loop_end=(float)mx/170;
								lechanged=true;
							}
						}
						
						// search near-zero point for loop end and order re-search of loop start
						if(lechanged)
						{
							int ile=(int)(params.loop_end*params.size);
							float best_pos=params.loop_start;
							float best_error=1000.0f;
							for(int ci=ile-256;ci<=ile+256;ci++)
							{
								int cci=ci;
								if(cci<0) cci=0;
								if(cci>params.size-1) cci=params.size-1;
								float error=0.0f;
								float dist=(float)(ci-ile)/100;
								error+=dist*dist*0.1f; // slightly prefer values close to the selected point
								float level=params.wavedata[cci];
								error+=level*level*100.0f; // prefer near-zero values
								if(error<best_error)
								{
									best_error=error;
									best_pos=(float)cci/params.size;
								}
							}
							params.loop_end=best_pos;
							lschanged=true;
						}
						// search new suitable spot for loop start if it (or loop end) changed
						if(lschanged)
						{
							int ile=(int)(params.loop_end*params.size);
							float ed1=0.0f; // left delta
							float ed2=0.0f; // right delta
							for(int i=0;i<64;i++)
							{
								float fi=1.0f/(float)(5+i);
								int pi;
								int ci=i;
								if(ci<0) ci=0;
								if(ci>params.size-1) ci=params.size-1;
								pi=ci-1;
								if(pi<0) pi=0;
								ed1+=(params.wavedata[ci]-params.wavedata[pi])*fi;
								pi=ci+1;
								if(pi>params.size-1) pi=params.size-1;
								ed2+=(params.wavedata[pi]-params.wavedata[ci])*fi;
							}
							int ils=(int)(params.loop_start*params.size);
							float best_pos=params.loop_start;
							float best_error=1000.0f;
							for(int ci=ils-1024;ci<=ils+1024;ci++)
							{
								int cci=ci;
								if(cci<0) cci=0;
								if(cci>params.size-1) cci=params.size-1;
								float d1=0.0f;
								float d2=0.0f;
								for(int i=0;i<64;i++)
								{
									float fi=1.0f/(float)(5+i);
									int pi;
									int ci=i;
									if(ci<0) ci=0;
									if(ci>params.size-1) ci=params.size-1;
									pi=ci-1;
									if(pi<0) pi=0;
									d1+=(params.wavedata[ci]-params.wavedata[pi])*fi;
									pi=ci+1;
									if(pi>params.size-1) pi=params.size-1;
									d2+=(params.wavedata[pi]-params.wavedata[ci])*fi;
								}
								float error=0.0f;
								float dist=(float)(ci-ils)/5000;
								error+=dist*dist*0.1f; // slightly prefer values close to the selected point
								float level=params.wavedata[cci];
								error+=level*level*50.0f; // prefer near-zero values
								float d1diff=(d1-ed1);
								error+=d1diff*50000.0f; // left delta error
								float d2diff=(d2-ed2);
								error+=d2diff*50000.0f; // right delta error
								if(error<best_error)
								{
									best_error=error;
									best_pos=(float)cci/params.size;
								}
							}
							params.loop_start=best_pos;
						}

						if(params.loop_start>params.loop_end)
							params.loop_start=params.loop_end;
					}
				}
				pmx=mx;
			}
			dui->EndScrollArea();
		}
	};

	bool Save(FILE *file)
	{
		kfwrite2(name, 64, 1, file);
		int numbytes=sizeof(vsmp_params)+64+params.size*sizeof(short);
		kfwrite2(&numbytes, 1, sizeof(int), file);
		kfwrite2(visname, 64, 1, file);
		kfwrite(&params, 1, sizeof(vsmp_params), file);
		for(int wi=0;wi<params.size;wi++)
		{
			short sample=(short)(params.wavedata[wi]*32767.0f);
			kfwrite2(&sample, 1, sizeof(short), file);
		}
		return true;
	};

	bool Load(FILE *file)
	{
		int numbytes=0;
		kfread2(&numbytes, 1, sizeof(int), file);
		// crude compatibility/version test -------------------------------- doesn't really work with variable data size... hmm, check swave
/*		if(numbytes!=sizeof(vsmp_params)+64+params.size*sizeof(short))
		{
			LogPrint("ERROR: tried to load incompatible version of instrument \"%s\"", name);
			ReadAndDiscard(file, numbytes);
			return false;
		}*/
		kfread2(visname, 64, 1, file);
		kfread(&params, 1, sizeof(vsmp_params), file);

		params.wavedata=(float*)malloc(params.size*sizeof(float));
		for(int wi=0;wi<params.size;wi++)
		{
			short sample;
			kfread2(&sample, 1, sizeof(short), file);
			params.wavedata[wi]=(float)sample/32767.0f;
		}

		if(params.mono)
			numchannels=1;
		else
			numchannels=16;
		return true;
	};
};

#endif

