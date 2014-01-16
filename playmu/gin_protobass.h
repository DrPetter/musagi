#ifndef gin_protobass_h
#define gin_protobass_h

#include "musagi.h"
#include "gear_instrument.h"

struct protobass_params
{
	float filter_cutoff;
	float filter_resonance;
	float test_envmax;
	float test_envspeed;
	float test_envshape;
	float* sawform[5];
};

class protobass_channel : gear_ichannel
{
public:
	// parameters
	protobass_params *params;

	// internal data
	int period;
	int tick;
	int cur_note;
	float volume;

	int ppos;
	int rpos;
	int pspos;
	int wtick;
	float fwtick;

	float user_cutoff;
	float smooth_cutoff;
	float smooth_resonance;
	float cur_cutoff;
	float cur_resonance;
	float dr;
	float lp_wave;

	int al_note_attack;
	int al_note_decay;
	int al_rapid_decay;
	int al_note_freq;
	float al_note_freqf;
	int al_next_freq;
	bool al_note_on;
	bool al_note_off;

	int numslide;
	double dslide;

	protobass_channel()
	{
		params=NULL;

		tick=0;
		al_note_attack=0;
		al_note_on=false;
		al_note_off=false;
		volume=0.0f;
		cur_note=-1;

		ppos=0;
		rpos=0;
		pspos=0;
//		wtick=0;
		fwtick=0.0f;

		user_cutoff=1.0f;
		smooth_cutoff=1.0f;
		smooth_resonance=0.0f;
		cur_cutoff=1.0f;
		cur_resonance=0.0f;
		dr=0.0f;
		lp_wave=0.0f;
	};

	~protobass_channel()
	{
	};

