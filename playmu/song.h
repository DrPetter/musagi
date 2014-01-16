#ifndef song_h
#define song_h

class Song;

#include "musagi.h"
#include "part.h"
#include "AudioStream.h"

struct SPart
{
	Part *part;
	int ypos;
	int start;
	int loops;
	bool triggered;
	char sid[10];
	bool selected;
	bool clone;
};

struct KnobRecord
{
	float *value;
	bool rec;
	int curentry;
	int numentries;
	int numalloc;
	float entrydefault;
	int *entrytime;
	float *entryvalue;
};

class Song
{
public:
	SPart *parts;
	int maxparts;
	int numparts;
	int guid;

	char info_text[4][50];

//	bool krecvis[4096];
	KnobRecord knobrecs[1024];
	int numknobs;
	bool recknobs;
	int knobrecstart;
	int kr_tempalloc;
	int *kr_temptime;
	float *kr_tempvalue;
	
	bool playing;
	bool playprepped;
	bool loop;
	int playfrom;
	int playto;
	int loopfrom;
	int playstart;
	int playtime;
	int playindex;
	int playdrag;
	
	bool muted[60];
	
	float scrollx;
	float scrolly;
	int iscrollx;
	int iscrolly;
	int scrollsize;
	int scrollfollow;
	bool spacescroll;
	
	bool showsong;
	
	int selected_instrument;
	
public:
	Song()
	{
		maxparts=4096;
		parts=(SPart*)malloc(maxparts*sizeof(SPart));
		numparts=0;
		guid=0;
		for(int i=0;i<maxparts;i++)
			parts[i].part=NULL;
		playing=false;
		playprepped=false;
		scrollx=0.0f;
		scrolly=0.0f;
		iscrollx=0;
		iscrolly=0;
		scrollsize=0;
		scrollfollow=1;
		spacescroll=false;

		showsong=true;
		selected_instrument=-1;
		
		playtime=0;
		playindex=0;
		playfrom=0;
		loop=false;
		playto=48*(16*640);
		loopfrom=32*(16*640);
		playdrag=-1;

		for(int i=0;i<60;i++)
			muted[i]=false;
		
//		for(int i=0;i<4096;i++)
//			krecvis[i]=false;
		
		for(int i=0;i<50;i++)
		{
			info_text[0][i]=0;
			info_text[0][i]=0;
			info_text[0][i]=0;
			info_text[0][i]=0;
		}
		info_text[0][0]='\0';
		info_text[1][0]='\0';
		info_text[2][0]='\0';
		info_text[3][0]='\0';
		
		numknobs=0;
		recknobs=false;
		kr_tempalloc=4096*4;
		kr_temptime=(int*)malloc(kr_tempalloc*sizeof(int));
		kr_tempvalue=(float*)malloc(kr_tempalloc*sizeof(float));
	};
	~Song()
	{
		free(parts);
		for(int i=0;i<numknobs;i++)
		{
			LogPrint("freeing knobrec %i (%i values)", i, knobrecs[i].numentries);
			free(knobrecs[i].entrytime);
			free(knobrecs[i].entryvalue);
		}
		free(kr_temptime);
		free(kr_tempvalue);
	};

	void PlayButton()
	{
		if(!playing)
			Play(playfrom);
		else
			Stop();
	};

	void SelectInstrument(int i)
	{
		selected_instrument=i;
	};

	bool KnobAtMemPos(float *pos)
	{
		for(int i=0;i<numknobs;i++)
			if(knobrecs[i].value==pos)
				return true;
		return false;
	};

	void KnobToMemPos(float *oldpos, float *newpos)
	{
		for(int i=0;i<numknobs;i++)
			if(knobrecs[i].value==oldpos)
			{
				LogPrint("song: remapped knob %i ($%.8X -> $%.8X)", i, oldpos, newpos);
				knobrecs[i].value=newpos;
				return;
			}
		LogPrint("TERROR: KnobToMemPos() failed to find the correct knob ($%.8X -> $%.8X)", oldpos, newpos);
	};

	int KnobRecState(float *value)
	{
		for(int i=0;i<numknobs;i++)
			if(knobrecs[i].value==value)
			{
				if(knobrecs[i].rec)
					return 2;
				else
					return 1;
			}
		return 0;
	};

	void KnobRecClear(float *value)
	{
		for(int i=0;i<numknobs;i++)
			if(knobrecs[i].value==value)
			{
				knobrecs[i].numentries=0;
				if(!knobrecs[i].rec) // remove this knob record if we're not just about to start recording it
					knobrecs[i].value=NULL;
				return;
			}
	};

