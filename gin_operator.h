


/*
00000000

// store into these
wave_a[2][128*4]
wave_b[2][128*4]


012012012012012

111222333444555
x  x  x  x  x   <- crossfade between n and n+1
   
0  ffff  0000    [0] a,b
0000  ****  0000 [1] a,b
*/

#ifndef gin_operator_h
#define gin_operator_h

#include "musagi.h"
#include "gear_instrument.h"

struct operator_params
{
	GearStack* gearstack1;
	GearStack* gearstack2;
	float strength;
	bool link;
	int mode;
	float debug_bands[20];
};

#define NUMBANDS 20

class operator_channel : gear_ichannel
{
public:
	// parameters
	operator_params *params;

	// internal data
	int period;
	int tick;
	int cur_note;
	float volume;

	int numslide;
	double dslide;
	gear_instrument* parent;
	
	double wavebuffer_a[2][512];
	double wavebuffer_b[2][512];
	double fftbuffer[1024];
	double fftcons[1024];
	int curbuffer;
	int curstep;
	
	struct FBand
	{
		int pos;
		float value;
		float weight;
	};
	FBand fbands[20];

	operator_channel()
	{
		params=NULL;

		tick=0;
		volume=0.0f;
		cur_note=-1;
		parent=NULL;

		curbuffer=0;
		curstep=0;
		for(int i=0;i<512;i++)
		{
			wavebuffer_a[0][i]=0.0;
			wavebuffer_b[0][i]=0.0;
			wavebuffer_a[1][i]=0.0;
			wavebuffer_b[1][i]=0.0;
		}
		for(int i=0;i<NUMBANDS;i++)
		{
			fbands[i].pos=(int)(pow((float)i/(NUMBANDS-1), 2.0f)*30.0f);
			fbands[i].weight=(float)fbands[i].pos/100*0.03+0.0005f;
		}
		// treble band
		fbands[NUMBANDS-1].pos=255;
		fbands[NUMBANDS-1].weight=0.0f;
		fbands[NUMBANDS-2].weight=0.0f;
	};

	~operator_channel()
	{
	};