	int RenderBuffer(StereoBufferP *buffer, int size)
	{
		if(!active)
		{
			// instantly update cutoff
			smooth_cutoff=pow(params->filter_cutoff, 2.0f)*512;
			cur_cutoff=smooth_cutoff+params->test_envmax*200;
			smooth_resonance=params->filter_resonance;
			cur_resonance=smooth_resonance;
			return 0;
		}

		float* sawform[5];
		for(int i=0;i<5;i++)
			sawform[i]=params->sawform[i];
		
		for(int i=0;i<size;i++)
		{
			float sample=0.0f;

			if(numslide>0)
			{
				al_note_freqf=(float)((double)al_note_freqf/dslide);
				al_note_freq=(int)al_note_freqf;
				numslide--;
			}
			
			if(al_note_off) // Note off
				al_note_decay=1;
			if(al_note_on) // Note on
			{
				if(al_note_attack>0) // smooth note transition
					al_rapid_decay=1;
				else
				{
					al_note_freq=al_next_freq;
					al_note_freqf=(float)al_note_freq;
					al_note_attack=1;
					al_note_decay=0;
					al_rapid_decay=0;
					wtick=0; // Reset wave generator phase on new note
//					fwtick=0.0f; // Reset wave generator phase on new note
					pspos=4096; // Ensure immediate wrap
				}
			}
			al_note_on=false;
			al_note_off=false;

			if(al_note_attack>0)
			{
				wtick++;
				fwtick+=al_note_freqf*4096/44100;
//				int spos=(int)((float)(wtick%(44100/al_note_freq))/(44100/al_note_freq)*4096);
//				sposf+=
				if(fwtick>=4096)
					fwtick-=4096;
				int spos=(int)fwtick;
				rpos++;
				if(spos<pspos) // Wrap - reset resonance counter and lock filter parameters
				{
					rpos=0;
					dr=((float)rpos-15)*(cur_cutoff+6.0f)/327;
					smooth_resonance=lerp(0.2f, smooth_resonance, params->filter_resonance);
					cur_resonance=smooth_resonance;
				}
				pspos=spos;
				if(spos>4095)
					spos=4095;
	
				float reso_factor;
				dr+=(cur_cutoff+6.0f)/327;
				if(dr<0)
					reso_factor=0.0f;
				else
					reso_factor=(1.0f-cos(dr))*1.0f;
				if(spos>3296) // fade out to avoid discontinuity
					reso_factor*=0.5f-cos((float)(4096-spos)/800*PI)*0.5f;
				float new_lp_wave;
				if(cur_cutoff/150<1.0f)
					new_lp_wave=lerp(cur_cutoff/150, sawform[4][spos], sawform[3][spos]);
				else if(cur_cutoff/150-1.0f<1.0f)
					new_lp_wave=lerp(cur_cutoff/150-1.0f, sawform[3][spos], sawform[2][spos]);
				else
					new_lp_wave=lerp(cur_cutoff/150-2.0f, sawform[2][spos], sawform[1][spos]);
				lp_wave=lerp(0.1f, lp_wave, new_lp_wave); // smooth interpolation to avoid pops during cutoff sweep
				float dr2=(float)rpos-10;
				if(dr2<0.0f) dr2=0.0f;
				sample=-(1.0f-cur_resonance*0.8f)*lp_wave
						- exp(-dr2/(0.0001f+pow(cur_resonance, 4)*400.0f))*(sawform[0][spos]+pow(1.0f-cur_cutoff/512, 3.0f)*0.8f)*reso_factor + 0.35f;
	
				sample*=volume;
	
				if(al_note_attack<300) // envelope attack
				{
					sample*=(float)al_note_attack/300;
					al_note_attack++;
				}
				if(al_note_decay>0) // envelope decay
				{
					sample*=exp(-(float)al_note_decay/1000);
					al_note_decay++;
					if(al_note_decay>3000) // end of note (arbitrary)
						al_note_attack=0;
				}
				if(al_rapid_decay>0) // rapid decay
				{
					sample*=exp(-(float)al_rapid_decay/200);
					al_rapid_decay++;
					if(al_rapid_decay>500) // end of note (arbitrary)
					{
						al_note_freq=al_next_freq;
						al_note_freqf=(float)al_note_freq;
						al_note_attack=1;
						al_note_decay=0;
						al_rapid_decay=0;
						wtick=0; // Reset wave generator phase on new note
						fwtick=0.0f; // Reset wave generator phase on new note
//						fwtick2=0.0f; // Reset wave generator phase on new note
						pspos=4096; // Ensure immediate wrap
					}
				}

				// Update cutoff
				smooth_cutoff=lerp(0.001f, smooth_cutoff, pow(params->filter_cutoff, 2.0f)*512);
				float envelope_max=params->test_envmax*200;
				float envelope=(float)wtick*(float)pow(params->test_envspeed, 2.0f)/1000;
//				float envelope=fwtick2*(float)pow(params->test_envspeed, 2.0f)/1000;
				if(envelope>1.0f)
					envelope=1.0f;
				envelope=pow(envelope, 0.5f+(float)pow(params->test_envshape*2, 2.0f)*0.75f);
				cur_cutoff=smooth_cutoff+envelope_max-envelope*envelope_max;
	
				buffer->left[i]+=sample;
			}
			else
				active=false;

			tick++;
		}
			
		return 1; // produced mono output (left channel)
	};
	
	void Trigger(int tnote, float tvolume)
	{
		al_next_freq=(int)GetNoteFrequency(tnote);
		al_note_on=true;

		cur_note=tnote;
		volume=tvolume;

		dslide=1.0;
		numslide=0;

		active=true;
	};
	
	void Release()
	{
		al_note_off=true;
		cur_note=-1;
	};

	void Slide(int tnote, int length)
	{
		if(!active)
			return;

		al_note_freqf=(float)al_note_freq;
		float period=44100.0f/al_note_freqf;

		numslide=length;
		dslide=1.0+log(44100.0/GetNoteFrequency(tnote)/period)/(double)numslide;
		numslide+=1;
	};
};

class gin_protobass : public gear_instrument
{
private:
	protobass_params params;
	float sawform[5][4096];
	
