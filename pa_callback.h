#include <portaudio.h>

static int pa_callback(void *inputBuffer, void *outputBuffer,
			unsigned long framesPerBuffer, PaTimestamp outTime, void *userData)
{
	float *out=(float*)outputBuffer;
//	float *in=(float*)inputBuffer;
	(void)outTime;
	(void)inputBuffer;
	AudioStream *as=(AudioStream*)userData;

/*	int debug_nb=-1;
	int debug_bs[10];
	for(int i=0;i<10;i++)
		debug_bs[i]=-1;
*/
	as->perftimer->SetMark(0);
	
	StereoBufferP *buffer=&as->buffer;

	if(framesPerBuffer!=buffer->size)
	{
		// allocate new buffer to match portaudio settings (should not happen)
		buffer->size=framesPerBuffer;
		LogPrint("audiostream: CAUTION! buffer size mismatch!");
		// ...
	}

	// latch critical variables (not needed since callback function always executes in full?)
//	StreamInfo li=stream_info;


	// !!!!!!! song/part stepping should be tweaked to get maximum trigger precision (maybe introduce read-ahead/delayed triggering?)


	int subsize=128;
	
	bool debug_log=false;

	for(int substep=0;substep<buffer->size/subsize;substep++)
	{
		StereoBufferP sbuffer;
		sbuffer.right=buffer->right+substep*subsize;
		sbuffer.left=buffer->left+substep*subsize;
		sbuffer.size=subsize;
		sbuffer.undefined=true;

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
		
	//	as->song->PlayStep((as->globaltick+buffer->size-as->song->playstart)/GetTempo(), as);
		if(as->song!=NULL)
			as->song->PlayStep(sbuffer.size*16/GetTempo(), as);

		// handle all playing parts (trigger/release instruments)
		for(int i=0;i<as->numparts;i++)
		{
			Part *part=as->parts[i].part;
	//		float curtime=(as->globaltick+buffer->size-as->parts[i].start)/44.100f;
	//		int curtime=(as->globaltick+buffer->size-as->parts[i].start)/GetTempo();
	//		int curtime=as->song->playtime-as->parts[i].start;
			part->playtime+=sbuffer.size*16/GetTempo();
			int curtime=part->playtime;
			Trigger *trig=part->GetTrigger(curtime);
			while(trig!=NULL) // consume all due triggers
			{
				gear_instrument *instr=part->gearstack->instrument;
				
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
	

//		float debugtime1=as->perftimer->GetElapsedSeconds(0);


		// gather audio from all active instruments
		StereoBufferP ibuffers[512]; // maximum number of concurrently playing instruments
		int numbuffers=0;
		int maxi=0;
		
//		try
//		{
			
		for(int gi=0;gi<as->num_gearstacks;gi++)
		{
			GearStack *gs=as->gearstacks[gi];
			if(gs==NULL || gs->instrument==NULL) // this stack has recently been removed, ignore it
				continue;
			ibuffers[numbuffers]=gs->instrument->RenderBuffer(sbuffer.size);
/*			if(ibuffers[numbuffers].size>0)
			{
				LogPrint("callback: audio!");
				LogPrint("callback: (1-) ibuffers[%i].size=%i ibuffers[].left=[%.8X] ibuffers[].right=[%.8X]", numbuffers, ibuffers[numbuffers].size, ibuffers[numbuffers].left, ibuffers[numbuffers].right);
				debug_log=true;
			}

			if(debug_log)
				LogPrint("callback: (-a-) ibuffers[%i].size=%i ibuffers[].left=[%.8X] ibuffers[].right=[%.8X]", 0, ibuffers[0].size, ibuffers[0].left, ibuffers[0].right);
*/
			for(int ei=0;ei<gs->num_effects;ei++)
			{
				int ni=gs->effects[ei]->ProcessBuffer(&ibuffers[numbuffers]);
				if(ni>maxi)
					maxi=ni;
			}

			if(ibuffers[numbuffers].size==0)
				continue;
/*			
			if(debug_log)
				LogPrint("callback: (-b-) ibuffers[%i].size=%i ibuffers[].left=[%.8X] ibuffers[].right=[%.8X]", 0, ibuffers[0].size, ibuffers[0].left, ibuffers[0].right);

			if(debug_log)
			{
//				LogPrint("callback: p1, ibuffers[%i].size=%i", numbuffers, ibuffers[numbuffers].size);
				LogPrint("callback: (2) ibuffers[%i].size=%i ibuffers[].left=[%.8X] ibuffers[].right=[%.8X]", numbuffers, ibuffers[numbuffers].size, ibuffers[numbuffers].left, ibuffers[numbuffers].right);
			}
*/
//			debug_bs[numbuffers]=ibuffers[numbuffers].size; // DEBUG

			numbuffers++;
		}
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
	
		// accumulate buffers
		ZeroBuffer(sbuffer.left, sbuffer.size);
		ZeroBuffer(sbuffer.right, sbuffer.size);

//		if(debug_log)
//			LogPrint("callback: p3");

		for(int bi=0;bi<numbuffers;bi++)
		{
			int bsize=ibuffers[bi].size;
			if(bsize==0)
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
		}
		
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
				float sample=sin(angle);
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

//		as->globaltick+=subsize;
	}

//	if(debug_log)
//		LogPrint("callback: p5");

	// write to output buffer
	as->peak_left=0.0f;
	as->peak_right=0.0f;
	for(int i=0;i<buffer->size;i++)
	{
		float sleft=buffer->left[i]*as->master_volume;
		float sright=buffer->right[i]*as->master_volume;
		*out++=sleft;
		*out++=sright;

		float peak=fabs(sleft);
		if(peak>as->peak_left) as->peak_left=peak;
		if(peak>1.0f) as->clip_left=1000;
		peak=fabs(sright);
		if(peak>as->peak_right) as->peak_right=peak;
		if(peak>1.0f) as->clip_right=1000;

		if(as->file_output)
		{
			float fsample=sleft;
			if(fsample>1.0f) fsample=1.0f;
			if(fsample<-1.0f) fsample=-1.0f;
			short isample=(short)(fsample*65535/2);
			fwrite(&isample, 1, 2, as->foutput);
			fsample=sright;
			if(fsample>1.0f) fsample=1.0f;
			if(fsample<-1.0f) fsample=-1.0f;
			isample=(short)(fsample*65535/2);
			fwrite(&isample, 1, 2, as->foutput);
			as->file_stereosampleswritten++;
		}

		as->globaltick++;
	}
	as->clip_left-=buffer->size/44;
	if(as->clip_left<0)
		as->clip_left=0;
	as->clip_right-=buffer->size/44;
	if(as->clip_right<0)
		as->clip_right=0;

//	if(debug_log)
//		LogPrint("callback: p6");

	// measure cpu usage
	float ctime=as->perftimer->GetElapsedSeconds(0);
	as->dtime+=ctime;
	as->dtime_num++;
	if(as->dtime_num>=30)
	{
		as->cpu_usage=(as->dtime*100/as->dtime_num)/((float)buffer->size/44100);
		as->dtime=0.0f;
		as->dtime_num=0;
	}
	
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
