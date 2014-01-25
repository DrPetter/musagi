// X.M.XN.XB.W.

#ifndef part_h
#define part_h

//class Part;

#include "musagi.h"
#include "dui.h"
//#include "AudioStream.h"

class Part
{
private:
	int maxtriggers;
	int numtriggers;
	int curtrigger;
	bool playing;

	int prec_start;
	int prec_curtime;
	bool pplay;
	int pplay_start;
	int pplay_curtrigger;
	int pplay_offset; // visualization
	float pscrollx;
	float pscrolly;
	int scrollsize;
	
	int edit_curtrigger;
	
public:
	int gindex;

	Trigger *triggers;

	int playtime;
	
	GearStack *gearstack;
	int gearid;

	char name[64];
	bool hidden;
//	bool iconify;
	bool selected;
	bool popup;
	bool wasactive;
	bool wantrelease;
	
	int winx;
	int winy;
	int winw;
	int winh;
	bool prec;
	
	int msnum;
	int msitriggers[512];
	Trigger mstriggers[512];
	int mst1;
	int msn1;
	bool ctrl_move;
	int slide_origin;
	
	bool sbox_active;
	int sbox_x1, sbox_y1, sbox_x2, sbox_y2;
	
//	int loops;
	int loopcounter;
	int loopsleft;
	int looptime;
	bool placing_loop;
	bool vlooped;
	int vloopcounter;

	Part(int index)
	{
                int i;
		gindex=index;
		
		maxtriggers=1024;
		triggers=(Trigger*)malloc(maxtriggers*sizeof(Trigger));
		numtriggers=0;
		curtrigger=0;
		playing=false;
//		loops=1;
		loopcounter=0;
		loopsleft=0;
		looptime=0;
		placing_loop=false;
		vlooped=false;
		vloopcounter=0;
		
		playtime=0;
		
		hidden=true;
//		iconify=false;
		popup=false;
		selected=false;
		wasactive=false;
		wantrelease=false;
		
		gearstack=NULL;
		gearid=0;

		winx=100+rnd(300);
		winy=100+rnd(200);
		winw=450;
		winh=300;
		prec=false;
		prec_start=0;
		prec_curtime=0;
		pplay=false;
		pplay_start=0;
		pplay_curtrigger=0;
		pscrollx=0.0f;
		pscrolly=0.75f;
		scrollsize=900;
		msnum=0;
		ctrl_move=false;
		slide_origin=-1;

		sbox_active=false;

		// generate a random name
		for(i=0;i<64;i++)
			name[i]=0;
		char vowels[7]="aoueiy";
		char consonants[21]="bcdfghjklmnpqrstvwxz";
		int vowel=rnd(1);
		for(i=0;i<6;i++)
		{
			if(vowel)
				name[i]=vowels[rnd(5)];
			else
				name[i]=consonants[rnd(19)];
			vowel=1-vowel;
			if(i>2 && rnd(5)==0)
				break;
		}
		name[i]='\0';
		
		edit_curtrigger=-1;
	};
	~Part()
	{
		free(triggers);
	};

	void Popup()
	{
		hidden=false;
		popup=true;
	};
	
	void CopyOf(Part *src)
	{
		char temp[64];
		strcpy(temp, src->name);
		sprintf(name, "%s%.3i", temp, rnd(999));

		gearstack=src->gearstack;
		gearid=src->gearid;

//		loops=src->loops;
		looptime=src->looptime;

		numtriggers=src->numtriggers;
		for(int i=0;i<numtriggers;i++)
			triggers[i]=src->triggers[i];
	};

	void SetGearStack(GearStack *gs, int gid)
	{
		gearstack=gs;
		gearid=gid;
	};

	void Clear()
	{
		numtriggers=0;
	};
	
	int Length()
	{
		int length=0;
		if(numtriggers>0)
			length=triggers[numtriggers-1].start+triggers[numtriggers-1].duration;
		if(length<2200*16)
			length=2200*16;
		return length;
	};