	int RenderBuffer(StereoBufferP *buffer, int size)
	{
		// silence input instruments
		if(params->gearstack1!=NULL && params->gearstack1->instrument!=NULL)
			params->gearstack1->instrument->override=8;
		if(params->gearstack2!=NULL && params->gearstack2->instrument!=NULL)
			params->gearstack2->instrument->override=8;

		if(params->gearstack1!=NULL && params->gearstack1->instrument!=NULL &&
		   params->gearstack2!=NULL && params->gearstack2->instrument!=NULL && params->link)
		{
			params->gearstack1->instrument->triggerlink=params->gearstack2->instrument;
			params->gearstack2->instrument->triggerlink=params->gearstack1->instrument;
		}
		else
		{
			if(params->gearstack1!=NULL && params->gearstack1->instrument!=NULL)
				params->gearstack1->instrument->triggerlink=NULL;
			if(params->gearstack2!=NULL && params->gearstack2->instrument!=NULL)
				params->gearstack2->instrument->triggerlink=NULL;
		}

		if(params->gearstack2!=NULL && params->gearstack2->instrument!=NULL)
			parent->triggerlink=params->gearstack2->instrument;
		else if(params->gearstack1!=NULL && params->gearstack1->instrument!=NULL)
			parent->triggerlink=params->gearstack1->instrument;
		else
			parent->triggerlink=NULL;

//		if(params->gearstack1!=NULL && params->gearstack2!=NULL) // activate when at least one input is selected
//			active=true;
//		else
//			active=false;

		if(params->gearstack1!=NULL && params->gearstack1->instrument!=NULL)
			if(!params->gearstack1->instrument->satisfied)
			{
				parent->satisfied=false;
				return 0; // can't produce output yet
			}
		if(params->gearstack2!=NULL && params->gearstack2->instrument!=NULL)
			if(!params->gearstack2->instrument->satisfied)
			{
				parent->satisfied=false;
				return 0; // can't produce output yet
			}



		// vocoder, manage buffers and perform filtering
		if(params->mode==1)
		{
			for(int i=0;i<size;i++)
			{
				float s1=0.0f;
				float s2=0.0f;
				if(params->gearstack1!=NULL && params->gearstack1->instrument!=NULL)
					s1=params->gearstack1->instrument->buffer.left[i]; // TODO: mix stereo to mono
				if(params->gearstack2!=NULL && params->gearstack2->instrument!=NULL)
					s2=params->gearstack2->instrument->buffer.left[i]; // TODO: mix stereo to mono
				wavebuffer_a[curbuffer][curstep*size+i]=s1;
				wavebuffer_b[curbuffer][curstep*size+i]=s2;
			}

			if(curstep==3) // last step, we've filled the buffer and can now perform filtering
			{
				// FFT on A
				fft_convert(wavebuffer_a[curbuffer], fftbuffer, size*4, 1);
				// analyze FFT
				for(int i=0;i<NUMBANDS-1;i++)
				{
					int num=0;
					for(int f=fbands[i].pos;f<=fbands[i+1].pos;f++)
					{
						double c1=fftbuffer[f*2+0];
						double c2=fftbuffer[f*2+1];
						fbands[i].value+=c1*c1+c2*c2;
						num++;
					}
					fbands[i].value/=num;
					fbands[i].value*=fbands[i].weight;
					if(fbands[i].value>1.0f) fbands[i].value=1.0f;

					params->debug_bands[i]=fbands[i].value;
				}
				// apply highpass filtering on A
//				for(int i=0;i<size*4*2;i++)
//					fftcons[i]=fftbuffer[i];
				for(int f=0;f<size*2;f++)
				{
					if(f<fbands[NUMBANDS-2].pos)
					{
						fftbuffer[f*2+0]=0.0;
						fftbuffer[f*2+1]=0.0;
						fftbuffer[(size*4-1-f)*2+0]=0.0;
						fftbuffer[(size*4-1-f)*2+1]=0.0;
					}
					else
					{
						double boost=-0.6+(float)(f-fbands[NUMBANDS-2].pos)/20;
						boost*=7.0;
						if(boost>10.0) boost=10.0;
						if(boost<0.0) boost=0.0;
						fftbuffer[f*2+0]*=boost;
						fftbuffer[f*2+1]*=boost;
						fftbuffer[(size*4-1-f)*2+0]*=boost;
						fftbuffer[(size*4-1-f)*2+1]*=boost;
					}
				}
				// inverse FFT on A
				fft_convert(wavebuffer_a[curbuffer], fftbuffer, size*4, -1);

				// FFT on B
				fft_convert(wavebuffer_b[curbuffer], fftbuffer, size*4, 1);
				// apply analysis on FFT
				int curband=0;
				for(int f=0;f<size*2;f++)
				{
/*					if(abs(f-size*2)>180)
					{
						fftbuffer[f*2+0]=0.0;
						fftbuffer[f*2+1]=0.0;
					}*/
					fftbuffer[f*2+0]*=fbands[curband].value;
					fftbuffer[f*2+1]*=fbands[curband].value;
					fftbuffer[(size*4-1-f)*2+0]*=fbands[curband].value;
					fftbuffer[(size*4-1-f)*2+1]*=fbands[curband].value;
					while(f>=fbands[curband+1].pos)
						curband++;
/*					if(curband==NUMBANDS-2) // copy treble parts from voice sound (consonants)
					{
						double boost=-0.6+(float)(f-fbands[curband].pos)/20;
						boost*=7.0;
						if(boost>10.0) boost=10.0;
						if(boost<0.0) boost=0.0;
						fftbuffer[f*2+0]=fftcons[f*2+0]*boost;
						fftbuffer[f*2+1]=fftcons[f*2+1]*boost;
						fftbuffer[(size*4-1-f)*2+0]=fftcons[(size*4-1-f)*2+0]*boost;
						fftbuffer[(size*4-1-f)*2+1]=fftcons[(size*4-1-f)*2+1]*boost;
					}*/
				}
				// multiply consonants with original B
/*				static double cons_p=0.0;
				double pp=0.0;
				for(int i=0;i<size*4;i++)
				{
					double t=-wavebuffer_b[curbuffer][i];
					if(t<0.0) t=0.0;
					cons_p+=(t-cons_p)*0.01;
					if(cons_p<0.0) cons_p=0.0;
					wavebuffer_a[curbuffer][i]*=cons_p;
					if(i==size*3)
						pp=cons_p;
				}
				cons_p=pp;*/
				// make consonants stutter
				if(curbuffer==0)
				{
					for(int i=0;i<size*4;i++)
						wavebuffer_a[curbuffer][i]=0.0;
				}
				else
				{
					double vol=0.0;
					for(int i=0;i<size*4;i+=4)
						vol+=fabs(wavebuffer_b[curbuffer][i]);
					vol/=20;
					if(vol>1.0f) vol=1.0f;
					for(int i=0;i<size*4;i++)
						wavebuffer_a[curbuffer][i]*=vol;
				}
				// inverse FFT on B
				fft_convert(wavebuffer_b[curbuffer], fftbuffer, size*4, -1);
				// add consonants to B
				for(int i=0;i<size*4;i++)
					wavebuffer_b[curbuffer][i]+=wavebuffer_a[curbuffer][i];

				curbuffer=1-curbuffer; // switch buffers
				
				curstep=0; // skip ahead since we're repeating at 3 steps, not 4 (see below)
			}
		}
		else
			for(int i=0;i<NUMBANDS-1;i++)
				params->debug_bands[i]=sin((float)i/(NUMBANDS-2)*6.28f)*0.3f+0.5f;


/*		if(!active)
		{
			// instantly update smoothed parameters
			return 0;
		}
*/

//		StereoBufferP* wavein=GetWaveInBuffer();

		float volprobe=0.0f;

		for(int i=0;i<size;i++)
		{
			float sample=0.0f;

/*			if(numslide>0)
			{
				al_note_freqf=(float)((double)al_note_freqf/dslide);
				al_note_freq=(int)al_note_freqf;
				numslide--;
			}
*/

			// ring modulation
			if(params->mode==0)
			{
				float s1=0.0f;
				float s2=0.0f;
				if(params->gearstack1!=NULL && params->gearstack1->instrument!=NULL)
					s1=params->gearstack1->instrument->buffer.left[i]; // TODO: mix stereo to mono
				if(params->gearstack2!=NULL && params->gearstack2->instrument!=NULL)
					s2=params->gearstack2->instrument->buffer.left[i]; // TODO: mix stereo to mono
	
				sample=lerp(params->strength, (s1+s2), (s1*s2));
			}

			// vocoder
			if(params->mode==1)
			{
				if(curstep==0)
				{
					float f=(float)i/size;
					sample=wavebuffer_b[curbuffer][3*size+i]*(1.0f-f)+wavebuffer_b[1-curbuffer][0*size+i]*f; // crossfade
				}
				else
					sample=wavebuffer_b[1-curbuffer][curstep*size+i];

				float s1=0.0f;
				if(params->gearstack1!=NULL && params->gearstack1->instrument!=NULL)
					s1=params->gearstack1->instrument->buffer.left[i]; // TODO: mix stereo to mono
				sample=lerp(params->strength, s1, sample);
			}
			
//			sample+=(float)(i%64)/150; // test sawtooth

			buffer->left[i]+=sample;

//			buffer->left[i]+=(wavein->left[i]+wavein->right[i])*5.0f;
			
			volprobe+=fabs(sample);

			tick++;
		}
		
		if(volprobe>0.1f)
			active=true;
		else
			active=false;

		// vocoder
		if(params->mode==1)
		{
			if(curstep==0)
				for(int i=0;i<size;i++)
				{
					float s1=0.0f;
					float s2=0.0f;
					if(params->gearstack1!=NULL && params->gearstack1->instrument!=NULL)
						s1=params->gearstack1->instrument->buffer.left[i]; // TODO: mix stereo to mono
					if(params->gearstack2!=NULL && params->gearstack2->instrument!=NULL)
						s2=params->gearstack2->instrument->buffer.left[i]; // TODO: mix stereo to mono
					wavebuffer_a[curbuffer][curstep*size+i]=s1;
					wavebuffer_b[curbuffer][curstep*size+i]=s2;
				}

			curstep=(curstep+1)%4;
		}

		return 1; // produced mono output (left channel)
	};
	
