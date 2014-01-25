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

	chip_channel()
	{
		params=NULL;

		tick=0;
		volume=0.0f;
		irval=0;
		
		for(int i=0;i<1;i++)
		{
			channels[i].playing=false;
			channels[i].duty=0.5f;
			channels[i].pft=0.0f;
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
	};

	~chip_channel()
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
				fdy+=(sample-(fy+fdy*3))*0.05f;
				if(fdy>0.5f)
					fy=0.5f;
				if(fdy<-0.5f)
					fdy=-0.5f;
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
				channels[i].t=0.0f;
				channels[i].ft=0.0f;
				channels[i].pft=0.0f;
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
	};
	
	void Release()
	{
		for(int i=0;i<1;i++)
			channels[i].decay=true;
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

	void DoInterface(DUI *dui)
	{
		dui->DrawBar(0, 0, 400, 50, dui->palette[4]);
		dui->DrawBar(0, 30, 395, 15, dui->palette[6]);
		dui->DrawBox(0, 30, 395, 15, dui->palette[8]);
//		dui->DrawBox(20, 30, 45, 10, dui->palette[8]);
//		dui->DrawBar(21, 29, 43, 10, dui->palette[4]);
		dui->DrawBox(20, 30, 60, 10, dui->palette[8]);
		dui->DrawBar(21, 29, 58, 10, dui->palette[4]);
		dui->DrawBox(87, 30, 70, 10, dui->palette[8]);
		dui->DrawBar(88, 29, 68, 10, dui->palette[4]);
		dui->DrawBox(0, 1, 400, 49, dui->palette[6]);

		dui->DrawText(369, 40, dui->palette[6], "chip");
		dui->DrawText(371, 40, dui->palette[6], "chip");
		dui->DrawText(370, 40, dui->palette[9], "chip");

		dui->DrawText(30, 5, dui->palette[6], "vol");
		dui->DoKnob(12, 2, params.vol[0], -1, "knob_vol1", "volume");
		dui->DrawLED(2, 2, 0, params.vol[0]>0.0f);

		dui->DrawText(68, 5, dui->palette[6], "offset");
		dui->DoKnob(50, 2, params.duty[0], -1, "knob_dut1", "waveform offset");

		dui->DrawLED(162, 2, 0, params.mode==0);
		if(dui->DoButton(170, 2, "btn_params.mode1", "arpeggio"))
			params.mode=0;

		dui->DrawLED(127, 2, 0, params.mode==5);
		if(dui->DoButton(135, 2, "btn_params.mode6", "volume envelope"))
			params.mode=5;
		dui->DrawLED(127, 2+12, 0, params.mode==6);
		if(dui->DoButton(135, 2+12, "btn_params.mode7", "offset envelope"))
			params.mode=6;

		dui->DrawLED(92, 2+12, 0, params.mode==7);
		if(dui->DoButton(100, 2+12, "btn_params.mode8", "waveform shape"))
			params.mode=7;

		dui->DrawText(123, 2+25, dui->palette[6], "DPCM");
		dui->DrawLED(92, 2+24, 1, params.dpcm);
		if(dui->DoButton(100, 2+24, "btn_dpcm", "tries to mimic the delta-pcm channel of the NES"))
			params.dpcm=!params.dpcm;

		dui->DrawText(56, 2+25, dui->palette[6], "FLT");
		dui->DrawLED(25, 2+24, 1, params.filter);
		if(dui->DoButton(33, 2+24, "btn_flt", "filter waveform"))
			params.filter=!params.filter;

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
		else if(params.mode<5)
		{
			int topwiny=dui->gui_winy;
			float dummy=0.0f;
			dui->StartScrollArea(193, 1, 170, 48, dummy, params.patscrolly, 170, 49*3, dui->palette[14], "chipscroll");
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
		else
		{
			int topwiny=dui->gui_winy;
			float dummy=0.0f;
			dui->StartScrollArea(193, 1, 170, 48, dummy, dummy, 170, 48, dui->palette[14], "chipscroll2");
			{
				int ecolor=9;
				if(params.mode==6)
					ecolor=3;
				int eoffset=0;
				if(params.mode==7) // chip wave
				{
					eoffset=256-170;
					if(params.vislenupdated>5)
						params.vislen=params.duty[0];
					params.vislenupdated++;
					dui->DrawBar((int)(params.vislen*170), 0, 171-(int)(params.vislen*170), 50, dui->palette[15]);
				}
				else
				{
					dui->DrawBar((int)(params.envmid[params.mode-5]*170), 0, 2, 50, dui->palette[6]);
					dui->DrawBar((int)(params.envloop[params.mode-5]*170), 0, 2, 50, dui->palette[7]);
				}
				dui->DrawBar((int)(params.envpos[params.mode-5]*170), 0, 2, 50, dui->palette[9]);
				if(params.mode==7)
				{
					for(int i=0;i<170;i++)
						dui->DrawBar(i, 42-(int)((params.envelope[params.mode-5][i+eoffset]*0.5f+0.5f)*38), 1, 2, dui->palette[9]);
				}
				else
				{
					for(int i=0;i<170;i++)
						dui->DrawBar(i, 42-(int)(params.envelope[params.mode-5][i+eoffset]*38), 1, 2, dui->palette[ecolor]);
				}
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
						if(!dui->shift_down && dui->lmbdown) // draw
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
								if(params.mode==7)
								{
									params.envelope[params.mode-5][x+eoffset]=params.envelope[params.mode-5][x+eoffset]*2.0f-1.0f;
									if(params.envelope[params.mode-5][x+eoffset]>1.0f) params.envelope[params.mode-5][x+eoffset]=1.0f;
									if(params.envelope[params.mode-5][x+eoffset]<-1.0f) params.envelope[params.mode-5][x+eoffset]=-1.0f;
								}
								else
								{
									if(params.envelope[params.mode-5][x+eoffset]>1.0f) params.envelope[params.mode-5][x+eoffset]=1.0f;
									if(params.envelope[params.mode-5][x+eoffset]<0.0f) params.envelope[params.mode-5][x+eoffset]=0.0f;
								}
							}
						}
						if(params.mode!=7) // place midpoint or loop point
						{
							if(dui->shift_down && dui->lmbdown)
								params.envloop[params.mode-5]=(float)mx/170;
							if(dui->rmbdown)
								params.envmid[params.mode-5]=(float)mx/170;
							if(params.envloop[params.mode-5]>params.envmid[params.mode-5])
								params.envloop[params.mode-5]=params.envmid[params.mode-5];
						}
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
							params.envelope[params.mode-5][x]=y1*(1.0f-f)+y2*f;
							if(params.envelope[params.mode-5][x]>1.0f) params.envelope[params.mode-5][x]=1.0f;
							if(params.envelope[params.mode-5][x]<0.0f) params.envelope[params.mode-5][x]=0.0f;
						}
						envline_active=false;
					}
					dui->DrawLine(envline_x1, envline_y1, envline_x2, envline_y2, 2, dui->palette[11]);
				}
				pmx=mx;
			}
			dui->EndScrollArea();
		}
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

