#ifndef gin_xnes_h
#define gin_xnes_h

#include "musagi.h"
#include "gear_instrument.h"

struct xnespattern
{
	int offset[32];
	int end;
	int pos;
};

struct xnes_params
{
	xnespattern pattern[4];
	float vol[4];
	float duty[4];
	float detune[4];
	
	float envelope[6][170];
	float envmid[6];
	float envloop[6];
	float envpos[6];

	float speed;
	bool loop;
	bool real;

	float slide_delay;
	float slide_speed;
	float vibrato_depth;
	float vibrato_speed;

	float envspd_attack;
	float envspd_decay;

	float patscrolly;
	
	bool mono;
	float nslide;

	int mode;
	
	// new 2007
	
//	bool autoarp;
//	int cur_arpnote;
};

struct xnes_oldparams
{
	xnespattern pattern[4];
	float vol[4];
	float duty[4];
	float detune[4];
	
	float envelope[6][170];
	float envmid[6];
	float envloop[6];
	float envpos[6];

	float speed;
	bool loop;
	bool real;

	float slide_delay;
	float slide_speed;
	float vibrato_depth;
	float vibrato_speed;

	float envspd_attack;
	float envspd_decay;

	float patscrolly;
	
	bool mono;
	float nslide;
};

class xnes_channel : gear_ichannel
{
public:
	// parameters
	xnes_params *params;

