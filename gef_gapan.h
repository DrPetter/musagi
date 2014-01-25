#ifndef gef_gapan_h
#define gef_gapan_h

#include "gear_effect.h"

class gef_gapan : public gear_effect
{
private:

	struct gapan_params
	{
		float gain;
		float pan;
		bool dist_on;
		int dist_type;
		float dist_amp;
		float reverb;
		float reverbdepth;
		float reverbfidelity;
		float reverbpts[256];
		float fcut;
		float fres;
		float fhp;
	};
	
	struct gapan_old2params
	{
		float gain;
		float pan;
		bool dist_on;
		int dist_type;
		float dist_amp;
		float reverb;
		float reverbdepth;
		float reverbfidelity;
		float reverbpts[256];
		float fcut;
		float fres;
	};
	
	struct gapan_oldparams
	{
		float gain;
		float pan;
		bool dist_on;
		int dist_type;
		float dist_amp;
		float reverb;
		float reverbdepth;
		float reverbfidelity;
		float reverbpts[256];
	};

	float *delaybuffer_l;
	float *delaybuffer_r;
	float delayoffset_l;
	float delayoffset_r;
	float psl;
	float psr;
	float fy_l, fdy_l;
	float fy_r, fdy_r;
	float hy_l, hps_l;
	float hy_r, hps_r;
	int fltout;
	int delayptr;
	int delaytimeout;
	StereoBufferP dbuffer;

	gapan_params params;

public:
	gef_gapan()
	{
//		LogPrint("gef_gapan: init");

		strcpy(name, "gapan");
				
		params.gain=0.5f;
		params.pan=0.5f;
		params.dist_on=false;
		params.dist_type=0;
		params.dist_amp=0.25f;
		params.reverb=0.0f;
		params.reverbdepth=0.5f;
		params.reverbfidelity=0.5f;
		params.fcut=0.0f;
		params.fres=0.0f;
		params.fhp=0.0f;
		GenReverbPts();
		
		delaybuffer_l=NULL;
		delaybuffer_r=NULL;
		delayoffset_l=0.0f;
		delayoffset_r=0.0f;
		psl=0.0f;
		psr=0.0f;
		fy_l=0.0f;
		fdy_l=0.0f;
		fy_r=0.0f;
		fdy_r=0.0f;
		fltout=0;
		hy_l=0.0f;
		hy_r=0.0f;
		hps_l=0.0f;
		hps_r=0.0f;
		delaytimeout=0;
		
		dbuffer.size=0;
		dbuffer.left=NULL;
		dbuffer.right=NULL;
	};

	~gef_gapan()
	{
		free(delaybuffer_l);
		free(delaybuffer_r);
		free(dbuffer.left);
		free(dbuffer.right);
	};