	void Trigger(int tnote, float tvolume)
	{
//		al_next_freq=(int)GetNoteFrequency(tnote);
//		al_note_on=true;

		cur_note=tnote;
		volume=tvolume;

		dslide=1.0;
		numslide=0;

		active=true;
	};
	
	void Release()
	{
//		al_note_off=true;
		cur_note=-1;
	};

	void Slide(int tnote, int length)
	{
		if(!active)
			return;
/*
		al_note_freqf=(float)al_note_freq;
		float period=44100.0f/al_note_freqf;

		numslide=length;
		dslide=1.0+log(44100.0/GetNoteFrequency(tnote)/period)/(double)numslide;
		numslide+=1;*/
	};
};

class gin_operator : public gear_instrument
{
private:
	operator_params params;
	float wavebuffer[4096];
	int selecting_gearstack;
	
public:
	gin_operator()
	{
//		LogPrint("gin_operator: init");

		strcpy(name, "operator");
		strcpy(visname, name);

		params.strength=1.0f;
		params.gearstack1=NULL;
		params.gearstack2=NULL;
		params.link=false;
		params.mode=0;

		selecting_gearstack=0;

		numchannels=1;
		for(int i=0;i<numchannels;i++)
		{
			channels[i]=(gear_ichannel*)(new operator_channel());
			((operator_channel*)channels[i])->params=&params;
			((operator_channel*)channels[i])->parent=this;
		}
	};

