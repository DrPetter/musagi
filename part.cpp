#include "part.h"
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
		playing=false; // yikes?
	}

	audiostream->GetTick(3); // reset song tick
};

bool Part::DoInterface(DUI *dui, void *astream)
{
	if(hidden)
		return false;

	AudioStream *audiostream=(AudioStream*)astream;
	
	gear_instrument *instrument=gearstack->instrument;

	int maxheight=560;
	int minheight=200;
	if(gearstack->instrument->Atomic())
	{
		maxheight=232;
		minheight=232;
		winh=maxheight;
	}
	
	dui->NameSpace(this);
	char windowtitle[256];
//	sprintf(windowtitle, "%s   (%i: %s)", name, gearstack->instrument->GetGid()+1, gearstack->instrument->Name());
	sprintf(windowtitle, "%s", name);
	bool winactive=false;
//	if(iconify)
//		winactive=dui->StartWindow(winx, winy, 400, 1, windowtitle, "partwin");
//	else
//		winactive=dui->StartWindow(winx, winy, 400, 200, windowtitle, "partwin");
		winactive=dui->StartWindowResize(winx, winy, winw, winh, 300, minheight, maxheight, windowtitle, "partwin");

	if(popup)
	{
		popup=false;
		dui->RequestWindowFocus("partwin");
		winactive=true;
	}
	if(winactive)
		SetCurrentPart(this);

//	if(!iconify)
//	{
	dui->DrawTexturedBar(1, 12, 100-1, winh-1, dui->palette[10], dui->brushed_metal, &dui->vglare);
	dui->DrawBox(99, 12, 1, winh, dui->palette[6]);
	
	dui->DrawText(38, 33, dui->palette[6], "record");
	dui->DrawLED(5, 32, 0, prec);
	if(dui->DoButton(15, 32, "btn_prec", "live recording - will count in 4 bars if metronome is enabled"))
	{
		if(!prec) // about to start
		{
			pplay=false;
			prec_curtime=0;
			if(audiostream->metronome_on)
			{
				prec_start=audiostream->GetTick(0)+4*32*320*GetTempo()/1600; // count in
				audiostream->metronome=32*320*GetTempo()/1600;
				audiostream->metrocount=0;
			}
			else
				prec_start=audiostream->GetTick(0);
		}
		prec=!prec;
		if(!prec) // stopped
		{
			// release any open notes
			for(int i=0;i<numtriggers;i++)
				if(triggers[i].duration==0)
					triggers[i].duration=prec_curtime-triggers[i].start;
			audiostream->metronome=0;
		}
	}
	
	dui->DrawText(38, 45, dui->palette[6], "play");
	dui->DrawLED(5, 44, 0, pplay);
	if(dui->DoButton(15, 44, "btn_pplay", "play this part from the beginning - will loop infinitely if one is set"))
		PlayButton(audiostream);
	
	dui->DrawText(38, 72, dui->palette[6], "place loop");
	dui->DrawLED(5, 71, 0, placing_loop);
	if(dui->DoButton(15, 71, "btn_plooptime", "set or clear loop position - press this then click in timeline"))
		placing_loop=!placing_loop;

	dui->DrawText(38, 87, dui->palette[6], "place start");
	dui->DrawLED(5, 86, 0, placing_start);
	if(dui->DoButton(15, 86, "btn_pstarttime", "set or clear start position (does not affect playback in song) - press this then click in timeline"))
		placing_start=!placing_start;

	dui->DrawBar(0, 103, 100, 1, dui->palette[6]);
	dui->DrawBar(0, 105, 100, 1, dui->palette[6]);

/*		char lstr[20];
	sprintf(lstr, "loop %i", loops);
	dui->DrawText(5, 68, dui->palette[6], lstr);
	dui->DrawText(38, 80, dui->palette[6], "loop up");
	if(dui->DoButton(15, 79, "btn_ploop1"))
		loops++;
	dui->DrawText(38, 92, dui->palette[6], "loop down");
	if(dui->DoButton(15, 91, "btn_ploop2") && loops>1)
		loops--;
*/
	dui->DrawText(22, winh-79, dui->palette[6], "load");
	if(dui->DoButton(25, winh-70, "btn_pload", "load part content - will not affect the instrument"))
	{
		char filename[256];
		if(dui->SelectFileLoad(filename, 1))
		{
			FILE *file=fopen(filename, "rb");
			if(file)
			{
				if(!LoadContent(file))
					MessageBox(NULL, "Unknown part format (maybe incompatible version)", "Error", MB_ICONEXCLAMATION);
				fclose(file);
			}
			else
				MessageBox(NULL, "File not found", "Error", MB_ICONEXCLAMATION);
		}
	}
	dui->DrawText(52, winh-79, dui->palette[6], "save");
	if(dui->DoButton(55, winh-70, "btn_psave", "save part content - does not save instrument settings"))
	{
		char filename[256];
		if(dui->SelectFileSave(filename, 1))
		{
			FILE *file=fopen(filename, "wb");
			if(file)
			{
				SaveContent(file);
				fclose(file);
			}
			else
				MessageBox(NULL, "Unable to save (write protected?)", "Error", MB_ICONEXCLAMATION);
		}
	}

	dui->DrawBar(0, winh-38, 100, 1, dui->palette[6]);
	dui->DrawText(28, winh-32, dui->palette[6], "clear");
	if(dui->DoButtonRed(5, winh-33, "btn_pclear", "clear all notes"))
	{
//		int choice=MessageBox(GetHwnd(), "Are you sure you want to clear this part?", "Caution", MB_ICONEXCLAMATION|MB_OKCANCEL);
//		if(choice==IDOK)
			Clear();

		// deselect multi
		for(int i=0;i<numtriggers;i++)
			triggers[i].selected=false;
		msnum=0;
	}
	dui->DrawBar(0, winh-19, 100, 1, dui->palette[6]);

	dui->DrawText(25, winh-13, dui->palette[6], "set instr.");
	if(dui->DoButton(75, winh-14, "btn_piset", "set to currently selected instrument") || (winactive && dui->CheckRCResult(31)))
	{
		GearStack *newgs=GetCurrentGearstack();
		if(newgs->instrument!=NULL)
		{
			gearstack=newgs;
			gearid=gearstack->instrument->GetGid();
		}
	}

	dui->DrawBar(1, winh, 98, 11, dui->palette[8]);
	dui->DrawBar(0, winh, 100, 1, dui->palette[6]);
	dui->DrawText(5, winh+2, dui->palette[6], "%i: %s", gearstack->instrument->GetGid()+1, gearstack->instrument->Name());
	if(dui->rmbclick && dui->MouseInZone(winx, winy+winh-12+12, 100, 12)) // rename instrument
		dui->EditString(gearstack->instrument->Name());
		
//		dui->DrawText(38-8, 50, dui->palette[6], "volume");
//		dui->DoKnob(38, 50+10, 16, 16, gearstack->volume, -1, "knob_volume");

	if(winactive && dui->rmbclick && !dui->shift_down) // deselect multi
	{
		for(int i=0;i<numtriggers;i++)
			triggers[i].selected=false;
		msnum=0;
	}
	if(!dui->lmbdown && msnum>=0) // reset multimove
		msn1=-1;
	if(winactive && (dui->shift_down || dui->ctrl_down) && dui->key_a) // shift+A or ctrl+A = select all
	{
		msnum=0;
		for(int i=0;i<numtriggers;i++)
		{
			triggers[i].selected=true;
			msitriggers[msnum++]=i;
		}
	}

	if(msnum>0 && winactive && dui->ctrl_down && dui->key_c) // ctrl+C = copy selected triggers
	{
		// add selected triggers to copy buffer
		PartCopy_Reset();
		for(int i=0;i<numtriggers;i++)
			if(triggers[i].selected)
				PartCopy_Add(triggers[i]);
		// deselect
		for(int i=0;i<numtriggers;i++)
			triggers[i].selected=false;
		msnum=0;
	}

//	if(msnum==0 && PartCopy_NumTriggers()>0 && winactive && dui->ctrl_down && dui->key_v) // ctrl+V = paste triggers
	if(!pasting && PartCopy_NumTriggers()>0 && winactive && dui->ctrl_down && dui->key_v) // ctrl+V = paste triggers
	{
		pasting=true;
		
		// deselect
		for(int i=0;i<numtriggers;i++)
			triggers[i].selected=false;
		msnum=0;

		// paste triggers, then select them so they're easy to move
		int pcstart=-1;
		for(int i=0;i<PartCopy_NumTriggers();i++)
			if(pcstart==-1 || PartCopy_Triggers(i).start<pcstart)
				pcstart=PartCopy_Triggers(i).start;
		int iwidth=winw-120-1;
		int offset=(int)((scrollsize-iwidth)*pscrollx)/4*4*320;
		if(offset+120*320==pcstart)
			offset+=12*320;
		for(int i=0;i<PartCopy_NumTriggers();i++)
		{
			int start=PartCopy_Triggers(i).start-pcstart+offset+120*320;
			int duration=PartCopy_Triggers(i).duration;
			int note=PartCopy_Triggers(i).note;
			float volume=PartCopy_Triggers(i).volume;
			int nt=InsertTrigger(note, start, duration, volume);
			triggers[nt].selected=true;
		}
		for(int i=0;i<numtriggers;i++)
			if(triggers[i].selected)
				msitriggers[msnum++]=i;
	}
	if(!(dui->ctrl_down && dui->key_v))
		pasting=false;

	float zoom=1.0f/320.0f;
	int notearea=0;
	AdjustScrollsize();

	int beatlength=32*4;
	if(GetBeatLength()==3)
		beatlength=32*3;

	static bool spacescroll=false;

	if(!instrument->Atomic())
	{
		notearea=560;
		if(winactive)
			pscrolly-=(float)dui->mousewheel*0.05f;

		if(winactive)
		{
			if(spacescroll)
			{
				int iwidth=winw-120-1;
				int iheight=winh-33;
				pscrollx-=(float)dui->mousedx/(1+scrollsize-iwidth);
				pscrolly-=(float)dui->mousedy/(1+notearea-iheight);
			}
			if(!dui->lmbdown)
				spacescroll=false;
		}

		// regular tonal instrument
//		dui->StartScrollArea(100, 12, winw-100-1, winh-33, pscrollx, pscrolly, scrollsize, notearea, dui->palette[10], "partscroll");
		dui->StartScrollArea(120, 12, winw-120-1, winh-33, pscrollx, pscrolly, scrollsize, notearea, dui->palette[10], "partscroll");
		{
			int iwidth=winw-120-1;
			int iscrollx=(int)((scrollsize-iwidth)*pscrollx);
			// piano roll area
			for(int y=0;y<notearea+8;y+=8) // C keys
				if(((y/8)%7)==0)
					dui->DrawBar(iscrollx-1, y-7, iwidth+2, 7, dui->palette[19]);
			for(int x=0;x<scrollsize;x+=16)
			{
				if(x<(int)(pscrollx*(scrollsize-iwidth+2)))
					continue;
				if(x>(int)(pscrollx*(scrollsize-iwidth+2)+iwidth+1))
					break;
//				if(x%beatlength==0)
//					dui->DrawBar(x-1, 0, 3, notearea, dui->palette[17]);
				if(x%beatlength!=0 && x%32==0)
					dui->DrawBar(x, 0, 1, notearea, dui->palette[17]);
				else
					dui->DrawBar(x, 0, 1, notearea, dui->palette[8]);
			}
			for(int y=0;y<notearea;y+=8) // divider lines between white keys
			{
				switch(((y/8)%7))
				{
				case 0:
				case 4:
					dui->DrawBar(iscrollx-1, y, iwidth+2, 1, dui->palette[8]);
/*					if(((y/8)%7)==4)
						for(int x=0;x<scrollsize;x+=32)
						{
							if(x<(int)(pscrollx*(scrollsize-iwidth+2)))
								continue;
							if(x>(int)(pscrollx*(scrollsize-iwidth+2)+iwidth+1))
								break;
							dui->DrawBar(x-2, y, 5, 1, dui->palette[5]); // dashed lines
						}*/
					break;
				default:
					dui->DrawBar(iscrollx-1, y-1, iwidth+2, 2, dui->palette[17]); // black keys
					break;
				}
			}
			for(int x=0;x<scrollsize;x+=16)
				if(x%beatlength==0)
					dui->DrawBar(x-1, 0, 3, notearea, dui->palette[17]);

			if(winactive)
			{
				// Visualize keypresses
				if(!dui->ctrl_down)
					for(int kn=0;kn<120;kn++)
						if(IsKeyDown(kn))
						{
							int ky=notearea/8*8-5-GetNotePos(kn)*4;
							if(NoteIsBlack(kn))
								dui->DrawBar(iscrollx-1, ky, iwidth+2, 2, dui->palette[9]);
							else
								dui->DrawBar(iscrollx-1, ky-1, iwidth+2, 4, dui->palette[9]);
						}
				if(dui->delete_pressed && msnum>0) // delete selected triggers
				{
					for(int i=0;i<numtriggers;i++)
						if(triggers[i].selected)
						{
							RemoveTrigger(i);
							i--; // check this spot again since the list was reordered
						}
					msnum=0; // deselect all triggers
				}
			}












			if(prec)
			{
				prec_curtime=(int)((float)(audiostream->GetTick(0)-prec_start)*1600/GetTempo());
				dui->DrawBar((int)(prec_curtime*zoom), 0, 1, notearea, dui->palette[11]);
			}
			if(pplay)
			{
				int px=(int)((audiostream->GetTick(0)-pplay_offset)*1600*zoom/GetTempo()+(float)starttime*zoom);
				dui->DrawBar(px, 0, 1, notearea, dui->palette[9]);

				if(vlooped)
				{
					vlooped=false;
					vloopcounter++;
					pplay_offset=audiostream->GetTick(0);
				}
			}
			else if(!Ended())
			{
				int px=(int)((playtime-(looptime-starttime)*loopcounter)*zoom);
				dui->DrawBar(px, 0, 1, notearea, dui->palette[9]);
			}











			if(looptime>0)
				dui->DrawBar((int)(looptime*zoom)-1, 0, 3, notearea, dui->palette[9]);
			if(starttime>0)
				dui->DrawBar((int)(starttime*zoom)-1, 0, 3, notearea, dui->palette[15]);

			int pm_x=0;
			int pm_y=0;
			int pm_note=0;
			int pm_on=false;
				
//			if(winactive && dui->MouseInZone(winx+100, winy+12, winw-100-8, winh-10))
			if(winactive && dui->MouseInZone(winx+120, winy+12, winw-120-8, winh-42))
			{
				if(dui->lmbclick && dui->space_down)
					spacescroll=true;

				int mx=dui->mousex+2-dui->gui_winx;
				//if(snap)
					mx=mx/4*4;
				int mnote=-1;
				int ny=-1;
				int tmnote=-1;
				int tny=-1;
				for(int i=0;i<120;i++)
				{
					ny=notearea/8*8-4-GetNotePos(i)*4;
					if(mnote==-1 && abs(ny-(dui->mousey-dui->gui_winy))<5) // wide keys, missing black
					{
						tmnote=i;
						tny=ny;
					}
					if(abs(ny-(dui->mousey-dui->gui_winy))<3)
					{
						mnote=i;
						break;
					}
				}
				if(mnote==-1 && tmnote>-1)
				{
					ny=tny;
					mnote=tmnote;
				}
				if(mnote!=-1 && !sbox_active && !dui->space_down)
				{
					dui->DrawBar(mx, 0, 2, notearea, dui->palette[11]);
					if(edit_curtrigger==-1)
						dui->DrawBar(iscrollx-1, ny-1, iwidth+2, 2, dui->palette[11]);
				}
				int curtime=(int)(mx/zoom);
				if(!spacescroll)
				{
	//				if(dui->rmbclick && dui->shift_down && msnum>0 && mnote!=-1) // start moving multiple notes
					if(dui->lmbclick && !dui->shift_down && msnum>0 && mnote!=-1) // start moving multiple notes
					{
						mst1=curtime;
						msn1=mnote;
					}
	//				if(dui->rmbdown && dui->shift_down && msnum>0 && msn1!=-1) // move multiple
					if(dui->lmbdown && !dui->shift_down && msnum>0 && msn1!=-1) // move multiple
					{
						int dt=curtime-mst1;
						int dn=mnote-msn1;
						if(mnote!=-1 && (dt!=0 || dn!=0))
						{
							for(int i=0;i<msnum;i++)
								mstriggers[i]=triggers[msitriggers[i]];
							for(int i=0;i<numtriggers;i++)
								if(triggers[i].selected)
								{
									RemoveTrigger(i);
									i--; // check this spot again since the list was reordered
								}
							for(int i=0;i<msnum;i++)
							{
								Trigger *trig=&mstriggers[i];
	//							int id=mstriggers[i];
	//							int tnote=triggers[id].note+dn;
	//							int ttime=triggers[id].start+dt;
	//							ttime=(int)(((int)(ttime/zoom)/4*4)*zoom); // snap
	//							int tdur=triggers[id].duration;
								trig->start+=dt;
								trig->note+=dn;
	//							float tvol=triggers[id].volume;
	//							RemoveTrigger(id);
								int id=InsertTrigger(trig->note, trig->start, trig->duration, trig->volume);
								triggers[id].slidenote=trig->slidenote+dn;
								triggers[id].slidelength=trig->slidelength;
								triggers[id].selected=true;
							}
							int nn=0;
							for(int i=0;i<numtriggers;i++)
								if(triggers[i].selected)
									msitriggers[nn++]=i;
							mst1+=dt;
							msn1+=dn;
						}
					}
					if(dui->rmbdown && !dui->shift_down) // delete trigger
					{
						if(placing_loop) // remove loop
						{
							looptime=0;
							placing_loop=false;
						}
						else if(placing_start) // remove start
						{
							starttime=0;
							placing_start=false;
						}
						else if(mnote!=-1)
						{
							int oldtrigger=FindTrigger(curtime, mnote);
							if(oldtrigger!=-1)
								RemoveTrigger(oldtrigger);
						}
					}
					if(dui->lmbclick && !dui->shift_down) // place new trigger
					{
						// deselect multi
	//					for(int i=0;i<numtriggers;i++)
	//						triggers[i].selected=false;
	//					msnum=0;
	
						if(placing_loop) // place loop position
						{
							looptime=curtime;
							placing_loop=false;
						}
						else if(placing_start) // place start position
						{
							starttime=curtime;
							placing_start=false;
						}
						else if(mnote!=-1)
						{
							if(!dui->shift_down && msnum<=0)
							{
								int oldtrigger=FindTrigger(curtime, mnote);
								if(oldtrigger==-1)
								{
									if(!dui->ctrl_down)
										edit_curtrigger=InsertTrigger(mnote, curtime, 10, 1.0f);
									if(!pplay || dui->ctrl_down)
										gearstack->instrument->Trigger(mnote, 1.0f, 2000);
								}
								else
									edit_curtrigger=oldtrigger;
							}
						}
					}
					if(dui->lmbclick && dui->shift_down) // start selection box
					{
						sbox_active=true;
						sbox_x1=dui->mousex-dui->gui_winx;
						sbox_y1=dui->mousey-dui->gui_winy;
						sbox_x2=sbox_x1;
						sbox_y2=sbox_y1;
					}
					if(dui->lmbdown) // determine trigger duration
					{
	//					if(dui->shift_down || (dui->ctrl_down && msnum==0)) // multiselect
						if(dui->ctrl_down && msnum==0) // select note for ctrl-move
						{
							int hittrigger=FindTrigger(curtime, mnote);
							if(hittrigger!=-1 && !triggers[hittrigger].selected)
							{
								msitriggers[msnum++]=hittrigger;
								triggers[hittrigger].selected=true;
								if(dui->ctrl_down) // start moving immediately
								{
									ctrl_move=true;
									mst1=curtime;
									msn1=mnote;
								}
							}
							edit_curtrigger=-1;
						}
						if(edit_curtrigger!=-1)
						{
							int pduration=triggers[edit_curtrigger].duration;
							triggers[edit_curtrigger].duration=curtime-triggers[edit_curtrigger].start+100; // +100!?
							if(triggers[edit_curtrigger].duration<10)
								triggers[edit_curtrigger].duration=10;
							if(triggers[edit_curtrigger].slidelength>-1)
							{
								triggers[edit_curtrigger].slidelength-=triggers[edit_curtrigger].duration-pduration;
								if(triggers[edit_curtrigger].slidelength<1)
									triggers[edit_curtrigger].slidelength=-1;
							}
						}
					}
					else
					{
						if(ctrl_move) // was in move-one mode, deselect it upon mouse release
						{
							ctrl_move=false;
							for(int i=0;i<numtriggers;i++)
								triggers[i].selected=false;
							msnum=0;
						}
	//					if(edit_curtrigger!=-1)
	//						gearstack->instrument->Release(mnote);
						edit_curtrigger=-1; // no trigger active
					}
					if(dui->mmbclick && !dui->shift_down) // starts setting slide
					{
						slide_origin=FindTrigger(curtime, mnote);
						if(slide_origin!=-1)
							triggers[slide_origin].slidelength=-1;
					}
	/*				if(dui->mmbclick && !dui->shift_down) // toggle slide (targets next trigger)
					{
						int hittrigger=FindTrigger(curtime, mnote);
						if(hittrigger!=-1)
						{
							if(triggers[hittrigger].slidelength>-1)
								triggers[hittrigger].slidelength=-1;
							else if(hittrigger<numtriggers-1)
							{
								int length=triggers[hittrigger+1].start-(triggers[hittrigger].start+triggers[hittrigger].duration);
								if(length>0)
								{
									triggers[hittrigger].slidenote=triggers[hittrigger+1].note;
									triggers[hittrigger].slidelength=length+100;
								}
							}
						}
					}*/
					if(dui->mmbdown && !dui->shift_down) // set slide
					{
						int length=curtime-(triggers[slide_origin].start+triggers[slide_origin].duration);
						if(length>0)
						{
							triggers[slide_origin].slidenote=mnote;
							triggers[slide_origin].slidelength=length+100; // +100 is some ugly fix to avoid gaps between notes, or something
						}
					}
				}

				// show popup note name at mouse cursor
				if(dui->ctrl_down)
				{
					pm_x=mx+15;
					pm_y=ny;
					pm_note=mnote;
					pm_on=true;
				}
			}
			int ptrigs=-1;
			int ptrign=-1;
			for(int i=0;i<numtriggers;i++) // ********** only check the ones potentially within visible window
			{
				Trigger *trig=&triggers[i];
				int nx=(int)(trig->start*zoom);
				int nd=(int)(trig->duration*zoom)+1;
				int ny=notearea/8*8-4-GetNotePos(trig->note)*4;
				int color=6;
				if(ptrigs!=-1 && trig->start==ptrigs && trig->note==ptrign) // overlapping, show warning graphic
					dui->DrawBar(nx-2, ny-3, 5, 7, dui->palette[11]);
				if(trig->selected)
					color=9;
				ptrigs=trig->start;
				ptrign=trig->note;
//				dui->DrawBar(nx-1, ny-2, 3, 4, dui->palette[color]);
//				dui->DrawBar(nx, ny-1, nd, 2, dui->palette[color]);
				Color trigcol;
				float iscale=0.6f;
				if(NoteIsBlack(trig->note))
					iscale=0.45f;
				trigcol.r=iscale-pow(trig->volume, 2.5f)*iscale;
				trigcol.g=trigcol.r;
				trigcol.b=trigcol.r;
				if(color==9)
					trigcol=dui->palette[color];
				dui->DrawBar(nx-1, ny-2, 3, 4, trigcol);
				dui->DrawBar(nx, ny-1, nd, 2, trigcol);
//				dui->DrawBar(nx, ny-1, 1, 2, dui->palette[6]);
				if(trig->slidelength>-1)
				{
					int sd=(int)(trig->slidelength*zoom);
					int ny2=notearea/8*8-4-GetNotePos(trig->slidenote)*4;
					if(color==6)
						color=15;
					dui->DrawLine(nx+nd-1, ny, nx+nd+sd, ny2, 2, dui->palette[color]);
				}
			}
			if(pm_on)
			{
				dui->DrawBar(pm_x, pm_y-6, 26, 11, dui->palette[9]);
				dui->DrawBox(pm_x, pm_y-6, 26, 11, dui->palette[6]);
				dui->DrawBarAlpha(pm_x, pm_y+5, 26, 2, dui->palette[6], 0.3f);
//				char strc[16];
//				sprintf(strc, "%s (%i)", GetNoteStr(pm_note), pm_note);
				dui->DrawText(pm_x+3, pm_y-5, dui->palette[6], GetNoteStr(pm_note));
			}
		}
	}
	else
	{
		// atomic instruments
		if(winactive)
		{
			if(spacescroll)
			{
				int iwidth=winw-120-1;
				pscrollx-=(float)dui->mousedx/(1+scrollsize-iwidth);
			}
			if(!dui->lmbdown)
				spacescroll=false;
		}

		dui->StartScrollArea(100, 12, winw-100-1, winh-33, pscrollx, pscrolly, scrollsize, winh-33, dui->palette[10], "partscroll");
		{
			int iwidth=winw-100-1;
			int iscrollx=(int)((scrollsize-iwidth)*pscrollx);
			// piano roll area
			for(int x=0;x<scrollsize;x+=16)
			{
				if(x<(int)(pscrollx*(scrollsize-iwidth+2)))
					continue;
				if(x>(int)(pscrollx*(scrollsize-iwidth+2)+iwidth+1))
					break;
				if(x%beatlength==0)
					dui->DrawBar(x-1, 0, 3, winh, dui->palette[17]);
				else if(x%32==0)
					dui->DrawBar(x, 0, 1, winh, dui->palette[17]);
				else
					dui->DrawBar(x, 0, 1, winh, dui->palette[8]);
			}
			for(int y=0;y<12;y++)
			{
				dui->DrawBar(0, 18+y*14-1, scrollsize, 2, dui->palette[17]);
				char string[4];
				sprintf(string, "%i", 12-y);
				for(int x=0;x<1600;x+=128)
				{
					if(x<(int)(pscrollx*(scrollsize-iwidth+2)-20))
						continue;
					if(x>(int)(pscrollx*(scrollsize-iwidth+2)+iwidth+1))
						break;
					dui->DrawText(x, 18+y*14-4, dui->palette[10], string);
					dui->DrawText(x+2, 18+y*14-4, dui->palette[10], string);
					dui->DrawText(x+1, 18+y*14-4, dui->palette[9], string);
				}
			}

			if(winactive)
			{
				// Visualize keypresses
				if(!dui->ctrl_down)
					for(int kn=0;kn<120;kn++)
						if(IsKeyDown(kn))
						{
							int ky=18+11*14-(kn%12)*14;
							dui->DrawBar(0, ky-2, scrollsize, 4, dui->palette[9]);
						}
				if(dui->delete_pressed && msnum>0) // delete selected triggers
				{
					for(int i=0;i<numtriggers;i++)
						if(triggers[i].selected)
						{
							RemoveTrigger(i);
							i--; // check this spot again since the list was reordered
						}
					msnum=0; // deselect all triggers
				}
			}










			if(prec)
			{
				prec_curtime=(int)((float)(audiostream->GetTick(0)-prec_start)*1600/GetTempo());
				dui->DrawBar((int)(prec_curtime*zoom), 0, 1, winh, dui->palette[0]);
			}
			if(pplay)
			{
				int px=(int)((audiostream->GetTick(0)-pplay_offset)*1600*zoom/GetTempo());
				dui->DrawBar(px, 0, 1, winh, dui->palette[9]);

				if(vlooped)
				{
					vlooped=false;
					vloopcounter++;
					pplay_offset=audiostream->GetTick(0);
				}
			}
			else if(!Ended())
			{
				int px=(int)((playtime-(looptime-starttime)*loopcounter)*zoom);
				dui->DrawBar(px, 0, 1, winh, dui->palette[9]);
			}








			if(looptime>0)
				dui->DrawBar((int)(looptime*zoom)-1, 0, 3, winh, dui->palette[9]);
			if(starttime>0)
				dui->DrawBar((int)(starttime*zoom)-1, 0, 3, winh, dui->palette[15]);
				
			if(winactive && dui->MouseInZone(winx+100, winy+12, iwidth-7, winh-42))
			{
				if(dui->lmbclick && dui->space_down)
					spacescroll=true;

				int mx=dui->mousex+2-dui->gui_winx;
				//if(snap)
					mx=mx/4*4;
				int mnote=-1;
				int ny=-1;
				for(int i=0;i<12;i++)
				{
					ny=18+11*14-i*14;
					if(abs(ny-(dui->mousey-dui->gui_winy))<9)
					{
						mnote=i+4*12;
						break;
					}
				}
				if(mnote!=-1 && !sbox_active && !dui->space_down)
				{
					dui->DrawBar(mx, 0, 2, winh, dui->palette[11]);
					dui->DrawBar(iscrollx, ny-1, iwidth+2, 2, dui->palette[11]);
				}
				int curtime=(int)(mx/zoom);
				if(!spacescroll)
				{
					if(dui->lmbclick && !dui->shift_down && msnum>0 && mnote!=-1) // start moving multiple notes
					{
						mst1=curtime;
						msn1=mnote;
					}
					if(dui->lmbdown && !dui->shift_down && msnum>0 && msn1!=-1) // move multiple
					{
						int dt=curtime-mst1;
						int dn=mnote-msn1;
						if(mnote!=-1 && (dt!=0 || dn!=0))
						{
							for(int i=0;i<msnum;i++)
								mstriggers[i]=triggers[msitriggers[i]];
							for(int i=0;i<numtriggers;i++)
								if(triggers[i].selected)
								{
									RemoveTrigger(i);
									i--; // check this spot again since the list was reordered
								}
							for(int i=0;i<msnum;i++)
							{
								Trigger *trig=&mstriggers[i];
	//							int id=mstriggers[i];
	//							int tnote=triggers[id].note+dn;
	//							int ttime=triggers[id].start+dt;
	//							ttime=(int)(((int)(ttime/zoom)/4*4)*zoom); // snap
	//							int tdur=triggers[id].duration;
								trig->start+=dt;
								trig->note+=dn;
	//							float tvol=triggers[id].volume;
	//							RemoveTrigger(id);
								int id=InsertTrigger(trig->note, trig->start, trig->duration, trig->volume);
								triggers[id].selected=true;
							}
							int nn=0;
							for(int i=0;i<numtriggers;i++)
								if(triggers[i].selected)
									msitriggers[nn++]=i;
							mst1+=dt;
							msn1+=dn;
						}
					}
					if(dui->rmbdown && !dui->shift_down) // delete trigger
					{
						if(placing_loop) // remove loop
						{
							looptime=0;
							placing_loop=false;
						}
						else if(placing_start) // remove start
						{
							starttime=0;
							placing_start=false;
						}
						else if(mnote!=-1)
						{
							int oldtrigger=FindTrigger(curtime, mnote);
							if(oldtrigger!=-1)
								RemoveTrigger(oldtrigger);
						}
					}
					if(dui->lmbclick && !dui->shift_down) // place new trigger
					{
						// deselect multi
	//					for(int i=0;i<numtriggers;i++)
	//						triggers[i].selected=false;
	//					msnum=0;
	
						if(placing_loop) // place loop position
						{
							looptime=curtime;
							placing_loop=false;
						}
						else if(placing_start) // place start position
						{
							starttime=curtime;
							placing_start=false;
						}
						else if(mnote!=-1 && msnum<=0)
						{
							if(!dui->shift_down)
							{
								int oldtrigger=FindTrigger(curtime, mnote);
								if(oldtrigger==-1)
								{
									if(!dui->ctrl_down)
										InsertTrigger(mnote, curtime, 1000, 1.0f);
									if(!pplay || dui->ctrl_down)
										gearstack->instrument->Trigger(mnote, 1.0f, 2000);
								}
							}
						}
					}
					if(dui->lmbclick && dui->shift_down) // start selection box
					{
						sbox_active=true;
						sbox_x1=dui->mousex-dui->gui_winx;
						sbox_y1=dui->mousey-dui->gui_winy;
						sbox_x2=sbox_x1;
						sbox_y2=sbox_y1;
					}
					if(dui->lmbdown) // multi-select or move
					{
						if(dui->ctrl_down && msnum==0) // select note for ctrl-move
						{
							int hittrigger=FindTrigger(curtime, mnote);
							if(hittrigger!=-1 && !triggers[hittrigger].selected)
							{
								msitriggers[msnum++]=hittrigger;
								triggers[hittrigger].selected=true;
							}
							if(dui->ctrl_down) // start moving immediately
							{
								ctrl_move=true;
								mst1=curtime;
								msn1=mnote;
							}
						}
					}
					else if(ctrl_move) // was in move-one mode, deselect it upon mouse release
					{
						ctrl_move=false;
						for(int i=0;i<numtriggers;i++)
							triggers[i].selected=false;
						msnum=0;
					}
				}
			}
			for(int i=0;i<numtriggers;i++) // ********** only check the ones potentially within visible window
			{
				Trigger *trig=&triggers[i];
				int nx=(int)(trig->start*zoom);
				int nd=(int)(trig->duration*zoom)+1;
//					int ny=400/8*8-4-GetNotePos(trig->note)*4;
				int ny=18+11*14-(trig->note%12)*14;
				int color=6;
				if(trig->selected)
					color=9;
//				dui->DrawBar(nx-1, ny-3, 3, 6, dui->palette[color]);
//				dui->DrawBar(nx-2, ny-2, 5, 4, dui->palette[color]);
				Color trigcol;
				float iscale=0.55f;
				trigcol.r=iscale-pow(trig->volume, 2.5f)*iscale;
				trigcol.g=trigcol.r;
				trigcol.b=trigcol.r;
				if(color==9)
					trigcol=dui->palette[color];
				dui->DrawBar(nx-1, ny-3, 3, 6, trigcol);
				dui->DrawBar(nx-2, ny-2, 5, 4, trigcol);
//					dui->DrawBar(nx, ny-1, nd, 2, dui->palette[color]);
			}
		}
//		dui->EndScrollArea();
	}

	if(sbox_active)
	{
		if(dui->lmbdown)
		{
			sbox_x2=dui->mousex-dui->gui_winx;
			sbox_y2=dui->mousey-dui->gui_winy;
			dui->DrawBarAlpha(sbox_x1, sbox_y1, sbox_x2-sbox_x1, sbox_y2-sbox_y1, dui->palette[1], 0.2f);
			dui->DrawBox(sbox_x1, sbox_y1, sbox_x2-sbox_x1, sbox_y2-sbox_y1, dui->palette[9]);
		}
		else if(!dui->lmbdown)
		{
			// select enclosed triggers
			if(sbox_x1>sbox_x2)
			{
				int t=sbox_x1;
				sbox_x1=sbox_x2;
				sbox_x2=t;
			}
			if(sbox_y1>sbox_y2)
			{
				int t=sbox_y1;
				sbox_y1=sbox_y2;
				sbox_y2=t;
			}

			int mx1=sbox_x1+2;
			int mx2=sbox_x2+2;
			int my1=sbox_y1;
			int my2=sbox_y2;
			int mnote1=-1;
			int mnote2=-1;
			int tmnote1=-1;
			int tmnote2=-1;
			if(!instrument->Atomic())
			{
				int ny=0;
				for(int i=0;i<120;i++)
				{
					ny=notearea/8*8-4-GetNotePos(i)*4;
					if(mnote1==-1 && abs(ny-my1)<5)
						tmnote1=i;
					if(abs(ny-(my1))<3)
						mnote1=i;
					if(mnote2==-1 && abs(ny-my2)<5)
						tmnote2=i;
					if(abs(ny-(my2))<3)
						mnote2=i;
				}
				if(mnote1==-1 && tmnote1>-1)
					mnote1=tmnote1;
				if(mnote2==-1 && tmnote2>-1)
					mnote2=tmnote2;
				if(mnote1==-1)
					if(my1<ny)
						mnote1=121;
			}
			else
			{
				int ny=0;
				for(int i=0;i<12;i++)
				{
					ny=22+11*14-i*14;
					if(abs(ny-my1)<9)
						mnote1=i+4*12;
					if(abs(ny-my2)<9)
						mnote2=i+4*12;
				}
				if(mnote1==-1)
					if(my1<ny)
						mnote1=100;
			}
			int curtime1=(int)(mx1/zoom);
			int curtime2=(int)(mx2/zoom);

			for(int i=0;i<numtriggers;i++)
			{
				if(triggers[i].selected)
					continue;
				if(triggers[i].start<=curtime2 && triggers[i].start+triggers[i].duration>=curtime1 && triggers[i].note>=mnote2 && triggers[i].note<=mnote1)
				{
					msitriggers[msnum++]=i;
					triggers[i].selected=true;
				}
			}
			sbox_active=false;
		}
	}
	dui->EndScrollArea();
/*
	// parameter curve (directly visible or transparent overlay)
//	dui->StartScrollArea(120, 12, winw-120-1, winh-33, pscrollx, pscrolly, scrollsize, notearea, dui->palette[10], "partscroll");
	float bogus=0.0f;
	dui->ShadowScrollArea(120, 12, winw-120-1-8, winh-33-8, pscrollx, bogus, scrollsize, winw-120-1-8, dui->palette[0], "pparscroll");
	{
		int iwidth=winw-120-1;
		int iscrollx=(int)((scrollsize-iwidth)*pscrollx);
		int ycenter=(winh-33-8)/2;
		// vertical extremes
		dui->DrawBarAlpha(0, ycenter-62, iwidth, 4, dui->palette[0], 0.2f);
		dui->DrawBarAlpha(0, ycenter+58, iwidth, 4, dui->palette[0], 0.2f);
		// center
		dui->DrawBarAlpha(0, ycenter, iwidth, 3, dui->palette[0], 0.15f);
		// actual parameter curve
		dui->DrawLineAlpha(0, ycenter, 100, ycenter, 3, dui->palette[1], 0.3f);
		dui->DrawLineAlpha(100, ycenter, 200, ycenter-50, 3, dui->palette[1], 0.3f);
		dui->DrawLineAlpha(200, ycenter-50, iwidth, ycenter-50, 3, dui->palette[1], 0.3f);
	}
	dui->EndScrollArea();
*/	

	// piano keyboard illustration
	int tvolx=100;
	if(!instrument->Atomic())
	{
		int hovernote=-1;
		tvolx=120;
		dui->ShadowScrollArea(100, 12, 20, winh-33-8, pscrollx, pscrolly, 20, notearea, dui->palette[9], "pkeysscroll");
		{
			bool mouseinside=dui->MouseInZone(dui->gui_winx, winy+12, 20, winh-33-8);
			int cn=120;
			for(int y=0;y<notearea+8;y+=8) // check for mouse hover on white keys
			{
				if(mouseinside && dui->MouseInZone(dui->gui_winx, dui->gui_winy+y-7, 20, 8))
					hovernote=cn;
				switch(((y/8)%7))
				{
				case 1:
				case 2:
				case 3:
				case 5:
				case 6:
					cn--;
					break;
				}
				cn--;
			}
			cn=120;
			for(int y=0;y<notearea+8;y+=8) // check for mouse hover on black keys
			{
				switch(((y/8)%7))
				{
				case 1:
				case 2:
				case 3:
				case 5:
				case 6:
					cn--;
					if(mouseinside && dui->MouseInZone(dui->gui_winx, dui->gui_winy+y-2, 13, 5))
						hovernote=cn;
					break;
				}
				cn--;
			}
			for(int y=0;y<notearea+8;y+=8) // C keys
				if(((y/8)%7)==0)
					dui->DrawBar(0, y-7, 20, 7, dui->palette[18]);
			cn=120;
			for(int y=0;y<notearea+8;y+=8) // draw white keys
			{
				if(cn==hovernote)
					dui->DrawBar(0, y-7, 20, 7, dui->palette[11]);
				switch(((y/8)%7))
				{
				case 1:
				case 2:
				case 3:
				case 5:
				case 6:
					cn--;
					break;
				}
				cn--;
			}
			cn=120;
			for(int y=0;y<notearea+8;y+=8) // draw black keys and dividers between white keys
			{
				dui->DrawBar(0, y, 20, 1, dui->palette[8]);
				switch(((y/8)%7))
				{
				case 1:
				case 2:
				case 3:
				case 5:
				case 6:
					cn--;
					int color=6;
					if(cn==hovernote) color=11;
//					dui->DrawBar(0, y, 20, 1, dui->palette[]);
					dui->DrawBar(0, y-2, 13, 5, dui->palette[color]);
					break;
				}
				cn--;
			}
			dui->DrawBar(19, 0, 1, notearea, dui->palette[7]);
		}
		dui->EndScrollArea();
		dui->DrawBar(100, 12+winh-33-8+1, 20, 32+8, dui->palette[8]);
		dui->DrawBox(99, 12+winh-33-8+1, 21, 32+8, dui->palette[6]);
		// display name of hovered-over note (if any)
		if(hovernote!=-1)
		{
			int pm_x=dui->mousex-winx+12;
			int pm_y=dui->mousey-winy;
			dui->DrawBar(pm_x, pm_y-6, 26, 11, dui->palette[9]);
			dui->DrawBox(pm_x, pm_y-6, 26, 11, dui->palette[6]);
			dui->DrawBarAlpha(pm_x, pm_y+5, 26, 2, dui->palette[6], 0.3f);
			dui->DrawText(pm_x+3, pm_y-5, dui->palette[6], GetNoteStr(hovernote));
			
			// play clicked note
			if(dui->lmbclick)
			{
				if(playing_note!=-1)
					gearstack->instrument->Release(playing_note);
				gearstack->instrument->Trigger(hovernote, 1.0f);
				playing_note=hovernote;
			}
		}

		// mouse not pressed, release playing note if there is one
		if(!(dui->lmbclick || dui->lmbdown) && playing_note!=-1)
		{
			gearstack->instrument->Release(playing_note);
			playing_note=-1;
		}
	}

	// trigger volume area
	dui->DrawBar(winw-9, 12+winh-33, 1, 32, dui->palette[6]);
	dui->DrawBar(winw-8, 12+winh-33, 7, 32, dui->palette[4]);
//	dui->ShadowScrollArea(100, 12+winh-33, winw-100-9, 32, pscrollx, pscrolly, scrollsize, 32, dui->palette[14], "ptvolscroll");
	dui->ShadowScrollArea(tvolx, 12+winh-33, winw-tvolx-9, 32, pscrollx, pscrolly, scrollsize, 32, dui->palette[14], "ptvolscroll");
	{
//		dui->DrawBar(100, 10, 4, 2, dui->palette[3]);
		int iwidth=winw-tvolx-1;
		int iscrollx=(int)((scrollsize-iwidth)*pscrollx);
		dui->DrawBar(iscrollx-1, 2, iwidth+2, 1, dui->palette[6]);
		dui->DrawBar(iscrollx-1, 3+26, iwidth+2, 1, dui->palette[6]);
		for(int y=0;y<26;y+=4)
			dui->DrawBar(iscrollx-1, 3+y, iwidth+2, 2, dui->palette[4]);
		for(int x=0;x<scrollsize;x+=16)
		{
			if(x<(int)(pscrollx*(scrollsize-iwidth+2)))
				continue;
			if(x>(int)(pscrollx*(scrollsize-iwidth+2)+iwidth+1))
				break;
			if(x%beatlength==0)
				dui->DrawBar(x-1, 3, 3, 25, dui->palette[7]);
			else if(x%32==0)
				dui->DrawBar(x, 3, 1, 25, dui->palette[7]);
			else
				dui->DrawBar(x, 3, 1, 25, dui->palette[4]);
		}
		// edit note volumes
		static bool clicked_in_notevol=false;
		if(winactive && dui->MouseInZone(winx+tvolx, winy+12+winh-33, winw-tvolx-8, 32))
		{
			int mx=dui->mousex+2-dui->gui_winx;
			mx=mx/4*4;
			int curtime=(int)(mx/zoom);
			float mv=1.0f-(float)((dui->mousey-(dui->gui_winy+4))/2)/12;
			if(mv<0.0f) mv=0.0f;
			if(mv>1.0f) mv=1.0f;
			mv=pow(mv, 1.25f); // use non-linear volume scale
			if(dui->lmbclick)
				clicked_in_notevol=true;
			if(dui->lmbdown && clicked_in_notevol)
			{
				for(int ni=1;ni<32;ni++)
				{
					int itrig=FindTrigger(curtime, -ni);
					if(itrig==-1)
						break;
					triggers[itrig].volume=mv;
				}
			}
		}
		if(winactive && !dui->lmbdown)
			clicked_in_notevol=false;
		// show trigger volumes
		int ptrigs=-1;
		int ptrign=-1;
		for(int i=0;i<numtriggers;i++) // ********** only check the ones potentially within visible window
		{
			Trigger *trig=&triggers[i];
			int nx=(int)(trig->start*zoom);
			int nd=(int)(trig->duration*zoom)+1;
//			int ny=notearea/8*8-4-GetNotePos(trig->note)*4;
			int color=3;
//			if(ptrigs!=-1 && trig->start==ptrigs && trig->note==ptrign) // overlapping, show warning graphic
//				dui->DrawBar(nx-2, ny-3, 5, 7, dui->palette[11]);
			if(trig->selected)
				color=9;
			ptrigs=trig->start;
			ptrign=trig->note;
			float lv=pow(trig->volume, 1.0f/1.25f); // convert to linear volume
			dui->DrawBar(nx-1, (int)(24+3-lv*24), 3, 2, dui->palette[color]);
//			dui->DrawBar(nx, ny-1, nd, 2, dui->palette[color]);
		}
	}
	dui->EndScrollArea();


	// resize button
	dui->DrawSprite(dui->icon_resize, winw-9, 12+winh-9, dui->palette[9]);
	

//	}

//	if(dui->lmbclick && dui->MouseInZone(winx+400-13, winy, 13, 12)) // toggle iconify
/*	if(dui->dlmbclick && dui->MouseInZone(winx, winy, 400-12, 12)) // toggle iconify
	{
		iconify=!iconify;
		if(iconify)
		{
			placing_loop=false;
			// deselect multi
			for(int i=0;i<numtriggers;i++)
				triggers[i].selected=false;
			msnum=0;
			// stop recording
			if(prec)
			{
				prec=false;
				// release any open notes
				for(int i=0;i<numtriggers;i++)
					if(triggers[i].duration==0)
						triggers[i].duration=prec_curtime-triggers[i].start;
				audiostream->metronome=0;
			}
			// stop playing
			if(pplay)
			{
				pplay=false;
				audiostream->RemovePart(this);
				instrument->ReleaseAll();
			}
		}
	}
*/
	if(dui->rmbclick && dui->MouseInZone(winx, winy, winw, 12)) // rename
		dui->EditString(name);

//	if(dui->lmbclick && dui->MouseInZone(winx+400-13, winy, 13, 12)) // hide (prep)
//		dui->mouseregion=90;
//	if(dui->lmbrelease && dui->MouseInZone(winx+400-13, winy, 13, 12) && dui->mouseregion==90) // hide
	if(dui->dlmbclick && dui->MouseInZone(winx, winy, winw, 12)) // hide
	{
		hidden=true;
		placing_loop=false;
		placing_start=false;
		// stop recording
		if(prec)
		{
			prec=false;
			// release any open notes
			for(int i=0;i<numtriggers;i++)
				if(triggers[i].duration==0)
					triggers[i].duration=prec_curtime-triggers[i].start;
			audiostream->metronome=0;
		}
		// stop playing
		if(pplay)
		{
			pplay=false;
			audiostream->RemovePart(this);
			instrument->ReleaseAll();
		}
	}

	dui->EndWindow();

/*	int instrument_offset=212;
	if(iconify)
		instrument_offset=12;
	gearstack->instrument->DefaultWindow(dui, winx, winy+instrument_offset, winz);
	for(int i=0;i<gearstack->num_effects;i++)
		gearstack->effects[i]->DefaultWindow(dui, winx, winy+instrument_offset+50+i*3, winz);
*/
	if(winactive)
		wasactive=true;
	if(wasactive && !winactive)
	{
		wasactive=false;
		// deselect multi (just in case)
		for(int i=0;i<numtriggers;i++)
			triggers[i].selected=false;
		msnum=0;

		if(!playing)
			wantrelease=true;
//			instrument->ReleaseAll();
	}
	dui->NameSpace(NULL);
	
	return winactive;
};

