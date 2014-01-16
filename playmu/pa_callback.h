static int pa_callback(void *inputBuffer, void *outputBuffer,
			unsigned long framesPerBuffer, PaTimestamp outTime, void *userData)
{
	float *out=(float*)outputBuffer;
//	float *in=(float*)inputBuffer;
	(void)outTime;
	(void)inputBuffer;
	AudioStream *as=(AudioStream*)userData;

	if(!audio_enabled)
	{
		for(int i=0;i<framesPerBuffer;i++);
		{
			*out++=0.0f;
			*out++=0.0f;
		}
		return 0;
	}

	LogPrint("pa_callback 1");

/*	int debug_nb=-1;
	int debug_bs[10];
	for(int i=0;i<10;i++)
		debug_bs[i]=-1;
*/
//	as->perftimer->SetMark(0);
	
	StereoBufferP *buffer=&as->buffer;
//	StereoBufferP *in_buffer=&as->in_buffer;

	if(framesPerBuffer!=buffer->size)
	{
		// allocate new buffer to match portaudio settings (should not happen)
		buffer->size=framesPerBuffer;
//		in_buffer->size=framesPerBuffer;
		LogPrint("audiostream: CAUTION! buffer size mismatch!");
		// ...
	}

	// latch critical variables (not needed since callback function always executes in full?)
//	StreamInfo li=stream_info;

/*	for(int i=0;i<in_buffer->size;i++)
	{
		in_buffer->left[i]=*in++;
		in_buffer->right[i]=*in++;
	}
*/
	// !!!!!!! song/part stepping should be tweaked to get maximum trigger precision (maybe introduce read-ahead/delayed triggering?)

	LogPrint("pa_callback 2");

	int subsize=128;
	
	bool debug_log=false;
	int rec_pstop=-1;

	for(int substep=0;substep<buffer->size/subsize;substep++)
	{
		EarUpdate();	

		if(as->midi_mode) // skip play handling, all midi playback is handled by thread in main
		{
			ZeroBuffer(buffer->left, buffer->size);
			ZeroBuffer(buffer->right, buffer->size);
			as->AdvanceTick(buffer->size/8); // to keep time ticking along (and play indicators moving...)
			break;
		}
		
		LogPrint("pa_callback 2b");



///// crash between here...

		if(as->delta_volume!=0.0f) // handle fades (should maybe be done in AudioStream...)
		{
			as->master_volume+=as->delta_volume;
			if(as->master_volume>as->default_volume)
			{
				as->master_volume=as->default_volume;
				as->delta_volume=0;
			}
			if(as->master_volume<0.0f)
			{
				as->master_volume=0.0f;
				as->delta_volume=0;
			}
		}

////// ...and here!?  (for drzool, unless he ran the wrong exe)


		
		LogPrint("pa_callback 2b1");

		StereoBufferP sbuffer;
		sbuffer.left=buffer->left+substep*subsize;
		sbuffer.right=buffer->right+substep*subsize;
		sbuffer.size=subsize;
		sbuffer.undefined=true;

		LogPrint("pa_callback 2b2");
		
//		StereoBufferP in_sbuffer;
//		in_sbuffer.left=in_buffer->left+substep*subsize;
//		in_sbuffer.right=in_buffer->right+substep*subsize;
//		in_sbuffer.size=subsize;
//		in_sbuffer.undefined=false;
//		SetWaveInBuffer(in_buffer->left+substep*subsize, in_buffer->right+substep*subsize);
//		SetWaveInBuffer(&in_sbuffer);

		if(as->metronome>0)
		{
			if(as->metrocount<=0)
			{
				as->blipcount+=4;
				as->blipvol=0.5f;
				as->blipangle=0.0f;
				as->metrocount+=as->metronome;
			}
			as->metrocount-=subsize;
		}
		
		LogPrint("pa_callback 2b3");

	//	as->song->PlayStep((as->globaltick+buffer->size-as->song->playstart)/GetTempo(), as);
		if(as->song!=NULL)
			as->song->PlayStep(sbuffer.size*1600/GetTempo(), as);

		LogPrint("pa_callback 2b4");

		if(as->file_output && as->record_song && !as->song->playing)
			rec_pstop=substep*subsize;

		LogPrint("pa_callback 2c");

		// handle all playing parts (trigger/release instruments)
		for(int i=0;i<as->numparts;i++)
		{
			Part *part=as->parts[i].part;
			gear_instrument *instr=part->gearstack->instrument;

//			if(instr->Midi()) // don't handle parts with midi instruments here, they are played using a thread in main.cpp
//				continue;

	//		float curtime=(as->globaltick+buffer->size-as->parts[i].start)/44.100f;
	//		int curtime=(as->globaltick+buffer->size-as->parts[i].start)/GetTempo();
	//		int curtime=as->song->playtime-as->parts[i].start;
			part->playtime+=sbuffer.size*1600/GetTempo();
			int curtime=part->playtime;
			Trigger *trig=part->GetTrigger(curtime);
			while(trig!=NULL) // consume all due triggers
			{				
//				instr->Trigger(trig->note, trig->volume, trig->duration);
				instr->Trigger(trig->note, trig->volume, trig->duration, trig->slidenote, trig->slidelength);
	
				if(part->Ended())
					as->parts[i].expired=true;
				
				trig=part->GetTrigger(curtime);
			}
		}
		int premoved=0;
		for(int i=0;i<as->numparts;i++) // clean up expired parts
		{
			if(i+premoved>=as->numparts)
				break;
			if(as->parts[i+premoved].expired)
				premoved++;
			if(premoved>0)
				as->parts[i]=as->parts[i+premoved];
		}
		as->numparts-=premoved;

		LogPrint("pa_callback 2d");

//		float debugtime1=as->perftimer->GetElapsedSeconds(0);


		// gather audio from all active instruments
		StereoBufferP ibuffers[512]; // maximum number of concurrently playing instruments
		bool add_to_mix[512];
		int numbuffers=0;
		int maxi=0;
		
//		try
//		{

		for(int gi=0;gi<as->num_gearstacks;gi++)
		{
			GearStack *gs=as->gearstacks[gi];
			if(gs==NULL || gs->instrument==NULL) // this stack has recently been removed, ignore it
				continue;
			gs->instrument->satisfied=false;
		}

		LogPrint("pa_callback 2e");

		int debug_sanity=0;
		bool all_satisfied=false;
		while(!all_satisfied)
		{
			debug_sanity++;
			if(debug_sanity>100 || numbuffers>400) // DEBUG...
			{
				LogPrint("callback: insanity (numbuffers=%i)", numbuffers);
				break;
			}

			all_satisfied=true;
			for(int gi=0;gi<as->num_gearstacks;gi++)
			{
				GearStack *gs=as->gearstacks[gi];
				if(gs==NULL || gs->instrument==NULL) // this stack has recently been removed, ignore it
					continue;
	
				if(gs->instrument->satisfied) // already produced output for this gearstack
					continue;
	
				add_to_mix[numbuffers]=true;
				if(gs->instrument->override>0) // this gearstack has its output routed somewhere and shouldn't produce sound
				{
					add_to_mix[numbuffers]=false;
					gs->instrument->override--;
				}
				else
					gs->instrument->triggerlink=NULL;

				ibuffers[numbuffers]=gs->instrument->RenderBuffer(sbuffer.size);

				for(int ei=0;ei<gs->num_effects;ei++)
				{
					int ni=gs->effects[ei]->ProcessBuffer(&ibuffers[numbuffers]);
					if(ni>maxi)
						maxi=ni;
				}

				if(!gs->instrument->satisfied) // this instrument was unable to produce output, should need signal from other gearstack, so we'll need another pass
					all_satisfied=false;
	
				if(ibuffers[numbuffers].size==0)
					continue;

				numbuffers++;
			}
		}

		LogPrint("pa_callback 2f");

		// gather audio from all active sound effects into one channel
		int firstsoundbuffer=numbuffers;
		playmu_ZeroSoundBuffer();
		ibuffers[numbuffers]=playmu_soundbuffer;
		for(int si=0;si<64;si++)
			if(playmu_sesounds[si].sound!=NULL)
				playmu_RenderSoundBuffer(playmu_sesounds[si], sbuffer.size);
		for(int si=0;si<64;si++)
			if(playmu_sesounds[si].sound!=NULL && playmu_sesounds[si].stop)
				playmu_sesounds[si].sound=NULL;
		add_to_mix[numbuffers]=true;
		numbuffers++;
/*
		if(debug_log)
			LogPrint("callback: (3) ibuffers[%i].size=%i ibuffers[].left=[%.8X] ibuffers[].right=[%.8X]", 0, ibuffers[0].size, ibuffers[0].left, ibuffers[0].right);

		if(debug_log)
			LogPrint("callback: p2");
*/
/*
		}
		catch(...)
		{
			LogPrint("exception?");
		}
*/		
/*		debug_nb=numbuffers; // DEBUG

		float debugtime2=as->perftimer->GetElapsedSeconds(0);
		if(debugtime2-debugtime1>50.0f/44100)
			LogPrint("callback: cpu alert (%.2f) [%i iterations]", (debugtime2-debugtime1)/(128.0f/44100), maxi);
*/
		
//		as->FlushGearstackList();
		
		// possible optimization/redesign: some effects could be applied once to the merged output of several instruments
		// (negligible benefit vs heavy redesign?)
	
		LogPrint("pa_callback 2g");

		// accumulate buffers
		ZeroBuffer(sbuffer.left, sbuffer.size);
		ZeroBuffer(sbuffer.right, sbuffer.size);

		LogPrint("pa_callback 2h");

//		if(debug_log)
//			LogPrint("callback: p3");

		for(int bi=0;bi<numbuffers;bi++)
		{
			int bsize=ibuffers[bi].size;
			if(bsize==0 || !add_to_mix[bi])
				continue;

//			if(debug_log)
//				LogPrint("callback: ibuffers[%i].size=%i ibuffers[].left=[%.8X] ibuffers[].right=[%.8X]", bi, ibuffers[bi].size, ibuffers[bi].left, ibuffers[bi].right);

			float *bp=sbuffer.left;
			float *sp=ibuffers[bi].left;
			for(int i=0;i<bsize;i+=8)
			{
				(*bp++)+=*sp++;
				(*bp++)+=*sp++;
				(*bp++)+=*sp++;
				(*bp++)+=*sp++;
				(*bp++)+=*sp++;
				(*bp++)+=*sp++;
				(*bp++)+=*sp++;
				(*bp++)+=*sp++;
			}
			bp=sbuffer.right;
			sp=ibuffers[bi].right;
			for(int i=0;i<bsize;i+=8)
			{
				(*bp++)+=*sp++;
				(*bp++)+=*sp++;
				(*bp++)+=*sp++;
				(*bp++)+=*sp++;
				(*bp++)+=*sp++;
				(*bp++)+=*sp++;
				(*bp++)+=*sp++;
				(*bp++)+=*sp++;
			}

			if(bi==firstsoundbuffer-1) // accumulated all music buffers, scale them with song master volume before adding sound buffers
			{
				for(int i=0;i<sbuffer.size;i++)
					sbuffer.left[i]*=as->master_volume;
				for(int i=0;i<sbuffer.size;i++)
					sbuffer.right[i]*=as->master_volume;
			}
		}
		
		LogPrint("pa_callback 2i");

//		if(debug_log)
//			LogPrint("callback: p4");

		if(as->blipcount>0)
		{
			as->blipcount--;
			float *lp=sbuffer.left;
			float *rp=sbuffer.right;
			float angle=as->blipangle;
			float bvol=as->blipvol;
			float dbvol=0.5f/(sbuffer.size*4);
			float metvol=2.0f*as->metronome_vol;
			for(int i=0;i<sbuffer.size;i++)
			{
				float sample=sin(angle); // used to get a periodic signal which is then quantized to squarewave...
				if(sample>=0.0f)
					sample=bvol;
				else
					sample=-bvol;
				sample*=metvol;
				(*lp++)+=sample;
				(*rp++)+=sample;
				angle+=8000.0f/44100;
				bvol-=dbvol;
			}
			as->blipangle=angle;
			as->blipvol=bvol;
		}

		// increment sample counter, only count every 8 samples to make the int range last longer (8 days?)
		as->AdvanceTick(subsize/8);
//		as->globaltick+=subsize/8;
//		as->songtick+=subsize/8;

//		as->globaltick++;
	}

	LogPrint("pa_callback 3");

//	if(debug_log)
//		LogPrint("callback: p5");

	// write to output buffer
	as->peak_left=0.0f;
	as->peak_right=0.0f;
	for(int i=0;i<buffer->size;i++)
	{
//		float sleft=buffer->left[i]*as->master_volume;
//		float sright=buffer->right[i]*as->master_volume;
		float sleft=buffer->left[i]*playmu_sysvolume;
		float sright=buffer->right[i]*playmu_sysvolume;

		float peak=fabs(sleft);
		if(peak>as->peak_left) as->peak_left=peak;
		if(peak>1.0f) as->clip_left=1000;
		peak=fabs(sright);
		if(peak>as->peak_right) as->peak_right=peak;
		if(peak>1.0f) as->clip_right=1000;

		// clip explicitly to make sure it's heard in the output even if speaker volume is turned down		
		if(sleft>1.0f) sleft=1.0f;
		if(sleft<-1.0f) sleft=-1.0f;
		if(sright>1.0f) sright=1.0f;
		if(sright<-1.0f) sright=-1.0f;

		if(as->file_output)
		{
			float fsample=sleft;
//			if(fsample>1.0f) fsample=1.0f;
//			if(fsample<-1.0f) fsample=-1.0f;
			short isample=(short)(fsample*65530/2);
			fwrite(&isample, 1, 2, as->foutput);
			fsample=sright;
//			if(fsample>1.0f) fsample=1.0f;
//			if(fsample<-1.0f) fsample=-1.0f;
			isample=(short)(fsample*65530/2);
			fwrite(&isample, 1, 2, as->foutput);
			as->file_stereosampleswritten++;

			if(i+1==rec_pstop)
				as->StopFileOutput();
		}

		*out++=sleft*as->speaker_volume;
		*out++=sright*as->speaker_volume;
	}
	as->clip_left-=buffer->size/44;
	if(as->clip_left<0)
		as->clip_left=0;
	as->clip_right-=buffer->size/44;
	if(as->clip_right<0)
		as->clip_right=0;

	LogPrint("pa_callback 4 - done");
	LogDisable();

//	if(debug_log)
//		LogPrint("callback: p6");

	// measure cpu usage
/*	float ctime=as->perftimer->GetElapsedSeconds(0);
	as->dtime+=ctime;
	as->dtime_num++;
	if(as->dtime_num>=30)
	{
		as->cpu_usage=(as->dtime*100/as->dtime_num)/((float)buffer->size/44100);
		if(as->cpu_usage<0.0f) as->cpu_usage=0.0f;
		as->dtime=0.0f;
		as->dtime_num=0;
	}
*/	
/*	if(ctime>((float)buffer->size/44100)/3)
	{
		LogPrint("crap: [%.2f] numbuffers=%i (%i, %i, %i, ...)", ctime/((float)buffer->size/44100), debug_nb, debug_bs[0], debug_bs[0], debug_bs[0]);
	}
*/
//	if(as->cpu_usage>30.0f)
//	{
//		LogPrint("callback: cpu=%.2f", as->cpu_usage);
//	}
//	LogPrint("AudioStream: pa_callback done");

	return 0;
}