	// internal data
	struct neschannel
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
//		int st;
		float envpos;
		float denvpos;
		bool decay;
		int patpos;
	};
	neschannel channels[4];
	
	float scale[49];
	
	float volume;
	int irval;
	int tick;
	int pt;
	float trianglewave[256];
	float noisewave[256];
	
	int numslide;
	double dslide;
	float vibphase;

	// auto-arpeggio (from polyphony on mono instrument)
	int numarpnotes;
	int autoarp[8];
	int autoarp_delay;

	bool mono_released;

	xnes_channel()
	{
		params=NULL;

		tick=0;
		volume=0.0f;
		irval=0;

		for(int i=0;i<256;i++)
			trianglewave[i]=0.0f;
		int tpos=256-40;
		for(int i=0;i<16;i++)
			trianglewave[tpos++]=2.0f-(float)i/16*4.0f;
		for(int i=0;i<16;i++)
			trianglewave[tpos++]=(float)i/16*4.0f-2.0f;
		for(int i=0;i<8;i++)
			trianglewave[tpos++]=2.0f-(float)i/16*4.0f;

		for(int i=0;i<256;i++)
			noisewave[i]=(float)rnd(16)/8-1.0f;

		for(int i=0;i<4;i++)
		{
			channels[i].playing=false;
			channels[i].duty=0.5f;
			channels[i].pft=0.0f;
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

	~xnes_channel()
	{
	};

	float SquareSample(float t1, float t2, float duty)
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

	float WaveformSample(float t1, float t2, float *wave, int length)
	{
		if(t2<t1) // wrapped
			t2+=1.0f;
		t1*=length;
		t2*=length;
		int i1=(int)(t1-0.5f);
		int i2=(int)(t2-0.5f);
		if(i1==i2) // both sample points on same wave sample
			return wave[256-length+i1];
		float fi1=(float)i1;
		float fi2=(float)i2;
		float p1=1.0f-(t1-fi1);
		float p2=t2-fi2;
		float ws=0.0f;
		int wsamples=0;
		for(int i=i1+1;i<i2;i++) // add all intermediate whole sample values
		{
			int wi=256-length+i;
			if(wi>=256)
				wi-=length;
			ws+=wave[wi];
			wsamples++;
		}
		if(i2>=length)
			i2-=length;
		i1+=256-length;
		i2+=256-length;
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
			int iperiod[4];
//			float ft[4];
			bool ptrig=false;
			pt++;
			if(pt>=iptperiod)
			{
				pt=0;
				ptrig=true;
			}
			int deadch=0;
			float volenv[4];
			float dutenv[2];
			if(numslide>0)
				numslide--;

			if(numarpnotes>0)
				autoarp_delay++; // time passed since trigger, only arpeggiate on near-simultaneous notes

			if(tick>=islidetrig)
				if(vibdep>0.0f)
				{
					vibphase+=vibspeed;
					if(vibphase>=2*PI)
						vibphase-=2*PI;
//					slidespd+=sin(vibphase)*(double)vibdep*0.01*vibspeed;
					slidespd+=cos(vibphase)*(double)vibdep*0.01*vibspeed;
				}
			for(int i=0;i<4;i++)
			{
				if(params->vol[i]==0.0f)
				{
					deadch++;
					continue;
				}

				if(numslide>0)
				{
					channels[i].noteperiod=(float)((double)channels[i].noteperiod*dslide);
					channels[i].period=(float)((double)channels[i].period*dslide);
				}
				if(tick>=islidetrig)
				{
					channels[i].noteperiod=(float)((double)channels[i].noteperiod*slidespd);
					channels[i].period=(float)((double)channels[i].period*slidespd);
					
					if(channels[i].period<2.0f) // don't go above 22 kHz... you fool
						channels[i].period=2.0f;
					if(channels[i].period>44100.0f) // don't go below 1 Hz... you damn fool!
						channels[i].period=44100.0f;
					if(channels[i].noteperiod<2.0f) // don't go above 22 kHz... for real
						channels[i].noteperiod=2.0f;
					if(channels[i].noteperiod>44100.0f) // don't go below 1 Hz... you're not listening!
						channels[i].noteperiod=44100.0f;
				}

				if(ptrig)
				{
					channels[i].patpos++;
					if(numarpnotes>1) // auto-arpeggio
					{
						if(channels[i].patpos>=numarpnotes)
						{
							if(params->loop)
								channels[i].patpos=0; // loop
							else
								channels[i].patpos--; // hold at the end position
						}
						if(!channels[i].playing)
							channels[i].t=0.0f; // reset phase if we're coming from silence
						if(channels[i].patpos>=0)
						{
							channels[i].playing=true;
							channels[i].period=44100.0f/GetNoteFrequency(autoarp[channels[i].patpos]);
						}
					}
					else
					{
						if(channels[i].patpos>params->pattern[i].end)
						{
							if(params->loop)
								channels[i].patpos=0; // loop
							else
								channels[i].patpos--; // hold at the end position
						}
						if(!channels[i].playing)
							channels[i].t=0.0f; // reset phase if we're coming from silence
						int offset=params->pattern[i].offset[channels[i].patpos];
						if(offset==999)
						{
							if(channels[i].playing)
								channels[i].duestop=true;
						}
						else
						{
							channels[i].playing=true;
							channels[i].period=channels[i].noteperiod*scale[offset+24];
						}
						params->pattern[i].pos=channels[i].patpos; // for visualization
					}
				}

				// envelope
				if(!channels[i].decay)
				{
					channels[i].envpos+=envspd_a;
					if(channels[i].envpos>params->envmid[i])
						channels[i].envpos-=params->envmid[i]-params->envloop[i];
					if(i<2) // duty envelope
					{
						channels[i].denvpos+=envspd_a;
						if(channels[i].denvpos>params->envmid[i+4])
							channels[i].denvpos-=params->envmid[i+4]-params->envloop[i+4];
					}
				}
				else
				{
					channels[i].envpos+=envspd_d;
					if(channels[i].envpos>=1.0f)
					{
						channels[i].envpos=1.0f;
						channels[i].playing=false;
						deadch++;
					}
					if(i<2) // duty envelope
					{
						channels[i].denvpos+=envspd_d;
						if(channels[i].denvpos>=1.0f)
							channels[i].denvpos=1.0f;
					}
				}
				params->envpos[i]=channels[i].envpos; // for visualization
				if(i<2) // duty envelope
					params->envpos[i+4]=channels[i].denvpos; // for visualization
				if(channels[i].envpos<1.0f)
				{
					int index=(int)(channels[i].envpos*170);
					if(index>169)
						index=169;
					volenv[i]=params->envelope[i][index];
				}
				else
					volenv[i]=0.0f;
				if(i<2) // duty envelope
				{
					int index=(int)(channels[i].denvpos*170);
					if(index>169)
						index=169;
					dutenv[i]=1.0f-(1.0f-params->envelope[i+4][index])*(1.0f-params->duty[i]);
				}

				channels[i].t+=1.0f/channels[i].period;
				if(channels[i].t>=1.0f)
				{
					if(channels[i].duestop)
					{
						channels[i].duestop=false;
						channels[i].playing=false;
					}
					channels[i].t-=1.0f;
					// latch stuff
					if(i<2)
						channels[i].duty=0.5f+pow(dutenv[i], 0.5f)*0.5f;
					if(i==3)
						for(int n=256-16;n<256;n++)
							noisewave[n]=(float)rnd(1)*2-1.0f;
				}
				channels[i].pft=channels[i].ft;
				channels[i].ft=channels[i].t;
			}
			if(deadch==4)
				active=false;

			float sample=0.0f;

			if(params->real)
			{
				// squares
				for(int i=0;i<2;i++)
					if(channels[i].playing && params->vol[i]>0.0f)
					{
						float rvol=(float)((int)(volenv[i]*params->vol[i]*16))/16;
						int rduti=(int)((channels[i].duty-0.66f)*6);
						float rdut=0.5f;
						switch(rduti)
						{
						case 0:
							rdut=0.5f;
							break;
						case 1:
							rdut=4.0f/16;
							break;
						case 2:
							rdut=2.0f/16;
							break;
						}
						sample+=SquareSample(channels[i].pft, channels[i].ft, rdut)*rvol;
					}
	
				// triangle
				if(channels[2].playing && params->vol[2]>0.0f)
					sample+=WaveformSample(channels[2].pft, channels[2].ft, trianglewave, 32)*0.5f;
	
				// noise
				if(channels[3].playing && params->vol[3]>0.0f)
				{
					float rvol=(float)((int)(volenv[3]*params->vol[3]*16))/16;
					sample+=WaveformSample(channels[3].pft, channels[3].ft, noisewave, 16)*rvol;
				}
			}
			else
			{
				// squares
				for(int i=0;i<2;i++)
					if(channels[i].playing && params->vol[i]>0.0f)
						sample+=SquareSample(channels[i].pft, channels[i].ft, channels[i].duty)*volenv[i]*params->vol[i];
	
				// triangle
				if(channels[2].playing && params->vol[2]>0.0f)
					sample+=WaveformSample(channels[2].pft, channels[2].ft, trianglewave, 32)*volenv[2]*params->vol[2];
	
				// noise
				if(channels[3].playing && params->vol[3]>0.0f)
					sample+=WaveformSample(channels[3].pft, channels[3].ft, noisewave, 16)*volenv[3]*params->vol[3];
			}

			buffer->left[si]+=sample*volume;
			tick++;
		}

		return 1; // produced mono output (left channel)
	};

	void Trigger(int tnote, float tvolume)
	{
//		LogPrint("xnes: trigger");
		
		active=true;
		volume=tvolume;

		int nch=0;
		for(nch=0;nch<4;nch++)
			if(params->vol[nch]>0.0f)
				break;
		if(params->mono && params->nslide>0.01f && nch<4 && channels[nch].noteperiod>0.0f && !mono_released)
		{
			numslide=(int)(params->nslide*20000.0f);
			dslide=1.0+log(44100.0/GetNoteFrequency(tnote)/channels[nch].noteperiod)/(double)numslide;
			numslide+=1;
		}
		else
		{
			dslide=1.0;
			numslide=0;
		}

		for(int i=0;i<4;i++)
		{
			if(numslide==0)
				channels[i].noteperiod=44100.0f/GetNoteFrequency(tnote);
			if(numslide==0 || channels[i].decay)
//			if(!params->mono || channels[i].decay)
			{
				if(channels[i].decay) // don't reset wave phase if we're interrupting a previous note (basically hack to allow smooth slide transitions)
				{
					channels[i].t=0.0f;
					channels[i].ft=0.0f;
					channels[i].pft=0.0f;
				}
//				channels[i].st=0;
				channels[i].envpos=0.0f;
				channels[i].denvpos=0.0f;
				channels[i].decay=false;
				channels[i].patpos=-1;
			}
//			params->pattern[i].pos=-1;
			channels[i].duestop=false;
		}
		vibphase=0.0f;
		pt=100000;
		tick=0;

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
//		LogPrint("xnes: release");
		for(int i=0;i<4;i++)
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
		for(nch=0;nch<4;nch++)
			if(params->vol[nch]>0.0f)
				break;
		if(channels[nch].noteperiod<=0.0f)
			return;

		numslide=length;
		dslide=1.0+log(44100.0/GetNoteFrequency(tnote)/channels[nch].noteperiod)/(double)numslide;
		numslide+=1;
	};
};

