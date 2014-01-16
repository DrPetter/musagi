#ifndef gin_chip_h
#define gin_chip_h

#include "musagi.h"
#include "gear_instrument.h"

struct chippattern
{
	int offset[32];
	int end;
	int pos;
};

struct chip_params
{
	chippattern pattern[1];
	float vol[1];
	float duty[1];
	float detune[1];
	float vislen;
	int vislenupdated;
	
	float envelope[3][256];
	float envmid[3];
	float envloop[3];
	float envpos[3];

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
};

struct chip_old1params
{
	chippattern pattern[1];
	float vol[1];
	float duty[1];
	float detune[1];
	float vislen;
	int vislenupdated;
	
	float envelope[3][256];
	float envmid[3];
	float envloop[3];
	float envpos[3];

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
//	float dpcmspeed;
};

struct chip_oldparams
{
	chippattern pattern[1];
	float vol[1];
	float duty[1];
	float detune[1];
	float vislen;
	int vislenupdated;
	
	float envelope[3][256];
	float envmid[3];
	float envloop[3];
	float envpos[3];

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
//	float dpcmspeed;
};

class chip_channel : gear_ichannel
{
public:
	// parameters
	chip_params *params;

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
		int st;
		float envpos;
		float denvpos;
		bool decay;
		int iwlen;
		int patpos;
	};
	neschannel channels[1];
	
	float scale[49];
	
	float volume;
	int irval;
	int tick;
	int pt;
	
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

	chip_channel()
	{
		params=NULL;

		tick=0;
		volume=0.0f;
		irval=0;

		fy=0.0f;
		fdy=0.0f;
		dslide=0.0f;
		vibphase=0.0f;
		
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
	};

	~chip_channel()
	{
	};

	float WaveformSample(float t1, float t2, float *wave, int length)
	{
		if(t2<t1) // wrapped
			t2+=1.0f;
		t1*=length;
		t2*=length;
		int i1=(int)(t1-0.5f);
		int i2=(int)(t2-0.5f);
//		if(params->filter)
//			i2=i1; // ************ test!
		if(i1==i2) // both sample points on same wave sample
		{
/*			if(params->filter)
			{
				float f=t1-(float)i1;
				if(i1>=length)
					i1=0;
				i2=i1+1;
				if(i2>=length)
					i2=0;
				float s1=wave[256-length+i1];
				float s2=wave[256-length+i2];
				return s1+(s2-s1)*f;
			}
			else*/
				return wave[256-length+i1];
		}
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

			if(dead || params->vol[0]==0.0f)
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

				if(channels[0].period<2.0f) // don't go above 22 kHz... you fool
					channels[0].period=2.0f;
				if(channels[0].period>44100.0f) // don't go below 1 Hz... you damn fool!
					channels[0].period=44100.0f;
				if(channels[0].noteperiod<2.0f) // don't go above 22 kHz... for real
					channels[0].noteperiod=2.0f;
				if(channels[0].noteperiod>44100.0f) // don't go below 1 Hz... you're not listening!
					channels[0].noteperiod=44100.0f;
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
					if(!channels[0].playing)
						channels[0].t=0.0f; // reset phase if we're coming from silence
					if(channels[0].patpos>=0)
					{
						channels[0].playing=true;
						channels[0].period=44100.0f/GetNoteFrequency(autoarp[channels[0].patpos]);
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
					if(!channels[0].playing)
						channels[0].t=0.0f; // reset phase if we're coming from silence
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
			int index=(int)(channels[0].denvpos*170);
			if(index>169)
				index=169;
			dutenv[0]=1.0f-(1.0f-params->envelope[1][index])*(1.0f-params->duty[0]);

			channels[0].t+=1.0f/channels[0].period;
			if(channels[0].t>=1.0f)
			{
				if(channels[0].duestop)
				{
					channels[0].playing=false;
					channels[0].duestop=false;
				}
				channels[0].t-=1.0f;
				// latch stuff
				channels[0].duty=dutenv[0];
				channels[0].iwlen=170-(int)(dutenv[0]*170);
				if(channels[0].iwlen<2)
					channels[0].iwlen=2;
				params->vislen=dutenv[0];
				params->vislenupdated=0;
//					if(i==3)
//						for(int n=256-16;n<256;n++)
//							noisewave[n]=(float)rnd(16)/8-1.0f;
			}
			channels[0].pft=channels[0].ft;
			channels[0].ft=channels[0].t;

//			if(deadch==1)
//				active=false;

			float sample=0.0f;

			// squares
//			for(int i=0;i<2;i++)
//				if(channels[i].playing && params->vol[i]>0.0f)
//					sample+=SquareSample(channels[i].pft, channels[i].ft, channels[i].duty)*volenv[i]*params->vol[i];

			// triangle
			if(channels[0].playing && params->vol[0]>0.0f)
			{
				if(!params->dpcm)
					sample+=WaveformSample(channels[0].pft, channels[0].ft, params->envelope[2], channels[0].iwlen)*volenv[0]*params->vol[0];
				else
				{
					float curlevel=(float)dpcmlevel/16;
					dpcmphase+=103.412543f/channels[0].period;
					if(dpcmphase>=1.0f)
					{
						dpcmphase-=1.0f;
						float target=WaveformSample(channels[0].pft, channels[0].ft, params->envelope[2], channels[0].iwlen)*volenv[0]*params->vol[0];
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
				params->envpos[2]=(float)(170-channels[0].iwlen)/170.0f+channels[0].ft*(float)channels[0].iwlen/170.0f;
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

		int nch=0;
		for(nch=0;nch<1;nch++)
			if(params->vol[nch]>0.0f)
				break;
		if(params->mono && params->nslide>0.01f && nch<4 && channels[nch].noteperiod>0.0f)
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

		for(int i=0;i<1;i++)
		{
			if(numslide==0)
				channels[i].noteperiod=44100.0f/GetNoteFrequency(tnote);
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
	};
	
	void Release()
	{
		for(int i=0;i<1;i++)
			channels[i].decay=true;

		numarpnotes=0;
		autoarp_delay=0;
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
		dslide=1.0+log(44100.0/GetNoteFrequency(tnote)/channels[nch].noteperiod)/(double)numslide;
		numslide+=1;
	};
};

class gin_chip : public gear_instrument
{
private:
	chip_params params;
	int pmx;
	bool envline_active;
	int envline_x1, envline_y1, envline_x2, envline_y2;
	
public:
	gin_chip()
	{
//		LogPrint("gin_chip: init");

		strcpy(name, "chip");
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
		params.duty[0]=0.0f;
		params.detune[0]=0.5f;
		for(int i=0;i<3;i++)
		{
			for(int ei=0;ei<256;ei++)
			{
				if(i==0)
					params.envelope[i][ei]=1.0f;
				if(i==1)
					params.envelope[i][ei]=0.0f;
				if(i==2)
				{
					params.envelope[i][ei]=1.0f;
					if(ei>230)
						params.envelope[i][ei]=0.0f;
					if(ei>240)
						params.envelope[i][ei]=1.0f;
					if(ei>245)
						params.envelope[i][ei]=-1.0f;
				}
			}
			params.envmid[i]=1.0f;
			params.envloop[i]=0.95f;
			params.envpos[i]=0.0f;
		}
		params.vol[0]=1.0f;
		params.vislen=0.0f;

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

		numchannels=16;
		for(int i=0;i<numchannels;i++)
		{
			channels[i]=(gear_ichannel*)(new chip_channel());
			((chip_channel*)channels[i])->params=&params;
		}
	};

	~gin_chip()
	{
	};

	bool Save(FILE *file)
	{
		kfwrite2(name, 64, 1, file);
		int numbytes=sizeof(chip_params)+64;
		kfwrite2(&numbytes, 1, sizeof(int), file);
		kfwrite2(visname, 64, 1, file);
		kfwrite(&params, 1, sizeof(chip_params), file);
		return true;
	};

	bool Load(FILE *file)
	{
		int numbytes=0;
		kfread2(&numbytes, 1, sizeof(int), file);
		// crude compatibility/version test
		if(numbytes!=sizeof(chip_params)+64)
		{
			if(numbytes==sizeof(chip_old1params)+64) // old version?
			{
				kfread2(visname, 64, 1, file);
				kfread(&params, 1, sizeof(chip_old1params), file);
				if(params.mono)
					numchannels=1;
				else
					numchannels=16;
				return true;
			}
			if(numbytes==sizeof(chip_oldparams)+64) // old version?
			{
				kfread2(visname, 64, 1, file);
				kfread(&params, 1, sizeof(chip_oldparams), file);
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
		kfread(&params, 1, sizeof(chip_params), file);
		if(params.mono)
			numchannels=1;
		else
			numchannels=16;
		return true;
	};
};

#endif