	void GenReverbPts()
	{
		for(int i=0;i<256;i++)
		{
			params.reverbpts[i]=(float)rnd(10000)/10000;
//			LogPrint("reverbpts[%i]=%.4f (-> %i)", i, params.reverbpts[i], (int)(params.reverbpts[i]*(65536*5-105))+100);
		}
	};
	
//	void ProcessBuffer(StereoBufferP *buffer)
	int ProcessBuffer(StereoBufferP *buffer)
	{
		int debug_iterations=0;
		bool triggered=buffer->size>0;
		
//		LogPrint("gapan: buffer->size=%i", buffer->size);
		bool dbout=false;
		if(buffer->size>0 && dbuffer.size==0)
		{
//			LogPrint("gapan: dbuffer alloc (%i)", buffer->size);
			dbuffer.size=buffer->size;
			dbuffer.left=(float*)malloc(dbuffer.size*sizeof(float));
			dbuffer.right=(float*)malloc(dbuffer.size*sizeof(float));
			dbuffer.mono=false;
			dbuffer.undefined=false;
		}
		if(params.reverb>0.0f || fltout<1000)
		{
			if(params.reverb>0.0f && delaybuffer_l==NULL) // no buffer allocated, remedy this
			{
//				LogPrint("gapan: delaybuffer alloc");
				delaybuffer_l=(float*)malloc(65536*sizeof(float));
				delaybuffer_r=(float*)malloc(65536*sizeof(float));
				for(int i=0;i<65536;i++)
				{
					delaybuffer_l[i]=0.0f;
					delaybuffer_r[i]=0.0f;
					debug_iterations++;
				}
				delayptr=0;
			}
			if(buffer->size==0 && dbuffer.size>0) // if input buffer is absent, use internal buffer (needs to generate sound)
			{
				if(delaytimeout>0 || fltout<1000)
				{
					buffer->size=dbuffer.size;
					buffer->left=dbuffer.left;
					buffer->right=dbuffer.right;
//					LogPrint("gapan: reassign and clear %i samples", buffer->size);
					for(int i=0;i<buffer->size;i++)
					{
						buffer->left[i]=0.0f;
						buffer->right[i]=0.0f;
						debug_iterations++;
					}
					delaytimeout-=buffer->size;
				}
			}
			else
				delaytimeout=(int)(params.reverbdepth*(65536-105))+100+buffer->size; // max reverb time caused by this input buffer
		}
		if(buffer->size==0) // silent input, and no lingering effects
		{
			fy_l=0.0f;
			fdy_l=0.0f;
			fy_r=0.0f;
			fdy_r=0.0f;
			hy_l=0.0f;
			hy_r=0.0f;
			hps_l=0.0f;
			hps_r=0.0f;
			return 0;
		}
		if(params.fhp>0.0f)
		{
			float damp=pow(params.fhp, 2.0f)*0.5f;
			for(int i=0;i<buffer->size;i++)
			{
				hy_l+=buffer->left[i]-hps_l;
				hps_l=buffer->left[i];
				hy_l-=hy_l*damp;
				if(hy_l>0.0f && hy_l<0.0001f)
					hy_l=0.0001f;
				if(hy_l<0.0f && hy_l>-0.0001f)
					hy_l=-0.0001f;
				buffer->left[i]=hy_l;
/*				if(!dbout && (hy_l>2.0f || hy_l<-2.0f))
				{
					dbout=true;
					LogPrint("asplode_L!");
				}*/
//				buffer->left[i]=hps_l;
//				buffer->left[i]=0.0f;
//				buffer->left[i]=-buffer->left[i];
				debug_iterations++;
			}
			dbout=false;
			for(int i=0;i<buffer->size;i++)
			{
				hy_r+=buffer->right[i]-hps_r;
				hps_r=buffer->right[i];
				hy_r-=hy_r*damp;
				if(hy_r>0.0f && hy_r<0.0001f)
					hy_r=0.0001f;
				if(hy_r<0.0f && hy_r>-0.0001f)
					hy_r=-0.0001f;
				buffer->right[i]=hy_r;
/*				if(!dbout && (hy_r>2.0f || hy_r<-2.0f))
				{
					dbout=true;
					LogPrint("asplode_R!");
				}*/
//				buffer->right[i]=hps_r;
//				buffer->right[i]=0.0f;
//				buffer->right[i]=-buffer->right[i];
				debug_iterations++;
			}
		}
		else
		{
			hy_l=0.0f;
			hy_r=0.0f;
			hps_l=0.0f;
			hps_r=0.0f;
		}
		if(params.fcut>0.0f)
		{
//			int lout=fltout;
//			int rout=fltout;
			float fp1=3.8f-pow(params.fres, 0.5f)*3.5f;
//			float fp2=1.5f-pow(params.fcut, 0.05f)*1.497f;
			float fp2=0.003f+pow(1.0f-params.fcut, 4.0f)*1.497f;
			float damp=(1.2f-params.fres)*0.05f;
			if(params.fcut<0.3f)
			{
				fp1+=(0.3f-fp1)*(0.3f-params.fcut)/0.3f;
//				damp*=params.fcut/0.3f;
			}
			float blend=1.0f;
			if(params.fcut<0.25f)
				blend=1.0f-pow((0.25f-params.fcut)*4, 2.0f);
			for(int i=0;i<buffer->size;i++)
			{
/*				if(buffer->left[i]==0.0f)
					lout++;
				else
					lout=0;*/
				fdy_l+=(buffer->left[i]-(fy_l+fdy_l*fp1))*fp2;
				fdy_l-=fdy_l*damp;
				if(fdy_l>5.0f) fy_l=5.0f;
				if(fdy_l<-5.0f) fdy_l=-5.0f;
				fy_l+=fdy_l;
				buffer->left[i]+=(fy_l-buffer->left[i])*blend;
				if(buffer->left[i]>0.0f && buffer->left[i]<0.0001f)
					buffer->left[i]=0.0001f;
				if(buffer->left[i]<0.0f && buffer->left[i]>-0.0001f)
					buffer->left[i]=-0.0001f;
				debug_iterations++;
			}
			for(int i=0;i<buffer->size;i++)
			{
/*				if(buffer->right[i]==0.0f)
					rout++;
				else
					rout=0;*/
				fdy_r+=(buffer->right[i]-(fy_r+fdy_r*fp1))*fp2;
				fdy_r-=fdy_r*damp;
				if(fdy_r>5.0f) fy_r=5.0f;
				if(fdy_r<-5.0f) fdy_r=-5.0f;
				fy_r+=fdy_r;
				buffer->right[i]+=(fy_r-buffer->right[i])*blend;
				if(buffer->right[i]>0.0f && buffer->right[i]<0.0001f)
					buffer->right[i]=0.0001f;
				if(buffer->right[i]<0.0f && buffer->right[i]>-0.0001f)
					buffer->right[i]=-0.0001f;
				debug_iterations++;
			}
/*			if(lout<rout)
				fltout=lout;
			else
				fltout=rout;*/
			if(triggered)
				fltout=0;
			else
				fltout+=buffer->size;
		}
		else
		{
			fy_l=0.0f;
			fdy_l=0.0f;
			fy_r=0.0f;
			fdy_r=0.0f;
			fltout=1000;
		}
		if(params.dist_on)
		{
			float damp=pow(params.dist_amp*4.0f, 3.0f);
			// left
			for(int i=0;i<buffer->size;i++)
			{
				buffer->left[i]*=damp;
				if(params.dist_type==0)
				{
					if(buffer->left[i]>1.0f)
						buffer->left[i]=1.0f;
					else if(buffer->left[i]<-1.0f)
						buffer->left[i]=-1.0f;
				}
				if(params.dist_type==1)
					buffer->left[i]=(float)sin(buffer->left[i]);
				if(params.dist_type==2)
				{
					if(buffer->left[i]>1.0f)
					{
						buffer->left[i]=2.0f-buffer->left[i];
						if(buffer->left[i]<-1.0f)
							buffer->left[i]=-1.0f;
					}
					else if(buffer->left[i]<-1.0f)
					{
						buffer->left[i]=-2.0f-buffer->left[i];
						if(buffer->left[i]>1.0f)
							buffer->left[i]=1.0f;
					}
				}
				debug_iterations++;
			}
			if(buffer->mono) // identical channels, copy results
			{
				float *bp=buffer->right;
				float *sp=buffer->left;
				for(int i=0;i<buffer->size;i+=8)
				{
					(*bp++)=*sp++;
					(*bp++)=*sp++;
					(*bp++)=*sp++;
					(*bp++)=*sp++;
					(*bp++)=*sp++;
					(*bp++)=*sp++;
					(*bp++)=*sp++;
					(*bp++)=*sp++;
					debug_iterations++;
				}
			}
			else
			{
				// right
				for(int i=0;i<buffer->size;i++)
				{
					buffer->right[i]*=damp;
					if(params.dist_type==0)
					{
						if(buffer->right[i]>1.0f)
							buffer->right[i]=1.0f;
						else if(buffer->right[i]<-1.0f)
							buffer->right[i]=-1.0f;
					}
					if(params.dist_type==1)
						buffer->right[i]=(float)sin(buffer->right[i]);
					debug_iterations++;
				}
			}
		}

		// reverb
		if(params.reverb>0.0f)
		{
			int rpts[256];
			float rppw[256];
			int ifi=(int)(params.reverbfidelity*240+16);
			for(int i=0;i<256;i++)
			{
				rpts[i]=(int)(params.reverbpts[i]*params.reverbdepth*(65536-105))+100;
				rppw[i]=(0.3f+params.reverbpts[255-i]*0.4f)*exp(-params.reverbpts[i]*3.0f)*200/(ifi+64);
				debug_iterations++;
			}
//			if(buffer->size>256)
//				LogPrint("buffer->size==%i", buffer->size);
			for(int i=0;i<buffer->size;i++)
			{
				float sample_l=0.0f;
				float sample_r=0.0f;
				delaybuffer_l[delayptr]=(psl+buffer->left[i])*0.5f*params.reverb;
				delaybuffer_r[delayptr]=(psr+buffer->right[i])*0.5f*params.reverb;

				psl=buffer->left[i];
				psr=buffer->right[i];

				for(int d=0;d<ifi;d++)
				{
					int rpos=delayptr-rpts[d];
					rpos+=65536;
					rpos&=65536-1;
					sample_l+=delaybuffer_l[rpos]*rppw[d];
					sample_r+=delaybuffer_r[rpos]*rppw[d];
					debug_iterations++;
				}

				delayoffset_l+=(sample_l-delayoffset_l)*0.001f;
				delayoffset_r+=(sample_r-delayoffset_r)*0.001f;

				buffer->left[i]+=sample_l-delayoffset_l;
				buffer->right[i]+=sample_r-delayoffset_r;

				delayptr++;
				delayptr&=65536-1;
				debug_iterations++;
			}
		}

		float span=params.pan;
		if(fabs(span-0.5f)<0.05f)
			span=0.5f;
		float pgain=pow(params.gain*2.0f, 2.0f);
		float gain;
		// left
		gain=(1.0f-span)*2.0f;
		if(gain>1.0f) gain=1.0f;
		gain*=pgain;
		for(int i=0;i<buffer->size;i++)
		{
			buffer->left[i]*=gain;
			debug_iterations++;
		}
		// right
		gain=(span)*2.0f;
		if(gain>1.0f) gain=1.0f;
		gain*=pgain;
		for(int i=0;i<buffer->size;i++)
		{
			buffer->right[i]*=gain;
			debug_iterations++;
		}
		
		return debug_iterations;
	};