	void KnobRecDefault(float *value)
	{
		for(int i=0;i<numknobs;i++)
			if(knobrecs[i].value==value)
			{
				knobrecs[i].entrydefault=*value;
				LogPrint("knobrec: set knob %i default to %.2f", i, *value);
				return;
			}
	};

	void KnobRecToggle(float *value)
	{
		int firstempty=-1;
		for(int i=0;i<numknobs;i++)
		{
			if(firstempty==-1 && knobrecs[i].value==NULL)
				firstempty=i;
			if(knobrecs[i].value==value)
			{
				LogPrint("knobrec: toggled existing knob %i", i);
				knobrecs[i].rec=!knobrecs[i].rec;
				if(knobrecs[i].numentries==0) // remove this knob record if there's no actual recording
					knobrecs[i].value=NULL;
				return;
			}
		}
		if(firstempty>-1)
			i=firstempty;
		// knob has no record, create one
		LogPrint("knobrec: toggle -> adding knob %i (value %.8X)", i, value);
		knobrecs[i].value=value;
		knobrecs[i].entrydefault=*value;
		knobrecs[i].rec=true;
		knobrecs[i].numalloc=0;
		knobrecs[i].numentries=0;
		knobrecs[i].entrytime=NULL;
		knobrecs[i].entryvalue=NULL;
		if(i==numknobs)
			numknobs++;
	};

	void KnobRecTick();

//	void SelectPart(Part *part)
	void SelectPart(char *sid)
	{
		for(int i=0;i<numparts;i++)
		{
			if(strcmp(parts[i].sid, sid)==0)
//				parts[i].part->selected=true;
//			else
//				parts[i].part->selected=false;
				parts[i].selected=true;
			else
				parts[i].selected=false;
		}
	};
	
	void DeselectPart()
	{
		for(int i=0;i<numparts;i++)
			parts[i].selected=false;
	};

	Part* GetSelectedPart()
	{
		for(int i=0;i<numparts;i++)
			if(parts[i].selected)
//			if(parts[i].part->selected)
				return parts[i].part;
		return NULL;
	};

	char* GetSelectedSid()
	{
		for(int i=0;i<numparts;i++)
			if(parts[i].selected)
//			if(parts[i].part->selected)
				return parts[i].sid;
		return NULL;
	};

	char* InsertPart(Part *part, int start, int ypos)
	{
//		LogPrint("song: insert@%i, numparts=%i", start, numparts);
		
		// find spot
		int i=0;
		for(i=numparts-1;i>=0;i--)
			if(parts[i].start<start)
				break;
		i++;
		numparts++;
		// move triggers to the right
		for(int j=numparts-1;j>i;j--)
			parts[j]=parts[j-1];
		// insert new
		parts[i].part=part;
		parts[i].start=start;
		parts[i].loops=1;
		parts[i].ypos=ypos;
		parts[i].clone=false;
		// search for prior sparts with this part
		for(int j=0;j<numparts;j++)
			if(j!=i && parts[j].part==part)
			{
				parts[i].clone=true;
				parts[j].clone=true;
				break;
			}
		parts[i].selected=false;
		// assign a unique name (for interface)
		int uid=guid++;
		uid%=1000000;
		sprintf(parts[i].sid, "pi%.6i", uid);
		return parts[i].sid;
	};
	
	int GenGuid()
	{
		guid++;
		return guid;
	};

//	void RemovePart(Part *part)
	bool RemovePart(char *sid)
	{
		LogPrint("song: trying to remove part \"%s\"", sid);
/*		if(part==NULL)
		{
			LogPrint("CAUTION: tried to remove NULL part");
			return false;
		}*/
		int id=-1;
		for(int i=0;i<numparts;i++)
		{
//			if(parts[i].part==part)
			if(strcmp(parts[i].sid, sid)==0)
			{
				id=i;
				break;
			}
		}
		if(id==-1) // not found
		{
			LogPrint("CAUTION: tried to remove non-existing part \"%s\"", sid);
			return false;
		}
		int instances=0;
		bool waslastpart=false;
		for(int i=0;i<numparts;i++)
			if(parts[i].part==parts[id].part)
				instances++;
		LogPrint("song:   part has %i instances", instances);
		if(instances==1) // there are no clones, so delete the actual part
		{
			waslastpart=true;
			LogPrint("song:   removed the last instance, delete the original part object");
		}
		if(instances==2) // after this removal there will only be one instance left of this part
		{
			for(int i=0;i<numparts;i++)
				if(parts[i].part==parts[id].part)
					parts[i].clone=false;
			LogPrint("song:   removed the last clone, mark the remaining instance as non-cloned");
		}
		// move triggers to the left
		LogPrint("song: reordering part triggers");
		for(int j=id;j<numparts-1;j++)
			parts[j]=parts[j+1];
		numparts--;
		LogPrint("song: part removal done");
		return waslastpart;
	};
	