	~gin_operator()
	{
	};

	void DoInterface(DUI *dui)
	{
		dui->DrawTexturedBar(0, 0, 400, 20, dui->palette[8], dui->brushed_metal, &dui->vglare);
		dui->DrawTexturedBar(0, 20, 400, 10, dui->palette[17], dui->brushed_metal, &dui->vglare);
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
		dui->DrawText(335-1, 40, dui->palette[4], "operator");
		dui->DrawText(335+1, 40, dui->palette[4], "operator");
		dui->DrawText(335, 40, dui->palette[6], "operator");
		dui->DoKnob(20, 30-8, params.strength, -1, "knob_strength", "strength of effect");
		dui->DrawText(6, 13, dui->palette[6], "strength");
/*		dui->DoKnob(75, 30-8, params.filter_resonance, -1, "knob_resonance", "amount of resonance at cutoff frequency");
		dui->DrawText(75-4, 12, dui->palette[6], "reso");
		dui->DoKnob(120, 30-8, params.test_envmax, -1, "knob_envmax", "scale of envelope modulation - envelope controls cutoff");
		dui->DrawText(120-14, 12, dui->palette[6], "env.mod.");
		dui->DoKnob(165, 30-8, params.test_envspeed, -1, "knob_envspeed", "speed of envelope decay");
		dui->DrawText(165-8, 12, dui->palette[6], "decay");
		dui->DoKnob(210, 30-8, params.test_envshape, -1, "knob_envshape", "envelope decay linearity");
		dui->DrawText(210-10, 12, dui->palette[6], "env.shp");*/

		dui->DrawText(175, 21, dui->palette[6], "Link");
		dui->DrawLED(140, 20, 0, params.link);
		if(dui->DoButton(150, 20, "btn_lnk", "link instruments to play in unison"))
			params.link=!params.link;

		dui->DrawText(275, 9, dui->palette[6], "ringmod");
		dui->DrawLED(240, 8, 0, params.mode==0);
		if(dui->DoButton(250, 8, "btn_m0", "ring modulation (multiply waveforms)"))
			params.mode=0;
		dui->DrawText(275, 21, dui->palette[6], "vocoder");
		dui->DrawLED(240, 20, 0, params.mode==1);
		if(dui->DoButton(250, 20, "btn_m1", "vocoder (A=voice B=synth)"))
			params.mode=1;
/*		dui->DrawText(275, 33, dui->palette[6], "chopper");
		dui->DrawLED(240, 32, 0, params.mode==2);
		if(dui->DoButton(250, 32, "btn_m2", "chops sound A into looping blocks"))
			params.mode=2;
		
		dui->DrawText(275, 6+0*10, dui->palette[6], "ringmod");
		dui->DrawLED(240, 5+0*10, 0, params.mode==0);
		if(dui->DoButton(250, 5+0*10, "btn_m0", "ring modulation (multiply waveforms)"))
			params.mode=0;
		dui->DrawText(275, 6+1*10, dui->palette[6], "vocoder");
		dui->DrawLED(240, 5+1*10, 0, params.mode==1);
		if(dui->DoButton(250, 5+1*10, "btn_m1", "vocoder (A=voice B=synth)"))
			params.mode=1;
		dui->DrawText(275, 6+2*10, dui->palette[6], "chopper");
		dui->DrawLED(240, 5+2*10, 0, params.mode==2);
		if(dui->DoButton(250, 5+2*10, "btn_m2", "chops sound A into looping blocks"))
			params.mode=2;
		dui->DrawText(275, 6+3*10, dui->palette[6], "flanger");
		dui->DrawLED(240, 5+3*10, 0, params.mode==3);
		if(dui->DoButton(250, 5+3*10, "btn_m3", "mix sound A with a delayed copy"))
			params.mode=3;
*/
		if(selecting_gearstack==1)
			dui->DrawText(90, 9, dui->palette[3], "A: SELECT");
		else
		{
			if(params.gearstack1==NULL || params.gearstack1->instrument==NULL)
			{
				dui->DrawText(90, 9, dui->palette[6], "A: (none)");
				params.gearstack1=NULL;
			}
			else
			{
				char text[128];
				sprintf(text, "A: %s", params.gearstack1->instrument->visname);
				dui->DrawText(90, 9, dui->palette[6], text);
			}
		}
		if(dui->DoButton(65, 8, "btn_gs1", "select instrument A"))
		{
			if(selecting_gearstack!=1)
				selecting_gearstack=1;
			else
				selecting_gearstack=0;
		}
		if(selecting_gearstack==2)
			dui->DrawText(90, 33, dui->palette[3], "B: SELECT");
		else
		{
			if(params.gearstack2==NULL || params.gearstack2->instrument==NULL)
			{
				dui->DrawText(90, 33, dui->palette[6], "B: (none)");
				params.gearstack2=NULL;
			}
			else
			{
				char text[128];
				sprintf(text, "B: %s", params.gearstack2->instrument->visname);
				dui->DrawText(90, 33, dui->palette[6], text);
			}
		}
		if(dui->DoButton(65, 32, "btn_gs2", "select instrument B"))
		{
			if(selecting_gearstack!=2)
				selecting_gearstack=2;
			else
				selecting_gearstack=0;
		}
		
		GearStack* cgs=GetCurrentGearstack();
		if(cgs!=NULL)
		{
			if(selecting_gearstack>0 && cgs->instrument!=this)
			{
				if(selecting_gearstack==1)
					params.gearstack1=cgs;
				if(selecting_gearstack==2)
					params.gearstack2=cgs;
				selecting_gearstack=0;
			}
		}

/*		if(params.gearstack1!=NULL && params.gearstack1->instrument!=NULL &&
		   params.gearstack2!=NULL && params.gearstack2->instrument!=NULL)
		{
			if(
			params.gearstack1->instrument->triggerlink=params.gearstack2->instrument;
			params.gearstack2->instrument->triggerlink=params.gearstack1->instrument;
		}
		else
		{
			params.gearstack1->instrument->triggerlink=NULL;
			params.gearstack2->instrument->triggerlink=NULL;
		}
			
/*		if(params->gearstack1!=NULL && params->gearstack1->instrument!=NULL &&
		   params->gearstack2!=NULL && params->gearstack2->instrument!=NULL)
		{
			params->gearstack1->instrument->triggerlink=params->gearstack2->instrument;
			params->gearstack2->instrument->triggerlink=params->gearstack1->instrument;
		}*/

		for(int i=0;i<NUMBANDS-1;i++)
		{
			dui->DrawBar(329+i*3, 10, 4, 30, dui->palette[6]);
			dui->DrawBar(330+i*3, 10, 2, 30, dui->palette[4]);
			dui->DrawBar(330+i*3, 40-(int)(params.debug_bands[i]*30), 2, (int)(params.debug_bands[i]*30), dui->palette[15]);
		}
	};