	void DoInterface(DUI *dui)
	{
		dui->DrawBar(1, 0, 400, 5, dui->palette[4]);
		dui->DrawBar(1, 25, 400, 5, dui->palette[4]);
		dui->DrawTexturedBar(1, 5, 400, 20, dui->palette[4], dui->brushed_metal, &dui->vglare);
		dui->DrawBox(0, 1, 400, 29, dui->palette[6]);

		dui->DrawText(349, 20, dui->palette[7], "g a p a n");
		dui->DrawText(350, 20, dui->palette[7], "g a p a n");

		dui->DrawText(23, 4, dui->palette[6], "gain");
		dui->DoKnob(5, 2, params.gain, -1, "knob_gain", "final gain");
		dui->DrawText(43, 17, dui->palette[6], "pan");
		dui->DoKnob(25, 12, params.pan, -1, "knob_pan", "peter");

		dui->DrawBar(66, 1, 1, 29, dui->palette[6]);
		dui->DrawBar(67, 1, 1, 28, dui->palette[7]);
		dui->DrawText(71, 5, dui->palette[4], "FLT");
		dui->DrawText(70, 5, dui->palette[14], "FLT");
		dui->DrawLED(129, 4, 1, params.fcut>0.0f||params.fhp>0.0f);
		dui->DrawText(88, 19, dui->palette[6], "RS");
		dui->DoKnob(70, 12, params.fres, -1, "knob_frs", "low pass filter resonance");
		dui->DrawText(110, 3, dui->palette[6], "HP");
		dui->DoKnob(92, 2, params.fhp, -1, "knob_fhp", "high pass filter frequency (applied before low pass)");
		dui->DrawText(125, 19, dui->palette[6], "LP");
		dui->DoKnob(107, 12, params.fcut, -1, "knob_fco", "low pass filter frequency");

		dui->DrawBar(139, 1, 1, 29, dui->palette[6]);
		dui->DrawBar(140, 1, 1, 28, dui->palette[7]);
		dui->DrawText(158, 5, dui->palette[14], "dist");
		dui->DrawText(157, 5, dui->palette[6], "dist");
		dui->DrawLED(180-33, 15, 0, params.dist_on);
		if(dui->DoButton(180-23, 15, "btn_dist", "toggle distortion"))
			params.dist_on=!params.dist_on;
		dui->DrawLED(220-33, 1, 0, params.dist_type==0);
		if(dui->DoButton(220-23, 1, "btn_dtype0", "distortion type: clip"))
			params.dist_type=0;
		dui->DrawLED(220-33, 10, 0, params.dist_type==1);
		if(dui->DoButton(220-23, 10, "btn_dtype1", "distortion type: sine"))
			params.dist_type=1;
		dui->DrawLED(220-33, 19, 0, params.dist_type==2);
		if(dui->DoButton(220-23, 19, "btn_dtype2", "distortion type: bounce and clip"))
			params.dist_type=2;
		dui->DrawText(238, 11, dui->palette[6], "amp");
		dui->DoKnob(220, 6, params.dist_amp, -1, "knob_amp", "amplification before applying distortion");
		dui->DrawBar(265, 1, 1, 29, dui->palette[6]);
		dui->DrawBar(266, 1, 1, 28, dui->palette[7]);

		dui->DrawLED(271, 18, 0, params.reverb>0.0f);
		dui->DrawText(298, 8, dui->palette[6], "str");
		dui->DoKnob(280, 3, params.reverb, -1, "knob_rvb", "reverb strength");
		dui->DrawText(335, 8, dui->palette[6], "dpt");
		dui->DoKnob(317, 3, params.reverbdepth, -1, "knob_rvf", "reverb depth");
		dui->DrawText(373, 8, dui->palette[6], "qty");
		dui->DoKnob(355, 3, params.reverbfidelity, -1, "knob_rfi", "reverb quality (and cpu usage)");
		dui->DrawText(283, 19, dui->palette[14], "reverb");
		dui->DrawText(282, 19, dui->palette[6], "reverb");
	};
	
