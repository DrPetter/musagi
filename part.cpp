#include "part.h"
#include "AudioStream.h"

bool Part::DoInterface(DUI *dui, void *astream)
{
	if(hidden)
		return false;

	AudioStream *audiostream=(AudioStream*)astream;
	
	gear_instrument *instrument=gearstack->instrument;

	int maxheight=560;
	if(gearstack->instrument->Atomic())
	{
		maxheight=200;
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
		winactive=dui->StartWindowResize(winx, winy, winw, winh, 300, 200, maxheight, windowtitle, "partwin");

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
				prec_start=audiostream->GetTick()+4*32*320*GetTempo()/16; // count in
				audiostream->metronome=32*320*GetTempo()/16;
				audiostream->metrocount=0;
			}
			else
				prec_start=audiostream->GetTick();
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
	{
		if(!pplay) // about to start
		{
			prec=false;
			pplay_start=audiostream->GetTick();
			pplay_offset=pplay_start;
			vlooped=false;
			vloopcounter=0;
			Rewind(99999);
			audiostream->AddPart(this);
		}
		pplay=!pplay;
		if(!pplay) // stopped
		{
			audiostream->RemovePart(this);
			instrument->ReleaseAll();
		}
	}
	
	dui->DrawText(38, 72, dui->palette[6], "place loop");
	dui->DrawLED(5, 71, 0, placing_loop);
	if(dui->DoButton(15, 71, "btn_plooptime", "set or clear loop position - press this then click in timeline"))
		placing_loop=!placing_loop;

	dui->DrawBar(0, 99, 100, 1, dui->palette[6]);
	dui->DrawBar(0, 101, 100, 1, dui->palette[6]);

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
					//MessageBox(NULL, "Unknown part format (maybe incompatible version)", "Error", MB_ICONEXCLAMATION);
                                        printf("Unknown part format (maybe incompatible version)\n");
				fclose(file);
			}
			else
				//MessageBox(NULL, "File not found", "Error", MB_ICONEXCLAMATION);
                                printf("File not found\n");
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
				//MessageBox(NULL, "Unable to save (write protected?)", "Error", MB_ICONEXCLAMATION);
                                printf("Unable to save (write protected?)");
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
	if(dui->DoButton(75, winh-14, "btn_piset", "set to currently selected instrument"))
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

	if(msnum==0 && PartCopy_NumTriggers()>0 && winactive && dui->ctrl_down && dui->key_v) // ctrl+V = paste triggers
	{
		// deselect
		for(int i=0;i<numtriggers;i++)
			triggers[i].selected=false;
		msnum=0;

		// paste triggers, then select them so they're easy to move
		int pcstart=-1;
		for(int i=0;i<PartCopy_NumTriggers();i++)
			if(pcstart==-1 || PartCopy_Triggers(i).start<pcstart)
				pcstart=PartCopy_Triggers(i).start;
		int iwidth=winw-100-1;
		int offset=(int)((scrollsize-iwidth)*pscrollx)/4*4*320;
		if(offset+100*320==pcstart)
			offset+=12*320;
		for(int i=0;i<PartCopy_NumTriggers();i++)
		{
			int start=PartCopy_Triggers(i).start-pcstart+offset+100*320;
			int duration=PartCopy_Triggers(i).duration;
			int note=PartCopy_Triggers(i).note;
			int nt=InsertTrigger(note, start, duration, 1.0f);
			triggers[nt].selected=true;
		}
		for(int i=0;i<numtriggers;i++)
			if(triggers[i].selected)
				msitriggers[msnum++]=i;
	}

	float zoom=1.0f/320.0f;
	int notearea=0;
	AdjustScrollsize();

	if(!instrument->Atomic())
	{
		notearea=560;
		if(winactive)
			pscrolly-=(float)dui->mousewheel*0.05f;
		// regular tonal instrument
//		dui->StartScrollArea(100, 12, 300-1, 200-1, pscrollx, pscrolly, scrollsize, notearea, dui->palette[10], "partscroll");
		dui->StartScrollArea(100, 12, winw-100-1, winh-1, pscrollx, pscrolly, scrollsize, notearea, dui->palette[10], "partscroll");
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
				if(x%128==0)
					dui->DrawBar(x-1, 0, 3, notearea, dui->palette[7]);
				else if(x%32==0)
					dui->DrawBar(x, 0, 1, notearea, dui->palette[7]);
				else
					dui->DrawBar(x, 0, 1, notearea, dui->palette[8]);
			}
			for(int y=0;y<notearea;y+=8)
			{
				switch(((y/8)%7))
				{
				case 0:
				case 4:
					dui->DrawBar(iscrollx-1, y, iwidth+2, 1, dui->palette[8]);
					if(((y/8)%7)==4)
						for(int x=0;x<scrollsize;x+=32)
						{
							if(x<(int)(pscrollx*(scrollsize-iwidth+2)))
								continue;
							if(x>(int)(pscrollx*(scrollsize-iwidth+2)+iwidth+1))
								break;
							dui->DrawBar(x-2, y, 5, 1, dui->palette[5]);
						}
					break;
				default:
					dui->DrawBar(iscrollx-1, y-1, iwidth+2, 2, dui->palette[7]);
					break;
				}
			}

			if(winactive)
			{
				// Visualize keypresses
				for(int kn=0;kn<120;kn++)
					if(IsKeyDown(kn))
					{
						int ky=notearea/8*8-5-GetNotePos(kn)*4;
						if(NoteIsBlack(kn))
							dui->DrawBar(iscrollx-1, ky, iwidth+2, 2, dui->palette[9]);
						else
							dui->DrawBar(iscrollx-1, ky-1, iwidth+2, 4, dui->palette[9]);
					}
			}

			if(prec)
			{
				prec_curtime=(int)((float)(audiostream->GetTick()-prec_start)*16/GetTempo());
				dui->DrawBar((int)(prec_curtime*zoom), 0, 1, notearea, dui->palette[11]);
			}
			if(pplay)
			{
				int px=(int)((audiostream->GetTick()-pplay_offset)*16*zoom/GetTempo());
				dui->DrawBar(px, 0, 1, notearea, dui->palette[9]);

				if(vlooped)
				{
					vlooped=false;
					vloopcounter++;
					pplay_offset=audiostream->GetTick();
				}
			}
			if(looptime>0)
				dui->DrawBar((int)(looptime*zoom)-1, 0, 3, notearea, dui->palette[9]);
				
			if(winactive && dui->MouseInZone(winx+100, winy+12, winw-100-8, winh-10))
			{
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
				if(mnote!=-1 && !sbox_active)
				{
					dui->DrawBar(mx, 0, 2, notearea, dui->palette[11]);
					if(edit_curtrigger==-1)
						dui->DrawBar(iscrollx-1, ny-1, iwidth+2, 2, dui->palette[11]);
				}
				int curtime=(int)(mx/zoom);
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
					else if(mnote!=-1)
					{
						if(!dui->shift_down && msnum<=0)
						{
							int oldtrigger=FindTrigger(curtime, mnote);
							if(oldtrigger==-1)
							{
								edit_curtrigger=InsertTrigger(mnote, curtime, 10, 1.0f);
								if(!pplay)
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
				dui->DrawBar(nx-1, ny-2, 3, 4, dui->palette[color]);
				dui->DrawBar(nx, ny-1, nd, 2, dui->palette[color]);
				if(trig->slidelength>-1)
				{
					int sd=(int)(trig->slidelength*zoom);
					int ny2=notearea/8*8-4-GetNotePos(trig->slidenote)*4;
					if(color==6)
						color=15;
					dui->DrawLine(nx+nd-1, ny, nx+nd+sd, ny2, 2, dui->palette[color]);
				}
			}
		}
	}
	else
	{
		// atomic instruments
		dui->StartScrollArea(100, 12, winw-100-1, winh-1, pscrollx, pscrolly, scrollsize, winh, dui->palette[10], "partscroll");
//		dui->StartScrollArea(100, 12, 300-1, 200-1, pscrollx, pscrolly, scrollsize, 200-1, dui->palette[10], "partscroll");
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
				if(x%128==0)
					dui->DrawBar(x-1, 0, 3, winh, dui->palette[7]);
				else if(x%32==0)
					dui->DrawBar(x, 0, 1, winh, dui->palette[7]);
				else
					dui->DrawBar(x, 0, 1, winh, dui->palette[8]);
			}
			for(int y=0;y<12;y++)
			{
				dui->DrawBar(0, 22+y*14-1, scrollsize, 2, dui->palette[7]);
				char string[4];
				sprintf(string, "%i", 12-y);
				for(int x=0;x<1600;x+=128)
				{
					if(x<(int)(pscrollx*(scrollsize-iwidth+2)-20))
						continue;
					if(x>(int)(pscrollx*(scrollsize-iwidth+2)+iwidth+1))
						break;
					dui->DrawText(x, 22+y*14-4, dui->palette[10], string);
					dui->DrawText(x+2, 22+y*14-4, dui->palette[10], string);
					dui->DrawText(x+1, 22+y*14-4, dui->palette[9], string);
				}
			}

			if(winactive)
			{
				// Visualize keypresses
				for(int kn=0;kn<120;kn++)
					if(IsKeyDown(kn))
					{
						int ky=22+11*14-(kn%12)*14;
						dui->DrawBar(0, ky-2, scrollsize, 4, dui->palette[9]);
					}
			}

			if(prec)
			{
				prec_curtime=(int)((float)(audiostream->GetTick()-prec_start)*16/GetTempo());
				dui->DrawBar((int)(prec_curtime*zoom), 0, 1, winh, dui->palette[0]);
			}
			if(pplay)
			{
				int px=(int)((audiostream->GetTick()-pplay_offset)*16*zoom/GetTempo());
				dui->DrawBar(px, 0, 1, winh, dui->palette[9]);

				if(vlooped)
				{
					vlooped=false;
					vloopcounter++;
					pplay_offset=audiostream->GetTick();
				}
			}
			if(looptime>0)
				dui->DrawBar((int)(looptime*zoom)-1, 0, 3, winh, dui->palette[9]);
				
			if(winactive && dui->MouseInZone(winx+100, winy+12, iwidth-7, winh-10))
			{
				int mx=dui->mousex+2-dui->gui_winx;
				//if(snap)
					mx=mx/4*4;
				int mnote=-1;
				int ny=-1;
				for(int i=0;i<12;i++)
				{
					ny=22+11*14-i*14;
					if(abs(ny-(dui->mousey-dui->gui_winy))<9)
					{
						mnote=i+4*12;
						break;
					}
				}
				if(mnote!=-1 && !sbox_active)
				{
					dui->DrawBar(mx, 0, 2, winh, dui->palette[11]);
					dui->DrawBar(iscrollx, ny-1, iwidth+2, 2, dui->palette[11]);
				}
				int curtime=(int)(mx/zoom);
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
					else if(mnote!=-1)
					{
						int oldtrigger=FindTrigger(curtime, mnote);
						if(oldtrigger!=-1)
							RemoveTrigger(oldtrigger);
					}
				}
				if(dui->lmbclick && !dui->shift_down && !dui->ctrl_down) // place new trigger
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
					else if(mnote!=-1 && msnum<=0)
					{
						if(!dui->shift_down)
						{
							int oldtrigger=FindTrigger(curtime, mnote);
							if(oldtrigger==-1)
								InsertTrigger(mnote, curtime, 10, 1.0f);
//								else
//									edit_curtrigger=oldtrigger;
							if(!pplay)
								gearstack->instrument->Trigger(mnote, 1.0f, 2000);
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
			for(int i=0;i<numtriggers;i++) // ********** only check the ones potentially within visible window
			{
				Trigger *trig=&triggers[i];
				int nx=(int)(trig->start*zoom);
				int nd=(int)(trig->duration*zoom)+1;
//					int ny=400/8*8-4-GetNotePos(trig->note)*4;
				int ny=22+11*14-(trig->note%12)*14;
				int color=6;
				if(trig->selected)
					color=9;
				dui->DrawBar(nx-1, ny-3, 3, 6, dui->palette[color]);
				dui->DrawBar(nx-2, ny-2, 5, 4, dui->palette[color]);
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
	if(dui->rmbclick && dui->MouseInZone(winx, winy, 400, 12)) // rename
		dui->EditString(name);

//	if(dui->lmbclick && dui->MouseInZone(winx+400-13, winy, 13, 12)) // hide (prep)
//		dui->mouseregion=90;
//	if(dui->lmbrelease && dui->MouseInZone(winx+400-13, winy, 13, 12) && dui->mouseregion==90) // hide
	if(dui->dlmbclick && dui->MouseInZone(winx, winy, 400, 12)) // hide
	{
		hidden=true;
		placing_loop=false;
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

