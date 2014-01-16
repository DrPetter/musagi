#include "part.h"
#include "song.h"
#include "AudioStream.h"


void Part::PlayButton(void *astream)
{
	AudioStream *audiostream=(AudioStream*)astream;

	if(!pplay) // about to start
	{
		prec=false;
		pplay_start=audiostream->GetTick(0);
		pplay_offset=pplay_start;
		vlooped=false;
		vloopcounter=0;
		Rewind(99999);
		while(GetTrigger(-1)) // skip past all triggers before the first one
		{}
		audiostream->AddPart(this);
	}
	pplay=!pplay;
	if(!pplay) // stopped
	{
		audiostream->RemovePart(this);
		gearstack->instrument->ReleaseAll();
	}

	audiostream->GetTick(3); // reset song tick
};

void Song::PlayPrep(AudioStream *audiostream) // flush past parts
{
	LogPrint("playprep");
	for(int i=playindex;i<numparts;i++)
	{
		if(parts[i].ypos>=58) // part is outside active song area
			continue;
		if(parts[i].part==NULL)
			LogPrint("ERROR: parts[%i].part==NULL", i);
		if(!parts[i].triggered && parts[i].start<=playtime-20) // -20 == arbitrary offset to reduce risk of timing errors at part beginning
		{
			playindex=i+1;
			parts[i].triggered=true;
//			LogPrint("PlayPrep: encountered part %i", i);
			if(parts[i].start+parts[i].part->LoopedLength(parts[i].loops)>playtime-20) // still within this part, start playing
			{
//				parts[i].part->Rewind(parts[i].part->loops-1);
				if(!muted[parts[i].ypos-1])
				{
					parts[i].part->Rewind(parts[i].loops-1);
					parts[i].part->playtime=playtime-parts[i].start;
//					LogPrint("PlayPrep: moving into part %i (part.playtime=%i)", i, parts[i].part->playtime);
					while(parts[i].part->GetTrigger(parts[i].part->playtime-1)) // flush past triggers (but not the immediately imminent ones)
					{
//						LogPrint("PlayPrep: flushed one trigger");
					}
					audiostream->AddPart(parts[i].part);
				}
			}
//			else
//				LogPrint("PlayPrep: skipped part %i", i);
		}
	}
	audiostream->GetTick(3); // reset song tick
	playprepped=true;
	LogPrint("playprep done");
};

void Song::PlayStep(int newtime, AudioStream *audiostream)
{
	LogPrint("playstep");

	if(playing && playtime+newtime>=playto)
	{
		bool was_recknobs=recknobs;
		Stop();
		if(loop && !was_recknobs && loopfrom<playto)
		{
			LogPrint("song: looped - error=%i, deltatime=%i", playtime+newtime-playto, newtime);
			Play(loopfrom); // can cause timing glitches... not noticable though (less than one sub-buffer)
		}
//			Play(playtime+newtime-(playto-loopfrom));
	}
	if(!playprepped)
		PlayPrep(audiostream);
	if(!playing)
	{
		LogPrint("playstep done (1)");
		return;
	}

//	playtime=newtime;

//	LogPrint("song: playtime=%.3f", playtime);

	for(int i=playindex;i<numparts;i++)
	{
		if(parts[i].ypos>=58) // part is outside active song area
			continue;
		if(parts[i].part==NULL)
			LogPrint("ERROR: parts[%i].part==NULL", i);
		if(!parts[i].triggered && parts[i].start<=playtime-20) // -20 == arbitrary offset to reduce risk of timing errors at part beginning
		{
//			LogPrint("song: adding new part (%.8X)", parts[i].part);
			playindex=i+1;
			if(!muted[parts[i].ypos-1])
			{
				parts[i].triggered=true;
				parts[i].part->Rewind(parts[i].loops-1);
				parts[i].part->playtime=parts[i].start-playtime;
				audiostream->AddPart(parts[i].part);
			}
		}
		else
			break;
	}

	playtime+=newtime;
	
	KnobRecTick();

	LogPrint("playstep done (2)");
};

void Song::KnobRecTick() // record or play back knob values
{
	if(!playing)
		return;
	for(int kn=0;kn<numknobs;kn++)
	{
		KnobRecord *kr=&knobrecs[kn];
		if(kr->value==NULL) // this knobrec has been deleted
			continue;
		if(kr->rec && recknobs) // record value
		{
			if(*kr->value!=kr->entryvalue[kr->curentry]) // only record changes
			{
				kr->curentry++;
				if(kr->curentry+2>=kr->numalloc)
				{
					kr->numalloc+=4096;
					kr->entrytime=(int*)realloc(kr->entrytime, kr->numalloc*sizeof(int));
					kr->entryvalue=(float*)realloc(kr->entryvalue, kr->numalloc*sizeof(float));
				}
				kr->entrytime[kr->curentry]=playtime;
				kr->entryvalue[kr->curentry]=*kr->value;
				LogPrint("knobrec: knob %i=%.2f @ time=%i", kn, *kr->value, playtime);
			}
		}
		else // play back value
		{
			while(kr->curentry<kr->numentries)
			{
				if(kr->entrytime[kr->curentry]>playtime)
				{
					if(kr->curentry>0)
						kr->curentry--;
					break;
				}
				kr->curentry++;
				if(kr->curentry==kr->numentries)
					LogPrint("knobrec: knob %i end of record @ time=%i", kn, playtime);
			}
			if(kr->curentry<kr->numentries)
			{
				if(kr->entryvalue[kr->curentry]==-1.0f) // end of a record section, use default value
					*kr->value=kr->entrydefault;
				else
					*kr->value=kr->entryvalue[kr->curentry];
			}
			else // outside recording, use default value
				*kr->value=kr->entrydefault;
		}
	}
};

