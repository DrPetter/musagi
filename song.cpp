#include "song.h"
#include "glkit_global.h"

void Song::PlayPrep(AudioStream *audiostream) // flush past parts
{
	for(int i=playindex;i<numparts;i++)
	{
		if(parts[i].ypos>57) // part is outside active song area
			continue;
		if(parts[i].part==NULL)
			LogPrint("ERROR: parts[%i].part==NULL", i);
		if(!parts[i].triggered && parts[i].start<=playtime-20) // -20 == arbitrary offset to reduce risk of timing errors at part beginning
		{
			playindex=i+1;
			parts[i].triggered=true;
			LogPrint("PlayPrep: encountered part %i", i);
			if(parts[i].start+parts[i].part->LoopedLength(parts[i].loops)>playtime-20) // still within this part, start playing
			{
//				parts[i].part->Rewind(parts[i].part->loops-1);
				if(!muted[parts[i].ypos-1])
				{
					parts[i].part->Rewind(parts[i].loops-1);
					parts[i].part->playtime=playtime-parts[i].start;
					LogPrint("PlayPrep: moving into part %i (part.playtime=%i)", i, parts[i].part->playtime);
					while(parts[i].part->GetTrigger(parts[i].part->playtime-1)) // flush past triggers (but not the immediately imminent ones)
					{
						LogPrint("PlayPrep: flushed one trigger");
					}
					audiostream->AddPart(parts[i].part);
				}
			}
			else
				LogPrint("PlayPrep: skipped part %i", i);
		}
	}
	playprepped=true;
};

