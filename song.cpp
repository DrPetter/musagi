#include "song.h"
#include "glkit_global.h"

void Song::PlayPrep(AudioStream *audiostream) // flush past parts
{
//	LogPrint("song: playprep");
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
//	LogPrint("song: playprep finished");
};

void Song::PlayStep(int newtime, AudioStream *audiostream)
{
	if(!playing)
		return;
//	LogPrint("song: playstep");
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

//	LogPrint("song: playstep finished");
};

void Song::KnobRecTick() // record or play back knob values
{
	if(!playing)
		return;
//	LogPrint("song: knobrectick");
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
//				LogPrint("knobrec: knob %i=%.2f @ time=%i", kn, *kr->value, playtime);
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
//	LogPrint("song: knobrectick finished");
};

void Song::DoInterface(DUI *dui, char* keep_part_selected)
{
	int cpart=-1;
	int num_selected_parts=0;
	for(int i=0;i<numparts;i++)
		if(parts[i].selected)
		{
			if(num_selected_parts==0)
				cpart=i;
			else
				cpart=-1;
			num_selected_parts++;
		}
	dui->DrawBar(5, glkitGetHeight()-53, 170, 48, dui->palette[8]);
	dui->DrawBox(5, glkitGetHeight()-53, 170, 48, dui->palette[6]);
	if(cpart!=-1)
	{
		dui->StartFlatWindow(6, glkitGetHeight()-52, 168, 46, "spset");
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

		if(dui->rmbclick && dui->MouseInZone(10, glkitGetHeight()-52, 160, 10))
			dui->EditString(parts[cpart].part->name);
		dui->EndWindow();
	}

	dui->DrawText(glkitGetWidth()-582, glkitGetHeight()-32, dui->palette[6], "+4 bars");
	if(dui->DoButton(glkitGetWidth()-605, glkitGetHeight()-33, "songb_p4b", "insert 4 bars in timeline at current start position"))
	{
		for(int i=0;i<numparts;i++)
			if(parts[i].ypos<58 && parts[i].start>=playfrom)
				parts[i].start+=16*4*640;
//				parts[i].start=(parts[i].start/64+16*4+2)/16*16*64;
	}

	dui->DrawText(glkitGetWidth()-582, glkitGetHeight()-20, dui->palette[6], "-4 bars");
	if(dui->DoButton(glkitGetWidth()-605, glkitGetHeight()-21, "songb_m4b", "remove 4 bars from timeline at current start position (if possible)"))
	{
		bool ok=true;
		for(int i=0;i<numparts;i++)
			if(parts[i].ypos<58 && parts[i].start>=playfrom && parts[i].start<playfrom+16*4*640)
			{
				ok=false;
				break;
			}
		if(ok)
			for(int i=0;i<numparts;i++)
				if(parts[i].ypos<58 && parts[i].start>=playfrom)
					parts[i].start-=16*4*640;
//					parts[i].start=(parts[i].start/64-16*4+2)/16*16*64;
	}

//	dui->StartScrollArea(0, 60, 1024, 650, scrollx, dummy, 1024*8, 650, dui->palette[7], "songscroll");
//	dui->StartScrollArea(0, 60, glkitGetWidth(), glkitGetHeight()-118, scrollx, dummy, 1024*8, glkitGetHeight()-118, dui->palette[7], "songscroll");
	float zoom=0.1f/64;
	if(dui->space_down && dui->lmbclick && dui->mousey>70 && dui->mousey<glkitGetHeight()-66)
	    spacescroll=true;
	if(spacescroll)
		scrollx-=(float)dui->mousedx/(scrollsize-glkitGetWidth());
	if(!dui->space_down || !dui->lmbdown)
	    spacescroll=false;
	AdjustScrollsize();
	int iwidth=glkitGetWidth();
	
	if(!showsong)
	{
		dui->DrawBox(-1, 61, glkitGetWidth()+2, glkitGetHeight()-119, dui->palette[6]);
		dui->DrawBar(-1, 61, glkitGetWidth()+2, 2, dui->palette[6]);
		return;
	}

	bool scroll_locked=false;
	dui->StartScrollArea(0, 60, glkitGetWidth(), glkitGetHeight()-118, scrollx, scrolly, scrollsize, 640, dui->palette[0], "songscroll");
	{
//		if(glkitGetHeight()-118<640)
//			iscrollx=(int)(scrollx*(scrollsize-glkitGetWidth()-8)); // -8 to compensate for vertical scroll bar
//		else
			iscrollx=(int)(scrollx*(scrollsize-glkitGetWidth()));
		iscrolly=(int)(scrolly*(640-(glkitGetHeight()-118-8))); // -8 to compensate for horizontal scroll bar (always)

		// scroll to follow song while playing
		if(scrollfollow && playing && playtime*zoom<iscrollx)
			scrollx=(float)(playtime*zoom-16)/(scrollsize-iwidth);
		if(scrollfollow && playing && playtime*zoom>(iscrollx+iwidth/2))
		{
			scrollx=(float)(playtime*zoom-iwidth/2-1)/(scrollsize-iwidth);
			scroll_locked=true;
		}
		
		dui->DrawBar(iscrollx-1, 0, iwidth+2, 10, dui->palette[15]); // play position bar

		int beatlength=16*4;
		if(GetBeatLength()==3)
			beatlength=16*3;

		for(int x=0;x<scrollsize;x+=16)
		{
			if(x<iscrollx)
				continue;
			if(x>iscrollx+glkitGetWidth())
				break;
			if(x%beatlength==0)
				dui->DrawBar(x, 0, 1, 10*58, dui->palette[8]);
			else
				dui->DrawBar(x, 0, 1, 10*58, dui->palette[4]);
		}
		for(int x=0;x<scrollsize;x+=16)
		{
			if(x<iscrollx-40) // -40 to avoid dropping left-most timing text
				continue;
			if(x>iscrollx+glkitGetWidth())
				break;
			if(x%beatlength==0 && x>=(int)(playfrom*zoom))
			{
				int sec=(int)(((float)x/zoom-playfrom)*GetTempo()/1600)/44100;  // [16/tempo] ticks per sample      [zoom] ticks per pixel             numsamples*16/GetTempo()
				int min=sec/60;
				sec%=60;
				dui->DrawText(x+2, 1, dui->palette[10], "%.2i:%.2i", min, sec);
			}
		}

		for(int y=9;y<10*57;y+=10)
			dui->DrawBar(iscrollx-1, y, iwidth+2, 1, dui->palette[6]);
		dui->DrawBar(iscrollx-1, y, iwidth+2, 2, dui->palette[6]);

		dui->DrawBar((int)(playto*zoom)-1, iscrolly, 3, glkitGetHeight()-110, dui->palette[0]);
		if(loop)
			dui->DrawBar((int)(loopfrom*zoom)-1, iscrolly, 3, glkitGetHeight()-110, dui->palette[8]);
		dui->DrawBar((int)(playfrom*zoom)-1, iscrolly, 3, glkitGetHeight()-110, dui->palette[10]);

		dui->DrawBar(iscrollx-1, 0, iwidth+2, 1, dui->palette[6]); // top black line

		int sp_x=0;
		int sp_yp=0;
		bool touchpart=false;
		bool clicked_selected_part=false;
		bool partmoved=false;	
		for(int i=0;i<numparts;i++)
		{
			int x=(int)(parts[i].start*zoom);
			Color picol=ColorFromHSV(0.9f, 0.25f, 0.45f); // window shown
			if(parts[i].part->hidden)
				picol=ColorFromHSV(0.6f, 0.4f, 0.4f); // window hidden
			float gui_hue=parts[i].part->gearstack->instrument->gui_hue;
			Color instcol=ColorFromHSV(gui_hue, 0.4f, 0.6f); // instrument tag
	
			char namestr[64];
			sprintf(namestr, "%2i %s", parts[i].part->gearid+1, parts[i].part->name);

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
				dui->DoPartIcon(x, parts[i].ypos*10, (int)(parts[i].part->Length()*zoom), parts[i].start, parts[i].ypos, zoom, picol, instcol, namestr, 0, parts[i].clone, parts[i].sid);
/*			else
			{
				dui->DoPartIcon(x, parts[i].ypos*10, (int)(parts[i].part->Length()*zoom), parts[i].start, parts[i].ypos, zoom, dui->palette[color], namestr, 1, parts[i].clone, parts[i].sid);
				if(dui->lmbdown && dui->mousedx!=0)
					partmoved=true;
			}
*/
			if(!dui->space_down && parts[i].selected && dui->lmbdown && dui->mousedx!=0)
				partmoved=true;
			
			if(parts[i].ypos<1)
				parts[i].ypos=1;

			bool mouse_on_part=dui->MouseInZone(x-iscrollx, 60+parts[i].ypos*10-iscrolly, (int)(parts[i].part->Length()*zoom), 10);

			if(!dui->space_down && (dui->lmbclick || dui->rmbclick) && mouse_on_part) // clicked on this part
			{
				if(!parts[i].selected && !dui->shift_down && !dui->ctrl_down)
					DeselectPart(); // deselect all parts
//				if(parts[i].selected)
					clicked_selected_part=true;
				SelectPart(parts[i].sid);
				sp_x=x;
				sp_yp=parts[i].ypos;
				dui->part_actualmove_dx=0;
				dui->part_actualmove_dy=0;
				touchpart=true;
			}
			if(!dui->space_down && dui->dlmbclick && mouse_on_part) // double-clicked on this part
			{
				parts[i].part->hidden=!parts[i].part->hidden;
				if(!parts[i].part->hidden)
					parts[i].part->popup=true;
				touchpart=true;
			}
			if(dui->rmbclick && mouse_on_part) // right-clicked on this part
			{
				dui->PopupMenu(2, i);
				touchpart=true;
			}

	//			dui->DrawBar(x, 60+parts[i].ypos*10, 50, 10, dui->palette[1]);
	//			dui->DrawBox(x, 60+parts[i].ypos*10, 50, 10, dui->palette[6]);
	//			dui->DrawText(38, 21, dui->palette[6], "record");
		}
		
		if((dui->lmbclick || dui->rmbclick) && !clicked_selected_part && !dui->space_down && dui->MouseInZone(0, 70, glkitGetWidth(), glkitGetHeight()-135)) // clicked somewhere not on a part
			DeselectPart();

		if(keep_part_selected!=NULL)
			SelectPart(keep_part_selected);
		
//		if(dui->lmbrelease && dui->mousemoved)
//			DeselectPart();

		if(dui->CheckRCResult(20) && num_selected_parts<2) // rename spart
			dui->EditString(parts[dui->rcresult_param].part->name);
		
		if(iscrolly<10 && !touchpart && dui->MouseInZone(0, 60, glkitGetWidth()-8, 10-iscrolly)) // set play positions
		{
			if(dui->lmbclick || dui->rmbclick)
				dui->mouseregion=738;
		}

		// move start/loop/end markers
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
			if(playdrag==1 && !loop) // click through loop position when it's not visible
				playdrag=-1;
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

//		bool mouse_in_song_area=dui->MouseInZone(0, 60, glkitGetWidth()-8, glkitGetHeight()-68);
//		bool mouse_in_song_area=dui->MouseInZone(0, 70, glkitGetWidth()-8, glkitGetHeight()-68);
		bool mouse_in_song_area=dui->MouseInZone(0, 70, glkitGetWidth()-8, glkitGetHeight()-135);
		
		// mute/unmute and empty space right-click menu
		int do_mute_toggle=0;
		if(!touchpart && ghostpart_mode==0 && dui->rmbclick && mouse_in_song_area)
			dui->PopupMenu(4, 0);
		if(dui->CheckRCResult(41)) // mute/unmute
			do_mute_toggle=1;
		if(dui->CheckRCResult(42)) // toggle solo
			do_mute_toggle=2;
		if(do_mute_toggle>0 || (!touchpart && dui->ctrl_down && dui->lmbclick && mouse_in_song_area))
		{
			int rmx=dui->mousex;
			int rmy=dui->mousey;
			if(do_mute_toggle>0)
			{
				rmx=dui->rcresult_mousex;
				rmy=dui->rcresult_mousey;
			}
			int chi=(rmy-70+iscrolly);
			if(chi>=0 && chi/10<58)
			{
				if(do_mute_toggle==2 || (do_mute_toggle==0 && dui->shift_down))
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

		// selection box start (press)
		if(!selectbox && !touchpart && dui->lmbclick && !dui->space_down && mouse_in_song_area && ghostpart_mode==0)
		{
			selectbox=true;
			selectbox_x1=dui->mousex-dui->gui_winx;
			selectbox_y1=dui->mousey-dui->gui_winy;
		}
		// selection box end (release)
		if(selectbox && !dui->lmbdown)
		{
			selectbox=false;
			int sbx1=selectbox_x1;
			int sby1=selectbox_y1;
			int sbx2=dui->mousex-dui->gui_winx;
			int sby2=dui->mousey-dui->gui_winy;
			int temp;
			if(sbx1>sbx2)
			{
				temp=sbx1;
				sbx1=sbx2;
				sbx2=temp;
			}
			if(sby1>sby2)
			{
				temp=sby1;
				sby1=sby2;
				sby2=temp;
			}
			// select sparts touched by selection box
			for(int i=0;i<numparts;i++)
			{
				parts[i].selected=false;
				int partx=(int)(parts[i].start*zoom);
				int party=parts[i].ypos*10;
				int partlength=(int)(parts[i].part->Length()*zoom);
				if(partx<sbx2 && partx+partlength>sbx1 && party<sby2 && party+10>sby1)
					parts[i].selected=true;
			}
		}
		if(selectbox)
		{
			int x2=dui->mousex-dui->gui_winx;
			int y2=dui->mousey-dui->gui_winy;
			dui->DrawBarAlpha(selectbox_x1, selectbox_y1, x2-selectbox_x1, y2-selectbox_y1, dui->palette[1], 0.2f);
			dui->DrawBox(selectbox_x1, selectbox_y1, x2-selectbox_x1, y2-selectbox_y1, dui->palette[9]);
		}

		// highlight selected sparts
//		if(sp_yp>0)
			for(int i=0;i<numparts;i++)
			{
				if(!parts[i].selected)
					continue;

	//			int x=(int)(parts[i].start*zoom);
				int color=1;
				if(!parts[i].part->hidden)
					color=0;
//				char namestr[64];
//				sprintf(namestr, "%i %s", parts[i].part->gearid+1, parts[i].part->name);
				dui->DoPartIcon((int)(parts[i].start*zoom), parts[i].ypos*10, (int)(parts[i].part->Length()*zoom), parts[i].start, parts[i].ypos, zoom, dui->palette[color], dui->palette[0], "", 1, parts[i].clone, parts[i].sid);
			}

		// autoscroll on dragging parts to screen edges
		if(dui->part_move)
		{
			// iscrollx=(int)(scrollx*(scrollsize-glkitGetWidth()));
//			if(sp_x-iscrollx<16) // autoscroll left
			if(dui->mousex<16 && scrollx>0.0f) // autoscroll left
			{
				scrollx-=12.0f/(scrollsize-glkitGetWidth());
				dui->part_scrollmove=-12;
			}
			if(dui->mousex>glkitGetWidth()-16 && scrollx<1.0f) // autoscroll right
			{
				scrollx+=12.0f/(scrollsize-glkitGetWidth());
				dui->part_scrollmove=12;
			}
		}
		dui->part_move=false;


		// move all selected parts if one of them were dragged
		for(int i=0;i<numparts;i++)
		{
			if(!parts[i].selected)
				continue;
			parts[i].start+=dui->part_actualmove_dx;
			parts[i].ypos+=dui->part_actualmove_dy;
			if(parts[i].start<(int)(-32.0f/zoom))
				parts[i].start=(int)(-32.0f/zoom);
			if(parts[i].ypos<1) parts[i].ypos=1;
//			if(parts[i].ypos>49) parts[i].ypos=49;
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
		if(playing && !scroll_locked)
		{
			int posx=(int)(playtime*zoom);
			dui->DrawBar(posx, 1, 1, glkitGetHeight(), dui->palette[9]);
			int x=posx;
			if(x>=(int)(playfrom*zoom))
			{
				int sec=(int)(((float)x/zoom-playfrom)*GetTempo()/1600)/44100;
				int min=sec/60;
				sec%=60;
				dui->DrawBar(x+1, 1, 33, 8, dui->palette[14]);
				dui->DrawText(x+3, 1, dui->palette[9], "%.2i:%.2i", min, sec);
			}
		}
	}
	dui->EndScrollArea();

	if(playing && scroll_locked)
	{
		int posx=glkitGetWidth()/2;
		int posy=60;
		dui->DrawBar(posx, posy+1, 1, glkitGetHeight()-118-9, dui->palette[9]);
		int x=(int)(playtime*zoom);
		if(x>=(int)(playfrom*zoom))
		{
			int sec=(int)(((float)x/zoom-playfrom)*GetTempo()/1600)/44100;
			int min=sec/60;
			sec%=60;
			dui->DrawBar(posx+1, posy+1, 33, 8, dui->palette[14]);
			dui->DrawText(posx+3, posy+1, dui->palette[9], "%.2i:%.2i", min, sec);
		}
	}
};