	bool Save(FILE *file)
	{
		kfwrite2(name, 64, 1, file);
		int numbytes=sizeof(gapan_params);
		kfwrite2(&numbytes, 1, sizeof(int), file);
		kfwrite(&params, 1, sizeof(gapan_params), file);
		return true;
	};

	bool Load(FILE *file, int fversion)
	{
//		fread(name, 64, 1, file);
		int numbytes=0;
		kfread2(&numbytes, 1, sizeof(int), file);
		if(fversion<2)
		{
			if(numbytes!=sizeof(gapan_oldparams))
			{
				LogPrint("ERROR: tried to load incompatible version of effect \"%s\"", name);
				ReadAndDiscard(file, numbytes);
				return false;
			}
			kfread(&params, 1, sizeof(gapan_oldparams), file);
			params.fcut=0.0f;
			params.fres=0.0f;
			params.fhp=0.0f;
		}
		else if(fversion<3)
		{
			if(numbytes!=sizeof(gapan_old2params))
			{
				LogPrint("ERROR: tried to load incompatible version of effect \"%s\"", name);
				ReadAndDiscard(file, numbytes);
				return false;
			}
			kfread(&params, 1, sizeof(gapan_old2params), file);
			params.fhp=0.0f;
		}
		else
		{
			if(numbytes!=sizeof(gapan_params))
			{
				LogPrint("ERROR: tried to load incompatible version of effect \"%s\"", name);
				ReadAndDiscard(file, numbytes);
				return false;
			}
			kfread(&params, 1, sizeof(gapan_params), file);
		}
		return true;
	};
};

#endif