void Song::PlayStep(int newtime, AudioStream *audiostream)
{
	if(playing && playtime+newtime>=playto)
	{
		bool was_recknobs=recknobs;
		Stop();
		if(loop && !was_recknobs)
		{
			LogPrint("song: looped - error=%i, deltatime=%i", playtime+newtime-playto, newtime);
			Play(loopfrom); // can cause timing glitches... not noticable though (less than one sub-buffer)
		}
//			Play(playtime+newtime-(playto-loopfrom));
	}
	if(!playprepped)
		PlayPrep(audiostream);
	if(!playing)
		return;

//	playtime=newtime;

//	LogPrint("song: playtime=%.3f", playtime);

	for(int i=playindex;i<numparts;i++)
	{
		if(parts[i].ypos>57) // part is outside active song area
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

void Song::DoInterface(DUI *dui)
{
	int cpart=-1;
        int i;
	for(i=0;i<numparts;i++)
		if(parts[i].selected)
		{
			cpart=i;
			break;
		}
	dui->DrawBar(5, glkitGetHeight()-53, 250, 48, dui->palette[8]);
	dui->DrawBox(5, glkitGetHeight()-53, 250, 48, dui->palette[6]);
	if(cpart!=-1)
	{
		dui->StartFlatWindow(6, glkitGetHeight()-52, 248, 46, "spset");
		char string[75];
		sprintf(string, "part: %s", parts[cpart].part->name);
		dui->DrawText(5, 2, dui->palette[6], string);
		sprintf(string, "loops: %i", parts[cpart].loops);
		dui->DrawText(5, 20, dui->palette[6], string);
		dui->DrawText(77, 17, dui->palette[6], "loop up");
		if(dui->DoButton(54, 13, "songb_loop1", "increase number of loops for this part"))
			parts[cpart].loops++;
		dui->DrawText(77, 27, dui->palette[6], "loop down");
		if(dui->DoButton(54, 27, "songb_loop2", "decrease number of loops for this part") && parts[cpart].loops>1)
			parts[cpart].loops--;

		if(dui->rmbclick && dui->MouseInZone(10, glkitGetHeight()-52, 240, 10))
			dui->EditString(parts[i].part->name);
		dui->EndWindow();
	}

	dui->DrawText(glkitGetWidth()-582, glkitGetHeight()-32, dui->palette[6], "+4 bars");
	if(dui->DoButton(glkitGetWidth()-605, glkitGetHeight()-33, "songb_p4b", "insert 4 bars in timeline at current start position"))
	{
		for(int i=0;i<numparts;i++)
			if(parts[i].start>=playfrom)
				parts[i].start+=16*4*640;
//				parts[i].start=(parts[i].start/64+16*4+2)/16*16*64;
	}

	dui->DrawText(glkitGetWidth()-582, glkitGetHeight()-20, dui->palette[6], "-4 bars");
	if(dui->DoButton(glkitGetWidth()-605, glkitGetHeight()-21, "songb_m4b", "remove 4 bars in timeline at current start position (if empty)"))
	{
		bool ok=true;
		for(int i=0;i<numparts;i++)
			if(parts[i].start>=playfrom && parts[i].start<playfrom+16*4*640)
			{
				ok=false;
				break;
			}
		if(ok)
			for(int i=0;i<numparts;i++)
				if(parts[i].start>=playfrom)
					parts[i].start-=16*4*640;
//					parts[i].start=(parts[i].start/64-16*4+2)/16*16*64;
	}

//	dui->StartScrollArea(0, 60, 1024, 650, scrollx, dummy, 1024*8, 650, dui->palette[7], "songscroll");
//	dui->StartScrollArea(0, 60, glkitGetWidth(), glkitGetHeight()-118, scrollx, dummy, 1024*8, glkitGetHeight()-118, dui->palette[7], "songscroll");
	float zoom=0.1f/64;
	AdjustScrollsize();
	int iwidth=glkitGetWidth();
	dui->StartScrollArea(0, 60, glkitGetWidth(), glkitGetHeight()-118, scrollx, scrolly, scrollsize, 640, dui->palette[7], "songscroll");
	{
		iscrollx=(int)(scrollx*(scrollsize-glkitGetWidth()));
		iscrolly=(int)(scrolly*(640-(glkitGetHeight()-118)));

		// scroll to follow song while playing
		if(scrollfollow && playing && playtime*zoom<iscrollx)
			scrollx=(float)(playtime*zoom-16)/(scrollsize-iwidth);
		if(scrollfollow && playing && playtime*zoom>(iscrollx+iwidth/2))
			scrollx=(float)(playtime*zoom-iwidth/2)/(scrollsize-iwidth);
		
		dui->DrawBar(iscrollx-1, 0, iwidth+2, 10, dui->palette[15]); // play position bar

		for(int x=0;x<scrollsize;x+=16)
		{
			if(x<iscrollx)
				continue;
			if(x>iscrollx+glkitGetWidth())
				break;
			if(x%64==0)
				dui->DrawBar(x, 0, 1, 10*58, dui->palette[8]);
			else
				dui->DrawBar(x, 0, 1, 10*58, dui->palette[4]);
		}

//		for(int y=9;y<glkitGetHeight()-118;y+=10)
  		int y;
		for(y=9;y<10*57;y+=10)
			dui->DrawBar(iscrollx-1, y, iwidth+2, 1, dui->palette[6]);
		dui->DrawBar(iscrollx-1, y, iwidth+2, 2, dui->palette[6]);

		dui->DrawBar((int)(playto*zoom)-1, 0, 3, glkitGetHeight(), dui->palette[0]);
		if(loop)
			dui->DrawBar((int)(loopfrom*zoom)-1, 0, 3, glkitGetHeight(), dui->palette[8]);
		dui->DrawBar((int)(playfrom*zoom)-1, 0, 3, glkitGetHeight(), dui->palette[10]);

		dui->DrawBar(iscrollx-1, 0, iwidth+2, 1, dui->palette[6]); // top black line

		int sp_x=0;
		int sp_yp=0;
		bool touchpart=false;
		bool partmoved=false;	
		for(int i=0;i<numparts;i++)
		{
			int x=(int)(parts[i].start*zoom);
			int color=1;
			if(!parts[i].part->hidden)
				color=0;
	
			char namestr[64];
			sprintf(namestr, "%i %s", parts[i].part->gearid+1, parts[i].part->name);

//			if(parts[i].part->loops>1 && parts[i].part->looptime>0)
			if(parts[i].loops>1 && parts[i].part->looptime>0)
			{
//				for(int j=1;j<parts[i].part->loops;j++)
				for(int j=1;j<parts[i].loops;j++)
				{
//					dui->DrawBar(x+j*(int)(parts[i].part->looptime*zoom)-(int)((parts[i].part->looptime-parts[i].part->Length())*zoom), parts[i].ypos*10+3, (int)(parts[i].part->looptime*zoom), 3, dui->palette[4]);
					dui->DrawBar(x+j*(int)(parts[i].part->looptime*zoom)-(int)((parts[i].part->looptime-parts[i].part->Length())*zoom), parts[i].ypos*10+3, (int)(parts[i].part->looptime*zoom), 3, dui->palette[6]);
					dui->DrawBar(x+j*(int)(parts[i].part->looptime*zoom), parts[i].ypos*10, (int)(parts[i].part->Length()*zoom), 10, dui->palette[8]);
					dui->DrawBox(x+j*(int)(parts[i].part->looptime*zoom), parts[i].ypos*10, (int)(parts[i].part->Length()*zoom), 10, dui->palette[6]);
				}
			}

			if(parts[i].selected)
			{
				sp_x=x;
				sp_yp=parts[i].ypos;
			}
	
//			if(!parts[i].part->selected)
//			if(!parts[i].selected)
				dui->DoPartIcon(x, parts[i].ypos*10, (int)(parts[i].part->Length()*zoom), parts[i].start, parts[i].ypos, zoom, dui->palette[color], namestr, 0, parts[i].clone, parts[i].sid);
/*			else
			{
				dui->DoPartIcon(x, parts[i].ypos*10, (int)(parts[i].part->Length()*zoom), parts[i].start, parts[i].ypos, zoom, dui->palette[color], namestr, 1, parts[i].clone, parts[i].sid);
				if(dui->lmbdown && dui->mousedx!=0)
					partmoved=true;
			}
*/
			if(parts[i].selected && dui->lmbdown && dui->mousedx!=0)
				partmoved=true;
			
			if(parts[i].ypos<1)
				parts[i].ypos=1;
	
			if((dui->lmbclick || dui->rmbclick) && dui->MouseInZone(x-iscrollx, 60+parts[i].ypos*10-iscrolly, (int)(parts[i].part->Length()*zoom), 10)) // clicked on this part
			{
				SelectPart(parts[i].sid);
				sp_x=x;
				sp_yp=parts[i].ypos;
				touchpart=true;
			}
			if(dui->dlmbclick && dui->MouseInZone(x-iscrollx, 60+parts[i].ypos*10-iscrolly, (int)(parts[i].part->Length()*zoom), 10)) // double-clicked on this part
			{
				parts[i].part->hidden=!parts[i].part->hidden;
				if(!parts[i].part->hidden)
					parts[i].part->popup=true;
				touchpart=true;
			}
			if(dui->rmbclick && dui->MouseInZone(x-iscrollx, 60+parts[i].ypos*10-iscrolly, (int)(parts[i].part->Length()*zoom), 10)) // clicked on this part
			{
				dui->EditString(parts[i].part->name);
				touchpart=true;
			}
	//			dui->DrawBar(x, 60+parts[i].ypos*10, 50, 10, dui->palette[1]);
	//			dui->DrawBox(x, 60+parts[i].ypos*10, 50, 10, dui->palette[6]);
	//			dui->DrawText(38, 21, dui->palette[6], "record");
		}
		
		if(iscrolly<10 && !touchpart && dui->MouseInZone(0, 60, glkitGetWidth()-8, 10-iscrolly)) // set play positions
		{
			if(dui->lmbclick || dui->rmbclick)
				dui->mouseregion=738;
		}
		if(dui->mouseregion==738 && (dui->lmbdown || dui->rmbdown))
		{
			int mt=(dui->mousex+iscrollx+8)/16*16;
			mt=(int)((float)mt/zoom);
			if(playdrag==-1 && dui->lmbclick)
			{
				if(abs(mt-loopfrom)<(int)(4.0f/zoom))
					playdrag=1;
				else if(abs(mt-playto)<(int)(4.0f/zoom))
					playdrag=2;
			}
			if(playdrag==-1)
			{
				if(dui->lmbclick && !dui->shift_down)
					playdrag=0;
				if(dui->lmbclick && dui->shift_down)
					playdrag=1;
				if(dui->rmbclick)
					playdrag=2;
			}
			if(playdrag==0)
				playfrom=mt;
			if(playdrag==1)
				loopfrom=mt;
			if(playdrag==2)
				playto=mt;
		}
		else
			playdrag=-1;
		
		// part icons may now be out of order if the currently selected one was moved, sort them (bubblesort may be ok)
		if(partmoved)
		{
			for(int i=0;i<numparts-1;i++)
			{
				if(parts[i].start>parts[i+1].start) // out of order, swap
				{
					SPart temp=parts[i];
					parts[i]=parts[i+1];
					parts[i+1]=temp;
					i-=2; // move back and re-iterate to see if we need to bubble further
					if(i<-1) i=-1;
				}
			}
		}
		
		// mute/unmute
		if(!touchpart && dui->rmbclick && dui->MouseInZone(0, 60, glkitGetWidth()-8, glkitGetHeight()-68))
		{
			int chi=(dui->mousey-70+iscrolly);
			if(chi>=0 && chi/10<58)
			{
				if(dui->shift_down)
				{
					int nm=0;
					for(int i=0;i<58;i++)
						if(muted[i])
							nm++;
					if(nm>29) // most are muted, unmute all
					{
						for(int i=0;i<58;i++)
							muted[i]=false;
					}
					else // few are muted, solo selected track
					{
						for(int i=0;i<58;i++)
							muted[i]=true;
						muted[chi/10]=false;
					}
				}
				else
					muted[chi/10]=!muted[chi/10];
			}
		}

		// visualize muted tracks
		for(int y=9;y<10*57;y+=10)
			if(muted[(y-9)/10])
				dui->DrawBarAlpha(iscrollx-1, y+1, iwidth+2, 9, dui->palette[4], 0.75f);

		// highlight selected spart
		if(sp_yp>0)
			for(int i=0;i<numparts;i++)
			{
				if(!parts[i].selected)
					continue;
	//			int x=(int)(parts[i].start*zoom);
				int color=1;
				if(!parts[i].part->hidden)
					color=0;
				char namestr[64];
				sprintf(namestr, "%i %s", parts[i].part->gearid+1, parts[i].part->name);
				dui->DoPartIcon(sp_x, sp_yp*10, (int)(parts[i].part->Length()*zoom), parts[i].start, parts[i].ypos, zoom, dui->palette[color], namestr, 1, parts[i].clone, parts[i].sid);
			}

//		if(dui->lmbclick && dui->MouseInZone(0, 60, 1024, 708)) // set start position
		
//		if(dui->show_key)
//			if(GetSelectedPart()!=NULL)
//				GetSelectedPart()->hidden=false;
		
		// re-order parts in case some were moved
		for(int i=0;i<numparts-1;i++)
		{
			if(i<0)
				i=0;
			if(parts[i].start>parts[i+1].start) // out of order -> swap
			{
				SPart temp=parts[i];
				parts[i]=parts[i+1];
				parts[i+1]=temp;
				i-=2; // move back in case this part needs to bubble down
			}
		}
		if(playing)
			dui->DrawBar((int)(playtime*zoom), 0, 1, glkitGetHeight(), dui->palette[9]);
	}
	dui->EndScrollArea();
};