	void Play(int start)
	{
		LogPrint("song: ---- playing from %i", start);
		playtime=start;
		playstart=GetTick(0);
		playindex=0;
		for(int i=0;i<numparts;i++)
			parts[i].triggered=false;
		playing=true;
		playprepped=false;
		knobrecstart=playtime;
		for(int i=0;i<numknobs;i++)
		{
			if(knobrecs[i].rec)
			{
				KnobRecord *kr=&knobrecs[i];
				if(kr->curentry+2>=kr->numalloc)
				{
					kr->numalloc+=4096;
					kr->entrytime=(int*)realloc(kr->entrytime, kr->numalloc*sizeof(int));
					kr->entryvalue=(float*)realloc(kr->entryvalue, kr->numalloc*sizeof(float));
				}
				if(kr->numentries==0) // insert a stop entry at the very beginning of the song
				{
					kr->entrytime[0]=0;
					kr->entryvalue[0]=-1.0f;
					kr->numentries=1;
				}
				kr->curentry=kr->numentries;
				kr->entrytime[kr->curentry]=playtime;
				kr->entryvalue[kr->curentry]=*kr->value;
			}
			else
				knobrecs[i].curentry=0;
		}
	};
	
	void Stop()
	{
		LogPrint("song: ---- stop");
		for(int i=0;i<numparts;i++)
			parts[i].part->Stop();
		playing=false;
		recknobs=false;
		for(int i=0;i<numknobs;i++)
		{
			if(knobrecs[i].rec)
			{
				LogPrint("song: splicing knobrec %i", i);
				KnobRecord *kr=&knobrecs[i];
				// as the last value, store what was originally recorded at this position in time
				float oldvalue=-1.0f;
				for(int j=0;j<kr->numentries;j++)
					if(kr->entrytime[j]>playtime)
					{
						if(j>0)
							j--;
						oldvalue=kr->entryvalue[j];
						break;
					}
				kr->curentry++;
				kr->entrytime[kr->curentry]=playtime;
				kr->entryvalue[kr->curentry]=oldvalue;
				int numnew=kr->curentry+1-kr->numentries;
				int firstnew=kr->numentries;
				int starttime=kr->entrytime[firstnew];
				int endtime=playtime;
				// find out how many old entries to remove (overwritten)
				int numremoved=0;
				int splicepos=kr->numentries;
				for(int j=0;j<kr->numentries;j++)
				{
					if(kr->entrytime[j]>=starttime)
					{
						if(numremoved==0)
							splicepos=j;
						numremoved++;
					}
					if(kr->entrytime[j]>=endtime)
					{
						if(numremoved>0)
							numremoved--; // this last one should remain
						break;
					}
				}
				// splice in newly recorded section into the old record
				int offset=numnew-numremoved;
				LogPrint("song: numold=%i numnew=%i numremoved=%i offset=%i splicepos=%i -> expected postsplice numentries=%i", kr->numentries, numnew, numremoved, offset, splicepos, kr->numentries+offset);
				if(offset<=0) // lost more than we gained, things need to move down
				{
					LogPrint("song: lost some");
					int spliced=0;
					for(int j=splicepos;j<kr->numentries+offset;j++)
					{
						if(spliced<numnew)
						{
							kr->entrytime[j]=kr->entrytime[firstnew+spliced];
							kr->entryvalue[j]=kr->entryvalue[firstnew+spliced];
							spliced++;
						}
						else
						{
							if(offset==0)
							{
								j=kr->numentries+offset;
								break;
							}
							kr->entrytime[j]=kr->entrytime[j-offset];
							kr->entryvalue[j]=kr->entryvalue[j-offset];
						}
					}
					kr->numentries=j;
				}
				else // gained more than we lost, things need to move up
				{
					LogPrint("song: gained some");
					while(kr->numentries+numnew>kr_tempalloc)
					{
						kr_tempalloc+=4096*4;
						free(kr_temptime);
						kr_temptime=(int*)malloc(kr_tempalloc*sizeof(int));
						free(kr_tempvalue);
						kr_tempvalue=(float*)malloc(kr_tempalloc*sizeof(float));
					}
					int ti=0;
					for(int j=0;j<splicepos;j++) // preserve old unaffected entries
					{
						kr_temptime[ti]=kr->entrytime[j];
						kr_tempvalue[ti]=kr->entryvalue[j];
						ti++;
					}
					for(int j=0;j<numnew;j++) // insert new entries
					{
						kr_temptime[ti]=kr->entrytime[firstnew+j];
						kr_tempvalue[ti]=kr->entryvalue[firstnew+j];
						ti++;
					}
					for(int j=0;j<kr->numentries-splicepos-numremoved;j++) // preserve old entries left behind new ones
					{
						kr_temptime[ti]=kr->entrytime[splicepos+numremoved+j];
						kr_tempvalue[ti]=kr->entryvalue[splicepos+numremoved+j];
						ti++;
					}
					kr->numentries=ti;
					for(int j=0;j<kr->numentries;j++) // copy entries over to permanent storage
					{
						kr->entrytime[j]=kr_temptime[j];
						kr->entryvalue[j]=kr_tempvalue[j];
					}
				}
				LogPrint("song: postsplice numentries=%i", kr->numentries);
			}
			knobrecs[i].rec=false;
		}
	};
	
