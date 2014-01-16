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

	bool Save(FILE *file)
	{
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