	bool Save(FILE *file)
	{
		kfwrite2(name, 64, 1, file);
		int numbytes=sizeof(operator_params)+64;
		kfwrite2(&numbytes, 1, sizeof(int), file);
		kfwrite2(visname, 64, 1, file);
		GearStack* gs1=params.gearstack1;
		GearStack* gs2=params.gearstack2;
		// find index of each input gearstack and save those to file rather than the pointers
		params.gearstack1=(GearStack*)MapFromGearStack(params.gearstack1);
		params.gearstack2=(GearStack*)MapFromGearStack(params.gearstack2);
		kfwrite(&params, 1, sizeof(operator_params), file);
		params.gearstack1=gs1;
		params.gearstack2=gs2;
		return true;
	};

	bool Load(FILE *file)
	{
		int numbytes=0;
		kfread2(&numbytes, 1, sizeof(int), file);
		// crude compatibility/version test
		if(numbytes!=sizeof(operator_params)+64)
		{
			LogPrint("ERROR: tried to load incompatible version of instrument \"%s\"", name);
			ReadAndDiscard(file, numbytes);
			return false;
		}
		kfread2(visname, 64, 1, file);
		kfread(&params, 1, sizeof(operator_params), file);

		// find new pointers for each gearstack used
		params.gearstack1=MapToGearStack((int)params.gearstack1);
		params.gearstack2=MapToGearStack((int)params.gearstack2);

		return true;
	};
/*	
	void RestorePointers()
	{
		// find new pointers for each gearstack used
		params.gearstack1=MapToGearStack((int)params.gearstack1);
		params.gearstack2=MapToGearStack((int)params.gearstack2);
	};*/
};

#endif