	void Load(FILE *file, Part **gparts, int numgparts)
	{
		kfread2(&guid, 1, sizeof(int), file);
		kfread2(&playfrom, 1, sizeof(int), file);
		kfread2(&playto, 1, sizeof(int), file);
		kfread2(&loopfrom, 1, sizeof(int), file);
		kfread2(&loop, 1, sizeof(bool), file);
		kfread2(&scrollx, 1, sizeof(float), file);
		kfread2(&scrolly, 1, sizeof(float), file);
		kfread2(info_text[0], 50, sizeof(char), file);
		kfread2(info_text[1], 50, sizeof(char), file);
		kfread2(info_text[2], 50, sizeof(char), file);
		kfread2(info_text[3], 50, sizeof(char), file);
		kfread2(&numparts, 1, sizeof(int), file);
		for(int i=0;i<numparts;i++)
		{
			int pindex=0;
			kfread2(&pindex, 1, sizeof(int), file);
			kfread2(&parts[i].start, 1, sizeof(int), file);
			kfread2(&parts[i].loops, 1, sizeof(int), file);
			kfread2(&parts[i].ypos, 1, sizeof(int), file);
			kfread2(&parts[i].clone, 1, sizeof(bool), file);
			kfread2(parts[i].sid, 10, sizeof(char), file);
//			parts[i].part=gparts[pindex];
			for(int pi=0;pi<numgparts;pi++)
				if(gparts[pi]!=NULL)
				{
//					LogPrint("song: looking for %i, is %i it?", pindex, gparts[pi]->gindex);
					if(gparts[pi]->gindex==pindex)
					{
//						LogPrint("song: found part %i", pi);
						parts[i].part=gparts[pi];
						break;
					}
				}
			parts[i].triggered=false;
			parts[i].selected=false;
//			LogPrint("song: spart start=%i, ypos=%i", parts[i].start, parts[i].ypos);
		}

		// unmute tracks (should actually load them from the file)
		for(int i=0;i<60;i++) // why was this 58?
			muted[i]=false;
		if(GetFileVersion()>=4) // mute tracks supported
			for(int i=0;i<60;i++)
				kfread2(&muted[i], 1, sizeof(bool), file);

		for(int i=0;i<numknobs;i++)
		{
//			LogPrint("freeing knobrec %i (%i values)", i, knobrecs[i].numentries);
			free(knobrecs[i].entrytime);
			free(knobrecs[i].entryvalue);
		}

		kfread2(&numknobs, 1, sizeof(int), file);
		for(int i=0;i<numknobs;i++)
		{
			kfread(&knobrecs[i], 1, sizeof(KnobRecord), file);
			knobrecs[i].entrytime=(int*)realloc(NULL, (knobrecs[i].numentries+4096)*sizeof(int));
			knobrecs[i].entryvalue=(float*)realloc(NULL, (knobrecs[i].numentries+4096)*sizeof(float));
			kfread2(knobrecs[i].entrytime, knobrecs[i].numentries, sizeof(int), file);
			kfread2(knobrecs[i].entryvalue, knobrecs[i].numentries, sizeof(float), file);
//			LogPrint("song: read knob %i (%i entries, value=$%.8X)", i, knobrecs[i].numentries, knobrecs[i].value);
		}

		LogPrint("song: loaded %i sparts", numparts);

		float loadedscrollx=scrollx;
		scrollsize=0;
		AdjustScrollsize();
		scrollx=loadedscrollx;
	};

	void AdjustScrollsize()
	{
	};
	
	void PlayPrep(AudioStream *audiostream);

	void PlayStep(int dtime, AudioStream *audiostream);
};

#endif