class gin_xnes : public gear_instrument
{
private:
	xnes_params params;
//	int mode;
	int pmx;
	bool envline_active;
	int envline_x1, envline_y1, envline_x2, envline_y2;
	
public:
	gin_xnes()
	{
//		LogPrint("gin_xnes: init");

		strcpy(name, "xnes");
		strcpy(visname, name);

		pmx=0;
		envline_active=false;

		params.mode=0;
		for(int i=0;i<4;i++)
		{
			for(int pi=0;pi<32;pi++)
				params.pattern[i].offset[pi]=999;
			params.pattern[i].offset[0]=0;
			params.pattern[i].offset[1]=0;
			params.pattern[i].offset[2]=0;
			params.pattern[i].end=2;
			params.pattern[i].pos=-1;
			params.vol[i]=0.0f;
			params.duty[i]=0.0f;
			params.detune[i]=0.5f;
		}
		for(int i=0;i<6;i++)
		{
			for(int ei=0;ei<170;ei++)
			{
				if(i<4)
					params.envelope[i][ei]=1.0f;
				else
					params.envelope[i][ei]=0.0f;
			}
			params.envmid[i]=1.0f;
			params.envloop[i]=0.95f;
			params.envpos[i]=0.0f;
		}
		params.vol[0]=1.0f;

		params.speed=0.8f;
		params.loop=true;
		params.real=false;
	
		params.slide_delay=0.0f;
		params.slide_speed=0.5f;
		params.vibrato_depth=0.0f;
		params.vibrato_speed=0.35f;

		params.envspd_attack=1.0f;
		params.envspd_decay=1.0f;
		
		params.patscrolly=0.5f;

		params.mono=false;
		params.nslide=0.0f;

		numchannels=16;
		for(int i=0;i<numchannels;i++)
		{
			channels[i]=(gear_ichannel*)(new xnes_channel());
			((xnes_channel*)channels[i])->params=&params;
		}
	};

	~gin_xnes()
	{
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
		if(numbytes!=sizeof(xnes_params)+64)
		{
			if(numbytes==sizeof(xnes_oldparams)+64)
			{
				kfread2(visname, 64, 1, file);
				kfread(&params, 1, sizeof(xnes_oldparams), file);
				if(params.mono)
					numchannels=1;
				else
					numchannels=16;
				return true;
			}
			LogPrint("ERROR: tried to load incompatible version of instrument \"%s\"", name);
			ReadAndDiscard(file, numbytes);
			return false;
		}
		kfread2(visname, 64, 1, file);
		kfread(&params, 1, sizeof(xnes_params), file);
		if(params.mono)
			numchannels=1;
		else
			numchannels=16;
		return true;
	};
};

#endif