	int LoopedLength(int loops)
	{
		int length=0;
		if(numtriggers>0)
			length=triggers[numtriggers-1].start+triggers[numtriggers-1].duration;
		if(length<2200*16)
			length=2200*16;
		length+=looptime*loops;
		return length;
	};

	void RecTrigger(int note, float volume, bool atomic)
	{
		if(prec && prec_curtime/320+4>=0)
		{
			int rtime=(prec_curtime/320+4) /8*8 *320;
			int ti=InsertTrigger(note, rtime, 0, volume);
			if(atomic)
				triggers[ti].duration=1;
		}
	};

	void RecRelease(int note)
	{
		if(prec)
		{
			int rtime=(prec_curtime/320+4) /8*8 *320;
			for(int i=0;i<numtriggers;i++)
				if(triggers[i].note==note && triggers[i].duration==0)
					triggers[i].duration=rtime-triggers[i].start+1;
		}
	};
	
	int InsertTrigger(int note, int start, int duration, float volume)
	{
		// reallocate if necessary

		// find spot
		int i=0;
		for(i=numtriggers-1;i>=0;i--)
			if(triggers[i].start<start)
				break;
		i++;
		numtriggers++;
		// move triggers to the right
		for(int j=numtriggers-1;j>i;j--)
			triggers[j]=triggers[j-1];
		// insert new
		triggers[i].note=note;
		triggers[i].start=start;
		triggers[i].duration=duration;
		triggers[i].slidenote=-1;
		triggers[i].slidelength=-1;
		triggers[i].volume=volume;
		triggers[i].selected=false;
		
		if(i<=curtrigger && curtrigger<numtriggers-1)
			curtrigger++;
		
		return i;
	};

	void CloseTrigger(int index, int duration)
	{
		triggers[index].duration=duration;
	};

	void RemoveTrigger(int index)
	{
		// move down (overwrite)
		for(int i=index;i<numtriggers-1;i++)
			triggers[i]=triggers[i+1];
		numtriggers--;

		if(index<curtrigger && curtrigger>0)
			curtrigger--;
	};

	int FindTrigger(int dtime, int note)
	{
		// return index of trigger matching given parameters
		for(int i=0;i<numtriggers;i++)
			if(note==triggers[i].note && dtime>=triggers[i].start && dtime<=triggers[i].start+triggers[i].duration)
				return i;
		return -1; // no hit
	};
	
	void Rewind(int numloops)
	{
//		LogPrint("part: rewind");
		loopcounter=0;
		loopsleft=numloops;
		curtrigger=0;
		playing=true;
		playtime=0;
	};
	
	void Stop()
	{
		curtrigger=0;
		playing=false;
		gearstack->instrument->ReleaseAll();
	};

	Trigger* GetTrigger(int dtime)
	{
//		LogPrint("part: dtime=%.1f (%.1f)", dtime, triggers[curtrigger].start);
		if(playing)
		{
			if(dtime>=looptime*vloopcounter && looptime>0)
//			if(dtime>=looptime*loopcounter && looptime>0)
				vlooped=true; // bleh, just to hint the gui so that the play indicator can jump back

			if(curtrigger<numtriggers && numtriggers>0 && dtime>=triggers[curtrigger].start+looptime*loopcounter)
			{
	//			LogPrint("part: trigger!");
				int irtrigger=curtrigger;
				
				curtrigger++;
				if(loopsleft>0 && looptime>0)
				{
					if(curtrigger>=numtriggers || triggers[curtrigger].start>looptime)
					{
						loopsleft--;
						loopcounter++;
						curtrigger=0;
					}
//					playtime-=looptime;
				}
				else if(curtrigger>=numtriggers)
				{
/*					if(loopsleft>0 && looptime>0)
					{
						loopsleft--;
						loopcounter++;
						curtrigger=0;
	//					playtime-=looptime;
					}
					else*/
						playing=false;
				}
				return &triggers[irtrigger];
			}
		}
		return NULL;
	};
	
	bool Ended()
	{
		return !playing;
	};