	void SmoothWaveform(float *wf, float *tw, int size, int flat, int iterations)
	{
		float max;
		for(int s=0;s<iterations;s++)
		{
			for(int f=0;f<flat;f++)
				tw[f]=tw[4095];
			for(int i=size;i<4096-size;i++)
			{
				float val=0.0f;
				int n=0;
				int end=i+size;
				if(size>30)
				{
					for(int index=i-size;index<=end;index+=4)
						val+=tw[index];
					wf[i]=val/((end-(i-size)+1)/4);
				}
				else if(size>20)
				{
					for(int index=i-size;index<=end;index+=2)
						val+=tw[index];
					wf[i]=val/((end-(i-size)+1)/2);
				}
				else
				{
					for(int index=i-size;index<=end;index++)
						val+=tw[index];
					wf[i]=val/((end-(i-size)+1));
				}
			}
			max=0.0f;
			for(int i=0;i<4096;i++)
			{
				tw[i]=wf[i];
				if(wf[i]>max) max=wf[i];
			}
		}
		// Normalize
		for(int i=0;i<4096;i++)
			wf[i]*=1.0f/max;
	};
	
	void InitWaveform()
	{
		// Generate 303-ish sawtooth waveform
		float tempform[4096];
		// Filtered saw 1
		for(int i=0;i<4096;i++)
		{
			float x=(float)i/4096;
			tempform[i]=pow(x, 0.3f)*exp(-x*7);
		}
		SmoothWaveform(sawform[1], tempform, 16, 50, 15);
		// Filtered saw 2
		for(int i=0;i<4096;i++)
		{
			float x=(float)i/4096;
			tempform[i]=pow(x, 0.5f)*exp(-x*7);
		}
		SmoothWaveform(sawform[2], tempform, 24, 50, 20);
		// Filtered saw 3
		for(int i=0;i<4096;i++)
		{
			float x=(float)i/4096;
			tempform[i]=x*x*exp(-x*15);
		}
		SmoothWaveform(sawform[3], tempform, 32, 50, 30);
		// Filtered saw 4
		for(int i=0;i<4096;i++)
		{
			float x=(float)i/4096;
			tempform[i]=x*x*x*x*exp(-x*15);
		}
		SmoothWaveform(sawform[4], tempform, 64, 50, 40);
		for(int i=0;i<4096;i++)
			tempform[i]=sawform[4][(i+400)%4096];
		for(int i=0;i<4096;i++)
			sawform[4][i]=tempform[i];
		// Hard saw (for resonance)
		for(int i=0;i<4096;i++)
		{
			float x=(float)i/4096;
			tempform[i]=exp(-x*6.0f)+0.1f;
		}
		SmoothWaveform(sawform[0], tempform, 16, 40, 10);
	};

public:
	gin_protobass()
	{
//		LogPrint("gin_protobass: init");

		strcpy(name, "protobass");
		strcpy(visname, name);

		params.filter_cutoff=0.0f;
		params.filter_resonance=1.0f;
		params.test_envmax=0.5f;
		params.test_envspeed=0.2f;
		params.test_envshape=0.0f;
		for(int i=0;i<5;i++)
			params.sawform[i]=sawform[i];

		InitWaveform();

		numchannels=4;
		for(int i=0;i<numchannels;i++)
		{
			channels[i]=(gear_ichannel*)(new protobass_channel());
			((protobass_channel*)channels[i])->params=&params;
		}
	};

	~gin_protobass()
	{
	};

	bool Save(FILE *file)
	{
		kfwrite2(name, 64, 1, file);
		int numbytes=sizeof(protobass_params)+64;
		kfwrite2(&numbytes, 1, sizeof(int), file);
		kfwrite2(visname, 64, 1, file);
		kfwrite(&params, 1, sizeof(protobass_params), file);
		return true;
	};

	bool Load(FILE *file)
	{
		int numbytes=0;
		kfread2(&numbytes, 1, sizeof(int), file);
		// crude compatibility/version test
		if(numbytes!=sizeof(protobass_params)+64)
		{
			LogPrint("ERROR: tried to load incompatible version of instrument \"%s\"", name);
			ReadAndDiscard(file, numbytes);
			return false;
		}
		kfread2(visname, 64, 1, file);
		kfread(&params, 1, sizeof(protobass_params), file);
		for(int i=0;i<5;i++)
			params.sawform[i]=sawform[i];
		return true;
	};
};

#endif