	void PlaceWindow(int x, int y)
	{
		winx=x;
		winy=y;
	};

	void AdjustScrollsize()
	{
		int newscrollsize=(int)(Length()/320)+winw*2;
		if(newscrollsize>scrollsize)
		{
			if(winh-1<560) // vertical scroll bar
			{
				float spos=(scrollsize-(winw-109))*pscrollx;
				pscrollx=spos/(newscrollsize-(winw-109));
				scrollsize=newscrollsize;
			}
			else // no vertical scroll bar
			{
				float spos=(scrollsize-(winw-101))*pscrollx;
				pscrollx=spos/(newscrollsize-(winw-101));
				scrollsize=newscrollsize;
			}
		}
	};

	void Save(FILE *file)
	{
		kfwrite2(name, 64, sizeof(char), file);

		kfwrite2(&gindex, 1, sizeof(int), file);
		kfwrite2(&looptime, 1, sizeof(int), file);
		kfwrite2(&selected, 1, sizeof(bool), file);
		kfwrite2(&hidden, 1, sizeof(bool), file);
		kfwrite2(&winx, 1, sizeof(int), file);
		kfwrite2(&winy, 1, sizeof(int), file);
		kfwrite2(&winw, 1, sizeof(int), file);
		kfwrite2(&winh, 1, sizeof(int), file);
		kfwrite2(&pscrollx, 1, sizeof(float), file);
		kfwrite2(&pscrolly, 1, sizeof(float), file);

		kfwrite2(&numtriggers, 1, sizeof(int), file);
		for(int i=0;i<numtriggers;i++)
			kfwrite2(&triggers[i], 1, sizeof(Trigger), file);
		kfwrite2(&gearid, 1, sizeof(int), file);
	};

	void Load(FILE *file, GearStack *gstacks, int version)
	{
		kfread2(name, 64, sizeof(char), file);

		kfread2(&gindex, 1, sizeof(int), file);
		kfread2(&looptime, 1, sizeof(int), file);
		kfread2(&selected, 1, sizeof(bool), file);
		kfread2(&hidden, 1, sizeof(bool), file);
		kfread2(&winx, 1, sizeof(int), file);
		kfread2(&winy, 1, sizeof(int), file);
		if(version>0)
		{
			kfread2(&winw, 1, sizeof(int), file);
			kfread2(&winh, 1, sizeof(int), file);
		}
		kfread2(&pscrollx, 1, sizeof(float), file);
		kfread2(&pscrolly, 1, sizeof(float), file);

		kfread2(&numtriggers, 1, sizeof(int), file);
		for(int i=0;i<numtriggers;i++)
		{
//			LogPrint("part: loading trigger %i", i);
			kfread2(&triggers[i], 1, sizeof(Trigger), file);
		}
		kfread2(&gearid, 1, sizeof(int), file);
//		LogPrint("part: gearstack index=%i", gearid);
		
		gearstack=&gstacks[gearid];

		float oldscrollx=pscrollx;
		AdjustScrollsize();
		pscrollx=oldscrollx;

//		hidden=true;
	};
	
	bool SaveContent(FILE *file)
	{
		fwrite("PMU01", 5, 1, file);
		fwrite(&looptime, 1, sizeof(int), file);
		fwrite(&numtriggers, 1, sizeof(int), file);
		for(int i=0;i<numtriggers;i++)
			fwrite(&triggers[i], 1, sizeof(Trigger), file);
		return true;
	};

	bool LoadContent(FILE *file)
	{
		char vstring[6];
		vstring[5]='\0';
		fread(vstring, 5, 1, file);
		if(strcmp(vstring, "PMU01")!=0)
			return false;
		fread(&looptime, 1, sizeof(int), file);
		fread(&numtriggers, 1, sizeof(int), file);
		for(int i=0;i<numtriggers;i++)
			fread(&triggers[i], 1, sizeof(Trigger), file);
		return true;
	};

//	void DoInterface(DUI *dui, AudioStream *audiostream);
	bool DoInterface(DUI *dui, void *audiostream);
};

#endif
