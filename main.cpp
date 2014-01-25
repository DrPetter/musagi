#include "glkit.h"

#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "DPInput.h"

DPInput *input;

int kbkbk[50];
int kbkbm[50];
bool kbkbkeydown[50];
bool kbkbsomething;
int kbkboctave;

int midioctave;



#include "musagi.h"
#include "musagig.h"

#include "dui.h"

#include "AudioStream.h"
#include "pa_callback.h"

#include "gin_protobass.h"
#include "gin_xnes.h"
#include "gin_chip.h"
#include "gin_swave.h"
#include "gin_vsampler.h"

#include "gef_gapan.h"

#include "part.h"

#include "song.h"

AudioStream *audiostream;

DUI *dui;
/*
//gin_protobass *protobass;
gin_squaredaddy *square_daddy;
gin_trianglemommy *triangle_mommy;
gin_deffector *deffector;
gin_deffector *deffector2;
*/
GearStack gearstacks[128];
int maxstacks=128;
/*
GearStack phatstack;
GearStack philstack;
GearStack puffstack;
GearStack paffstack;
*/
Part *mungoparts[1024];
int maxparts=1024;

Song *thesong;

bool diskrec=false;

int log_on=0;
int midi_id=0;
int scrollfollow=1;
char defpath[512];

char curfilename[512];
int autosave=0;

int errorcount;

float ftempo=0.5f;
float master_volume=0.5f;

bool shortkeydown=false;
bool fpressed=false;
int fcurstack=0;
int stacklist[128];
int stacklistnum=0;

int GetTick()
{
	return audiostream->GetTick();
}


void Reset();

void Keyboard_KeyDown(int note)
{
	if(dui->editstring || note<0 || note>=120)
		return;
	if(current_part!=NULL)
	{
		current_part->gearstack->instrument->Trigger(note, 1.0f);
		if(current_part->prec)
			current_part->RecTrigger(note, 1.0f, current_part->gearstack->instrument->Atomic());
	}
	else if(gearstacks[fcurstack].instrument!=NULL)
	{
//		LogPrint("main: trigger instrument %i [%.8X]", fcurstack, gearstacks[fcurstack].instrument);
		gearstacks[fcurstack].instrument->Trigger(note, 1.0f);
	}
	SetKeyDown(note, true);
}

void Keyboard_KeyUp(int note)
{
	if(dui->editstring || note<0 || note>=120)
		return;
	if(current_part!=NULL && !current_part->gearstack->instrument->Atomic())
	{
		current_part->gearstack->instrument->Release(note);
		if(current_part->prec)
			current_part->RecRelease(note);
	}
	else if(gearstacks[fcurstack].instrument!=NULL)
		gearstacks[fcurstack].instrument->Release(note);
	SetKeyDown(note, false);
}

void Keyboard_AllUp()
{
	if(current_part!=NULL)
		current_part->gearstack->instrument->ReleaseAll();
	else if(gearstacks[fcurstack].instrument!=NULL)
		gearstacks[fcurstack].instrument->ReleaseAll();
	for(int i=0;i<120;i++)
		SetKeyDown(i, false);
}

// TODO: Port midi_io.h to something more portable so we can actually use it.
//#include "midi_io.h"

void ToggleStackWindow(int gsi)
{
	gearstacks[gsi].winshow=!gearstacks[gsi].winshow;
	if(gearstacks[gsi].winshow) // show this window, topmost
	{
		gearstacks[gsi].winpop=true;
		stacklist[stacklistnum++]=gsi;
	}
	else // hide this window, remove it from the stacklist
	{
                int si;
		for(si=0;si<stacklistnum;si++)
			if(stacklist[si]==gsi)
				break;
		for(int i=si;i<stacklistnum-1;i++)
			stacklist[i]=stacklist[i+1];
		stacklistnum--;
	}
}


#include "main_fileops.h"


//----------------------------------------

int progress=0;
int progressmax=26;
int progressmark;
bool progressdone=false;

void glkRenderFrame()
{
	bool shortkey=false;
	
	if(!progressdone) // render progress bar to indicate load progress
	{
		int px=(int)((float)progress/progressmax*(1024-40));
		if(px>1024-40)
			px=1024-40;
		glBegin(GL_QUADS);
		glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
		glVertex3i(20, 730, 0);
		glVertex3i(20, 740, 0);
		glVertex3i(1024-20, 740, 0);
		glVertex3i(1024-20, 730, 0);
		glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
		glVertex3i(20, 730, 0);
		glVertex3i(20, 740, 0);
		glVertex3i(20+px, 740, 0);
		glVertex3i(20+px, 730, 0);
		glEnd();
		return;
	}

	dui->Update(input);

	dui->NameSpace((void*)1);
	dui->StartFlatWindow(0, 0, glkitGetWidth(), glkitGetHeight(), "mainw");

//	dui->DrawTexturedBar(0, 0, glkitGetWidth(), 20, dui->palette[9], dui->brushed_metal, &dui->vglare);
	dui->DrawTexturedBar(0, 0, 500, 60, dui->palette[10], dui->brushed_metal, &dui->vglare);
//	dui->DrawTexturedBar(500, 0, 1, 60, dui->palette[4], dui->brushed_metal, NULL);
	dui->DrawBar(500, 0, 1, 60, dui->palette[6]);
	dui->DrawBar(422, 0, 1, 60, dui->palette[6]);
	dui->DrawTexturedBar(501, 0, glkitGetWidth()-501, 60, dui->palette[16], dui->brushed_metal, NULL);
	dui->DrawTexturedBar(0, glkitGetHeight()-58, glkitGetWidth(), 58, dui->palette[16], dui->brushed_metal, NULL);
	glColor4f(1.0f, 1.0f, 1.0f, 0.15f);
	dui->DrawSprite(dui->logo, glkitGetWidth()-250, -2);

	// peak/level indicators
	int level=(int)(audiostream->peak_left*10);
	for(int i=0;i<10;i++)
	{
		if(i==9 && audiostream->clip_left>0)
			dui->DrawLED(80+i*7, 10, 0, true);
		else
			dui->DrawLED(80+i*7, 10, 0, i<level);
	}
	level=(int)(audiostream->peak_right*10);
	for(int i=0;i<10;i++)
	{
		if(i==9 && audiostream->clip_right>0)
			dui->DrawLED(80+i*7, 25, 0, true);
		else
			dui->DrawLED(80+i*7, 25, 0, i<level);
	}

	dui->DrawText(153, 11, dui->palette[6], "left");
	dui->DrawText(153, 26, dui->palette[6], "right");

	dui->DrawText(118, 45, dui->palette[6], "master");
	dui->DoKnob(100, 40, master_volume, -1, "knob_volume", "master volume");
	audiostream->master_volume=pow(master_volume, 2.0f)*0.5f;

	// cpu indicator
	int cpuu=(int)((audiostream->cpu_usage/100)*15);
	dui->DrawBar(190, 11, 8, 46, dui->palette[6]);
	for(int i=0;i<15;i++)
	{
		if(i<=cpuu)
		{
			int color=13;
			if(i>=13)
				color=11;
			dui->DrawBar(191, 12+42-i*3, 6, 2, dui->palette[color]);
			dui->DrawBar(193, 12+42-i*3, 2, 2, dui->palette[9]);
		}
		else
		{
			dui->DrawBar(191, 12+42-i*3, 1, 2, dui->palette[4]);
			dui->DrawBar(196, 12+42-i*3, 1, 2, dui->palette[4]);
		}
	}
	dui->DrawText(200, 11, dui->palette[6], "cpu %2.1f %%", audiostream->cpu_usage);
	
	dui->DrawText(38, 11, dui->palette[6], "play");
	dui->DrawLED(5, 10, 0, thesong->playing);
	bool quickplay=false;
	if(input->KeyPressed(SDLK_F5))
	{
		if(!shortkeydown)
			quickplay=true;
		shortkey=true;
	}
	if(dui->DoButton(15, 10, "songb_play", "(F5) play song from selected position") || quickplay)
	{
		thesong->PlayButton();
/*		if(!thesong->playing)
			for(int i=0;i<maxstacks;i++)
				if(gearstacks[i].instrument!=NULL)
					gearstacks[i].instrument->StopChannels();*/
	}

	dui->DrawText(38, 24, dui->palette[6], "loop");
	dui->DrawLED(5, 23, 2, thesong->loop);
	if(dui->DoButton(15, 23, "songb_loop", "toggle song loop (set using shift+leftclick)"))
		thesong->loop=!thesong->loop;

	dui->DrawText(38, 45, dui->palette[6], "disk rec");
	dui->DrawLED(5, 44, 0, diskrec);
	if(dui->DoButton(15, 44, "btn_drec", "record audio to .wav file"))
	{
		char filename[256];
		if(!diskrec)
		{
			if(dui->SelectFileSave(filename, 3))
			{
				diskrec=true;
				audiostream->StartFileOutput(filename);
			}
		}
		else
		{
			audiostream->StopFileOutput();
			diskrec=false;
		}
	}

	dui->DrawText(318, 11, dui->palette[6], "record knobs");
	dui->DrawLED(318-33, 10, 0, thesong->recknobs);
	bool quickkrec=false;
	if(input->KeyPressed(SDLK_F6))
	{
		if(!shortkeydown)
			quickkrec=true;
		shortkey=true;
	}
	if(dui->DoButton(318-23, 10, "btn_krec", "(F6) record movement of selected knobs during playback (right click on knobs first)") || quickkrec)
	{
		if(!thesong->recknobs && !thesong->playing)
		{
			thesong->recknobs=true;
			thesong->PlayButton();
		}
		else if(thesong->recknobs && thesong->playing)
			thesong->PlayButton();
	}

	dui->DrawText(205, 45, dui->palette[6], "tempo");
	dui->DoKnob(240, 40, ftempo, -1, "knob_tempo", "playback speed");
	tempo=20+(int)(pow(1.0f-ftempo, 2.0f)*60);
//	playtime+=16/tempo; // for each sample
	float bpm=44100.0f/(32*320*tempo/16)*60;
	dui->DrawText(205, 33, dui->palette[6], "%i:", tempo);
	if((int)bpm<100)
		dui->DrawText(205+27, 33, dui->palette[6], "%i bpm", (int)bpm);
	else
		dui->DrawText(205+20, 33, dui->palette[6], "%i bpm", (int)bpm);

	// --- preferences ---
	dui->DrawBar(glkitGetWidth()-200, glkitGetHeight()-60, 1, 60, dui->palette[6]);
	dui->DrawTexturedBar(glkitGetWidth()-199, glkitGetHeight()-58, 198, 58, dui->palette[10], dui->brushed_metal, NULL);
	
	dui->DrawText(glkitGetWidth()-200+35, glkitGetHeight()-53, dui->palette[6], "metronome");
	dui->DrawLED(glkitGetWidth()-200+5, glkitGetHeight()-54, 0, audiostream->metronome_on);
	dui->DoKnob(glkitGetWidth()-200+15, glkitGetHeight()-58, audiostream->metronome_vol, -1, "knob_metvol", "metronome volume (0=off)");
	audiostream->metronome_on=audiostream->metronome_vol>0.0f;

	dui->DrawText(glkitGetWidth()-200+38, glkitGetHeight()-38, dui->palette[6], "tooltips");
	dui->DrawLED(glkitGetWidth()-200+5, glkitGetHeight()-39, 0, dui->tooltips_on);
	if(dui->DoButton(glkitGetWidth()-200+15, glkitGetHeight()-39, "btn_toolt", "toggle tooltips (like this one)"))
		dui->tooltips_on=!dui->tooltips_on;

	dui->DrawText(glkitGetWidth()-100+38, glkitGetHeight()-38, dui->palette[6], "follow");
	dui->DrawLED(glkitGetWidth()-100+5, glkitGetHeight()-39, 0, scrollfollow);
	if(dui->DoButton(glkitGetWidth()-100+15, glkitGetHeight()-39, "btn_scrfo", "scroll to follow play position"))
		scrollfollow=!scrollfollow;

	dui->DrawText(glkitGetWidth()-200+5, glkitGetHeight()-20, dui->palette[6], "%i", kbkboctave);

	bool quickoctup=false;
	if(input->KeyPressed(SDLK_PAGEUP))
	{
		if(!shortkeydown)
			quickoctup=true;
		shortkey=true;
	}
	dui->DrawText(glkitGetWidth()-200+38, glkitGetHeight()-26, dui->palette[6], "octave up");
	if(dui->DoButton(glkitGetWidth()-200+15, glkitGetHeight()-27, "btn_octup", "increase keyboard offset") || quickoctup)
	{
		Keyboard_AllUp();
		if(kbkboctave<8)
		{
			kbkboctave++;
			midioctave++;
		}
	}
	bool quickoctdn=false;
	if(input->KeyPressed(SDLK_PAGEDOWN))
	{
		if(!shortkeydown)
			quickoctdn=true;
		shortkey=true;
	}
	dui->DrawText(glkitGetWidth()-200+38, glkitGetHeight()-14, dui->palette[6], "octave down");
	if(dui->DoButton(glkitGetWidth()-200+15, glkitGetHeight()-15, "btn_octdn", "decrease keyboard offset") || quickoctdn)
	{
		Keyboard_AllUp();
		if(kbkboctave>0)
		{
			kbkboctave--;
			midioctave--;
		}
	}
	// --- preferences ---

/*	if(save_queued)
	{
		strcpy(curfilename, filename);
		if(!SaveSong(filename))
			MessageBox(hWndMain, "Unable to save song (write protected?)", "Error", MB_ICONEXCLAMATION);
//		save_queued=false;
//		dui->calibrate=false;
	}
*/
	if(autosave>0)
		autosave--;
	if(autosave==0 && dui->ctrl_down && input->KeyPressed(SDLK_s))
		autosave=40;
	if(autosave>20)
		dui->DrawText(318, 32, dui->palette[9], "save song");
	else
		dui->DrawText(318, 32, dui->palette[6], "save song");
	if(dui->DoButton(318-23, 31, "btn_save", "save everything") || autosave==40)
	{
		char filename[256];
		bool ok=false;
		if(autosave==40 && curfilename[0]!='\0')
		{
			ok=true;
			strcpy(filename, curfilename);
		}
		else
			ok=dui->SelectFileSave(filename, 0);
		if(ok)
		{
			strcpy(curfilename, filename);
			if(!SaveSong(filename))
				//MessageBox(hWndMain, "Unable to save song (write protected?)", "Error", MB_ICONEXCLAMATION);
                                printf("Unable to save song (write protected?)\n");
//			dui->calibrate=true;
//			save_queued=true;
		}
	}

//	dui->calibration=false; // been around everything at least once, end calibration (if one was in progress)
	
	dui->DrawText(318, 45, dui->palette[6], "load song");
	if(dui->DoButton(318-23, 44, "btn_load", "load song - all changes will be lost!") || (dui->ctrl_down && input->KeyPressed(SDLK_l)))
	{
		thesong->Stop();
		char filename[256];
                // TODO: Fix the file selector and remove the ROTJ hardcoding
		if(1)//dui->SelectFileLoad(filename, 0))
		{
//			dui->calibration=true; // start calibrating knobrec pointers
	
			curfilename[0]='\0';
			if(!LoadSong("songs/rotj.smu"))//filename))
//			{
				//MessageBox(hWndMain, "Unknown song format (maybe incompatible version)", "Error", MB_ICONEXCLAMATION);
                                printf("Unknown song format (maybe incompatible version)\n");
//				calibration=false;
//			}
			else
				strcpy(curfilename, filename);
		}
	}

	// song info
	char string[75];
	dui->DrawText(glkitGetWidth()-505, glkitGetHeight()-52, dui->palette[6], "song info:");
	dui->StartFlatWindow(glkitGetWidth()-450, glkitGetHeight()-53, 246, 41, "msinfo");
	for(int i=0;i<4;i++)
	{
		if(i&1)
			dui->DrawBar(0, i*10, 246, 10, dui->palette[17]);
		else
			dui->DrawBar(0, i*10, 246, 10, dui->palette[8]);
		int y=i*10;
		dui->DrawText(3, y, dui->palette[6], thesong->info_text[i]);
//		if((dui->lmbclick || dui->rmbclick) && dui->mousex>=glkitGetWidth()-450 && dui->mousex<glkitGetWidth()-450+245 && dui->mousey>=y && dui->mousey<y+10)
		if((dui->lmbclick || dui->rmbclick) && dui->MouseInZone(glkitGetWidth()-450, glkitGetHeight()-53+y, 245, 10))
			dui->EditString(thesong->info_text[i]);
	}
	dui->EndWindow();
	dui->DrawBox(glkitGetWidth()-450, glkitGetHeight()-53, 246, 41, dui->palette[6]);

	// new song
	dui->DrawBar(glkitGetWidth()-613, glkitGetHeight()-58, 2, 58, dui->palette[6]);
	dui->DrawBar(glkitGetWidth()-522, glkitGetHeight()-58, 2, 58, dui->palette[6]);
//	dui->DrawTexturedBar(glkitGetWidth()-611, glkitGetHeight()-58, 90, 58, dui->palette[10], dui->brushed_metal, NULL);
	dui->DrawText(glkitGetWidth()-582, glkitGetHeight()-52, dui->palette[6], "new song");
	if(dui->DoButtonRed(glkitGetWidth()-605, glkitGetHeight()-53, "btn_snew", "erase everything and start a new song (unsaved data will be lost)"))
	{
		//int choice=MessageBox(hWndMain, "Are you sure you want to erase everything and start from scratch?", "Caution", MB_ICONEXCLAMATION|MB_OKCANCEL);
                int choice=1; // yeah... the user should had known better!
		if(choice==1)//IDOK)
		{
			LogPrint("main: new song");
			thesong->Stop();
			audiostream->SetSong(NULL);
			audiostream->Flush();
			// clear all data
			LogPrint("main: deleting gearstacks");
			for(int i=0;i<maxstacks;i++)
				if(gearstacks[i].instrument!=NULL)
				{
					for(int j=0;j<gearstacks[i].num_effects;j++)
						delete gearstacks[i].effects[j];
					gearstacks[i].num_effects=0;
					delete gearstacks[i].instrument;
					gearstacks[i].instrument=NULL;
	//				audiostream->RemoveGearStack(&gearstacks[i]);
				}
			LogPrint("main: deleting parts");
			for(int i=0;i<maxparts;i++)
				if(mungoparts[i]!=NULL)
				{
					delete mungoparts[i];
					mungoparts[i]=NULL;
				}
			LogPrint("main: deleting song");
			audiostream->SetSong(NULL);
			delete thesong;
			LogPrint("main: Reset()");
			Reset();
			LogPrint("main: done, new song ready");
		}
	}

	// gearstacks
	dui->DrawText(430, 3, dui->palette[6], "instruments:");
	dui->DrawText(430, 13, dui->palette[6], "load");
	dui->DrawText(467, 13, dui->palette[6], "save");
	if(dui->DoButton(432, 22, "btn_ldinstr", "load instrument into current slot"))
	{
		bool ok=true;
		if(gearstacks[fcurstack].instrument!=NULL)
		{
			int choice=1;//MessageBox(hWndMain, "Loading an instrument here will replace the current one, are you sure?", "Caution", MB_ICONEXCLAMATION|MB_OKCANCEL);
			if(choice!=1)
				ok=false;
		}
		if(ok)
		{
			char filename[256];
			if(dui->SelectFileLoad(filename, 2))
			{
				FILE *file=fopen(filename, "rb");
				if(file==NULL)
					//MessageBox(hWndMain, "File not found", "Error", MB_ICONEXCLAMATION);
                                        printf("File not found\n");
				else
				{
					if(gearstacks[fcurstack].instrument!=NULL)
					{
						audiostream->RemoveGearStack(&gearstacks[fcurstack]);
						for(int i=0;i<gearstacks[fcurstack].num_effects;i++)
							delete gearstacks[fcurstack].effects[i];
						delete gearstacks[fcurstack].instrument;
						gearstacks[fcurstack].instrument=NULL;
					}
					errorcount=0;
					if(!LoadInstrument(&gearstacks[fcurstack], file, false))
						//MessageBox(hWndMain, "Unknown instrument format (maybe incompatible version)", "Error", MB_ICONEXCLAMATION);
                                                printf("Unknown instrument format (maybe incompatible version)\n");
					else
					{
						gearstacks[fcurstack].num_effects=0;
						kfread2(&gearstacks[fcurstack].num_effects, 1, sizeof(int), file);
						for(int j=0;j<gearstacks[fcurstack].num_effects;j++)
						{
							if(!LoadEffect(&gearstacks[fcurstack], j, file, false, 3))
								//MessageBox(hWndMain, "Unknown instrument format (maybe incompatible version)", "Error", MB_ICONEXCLAMATION);
                                                                printf("Unknown instrument format (maybe incompatible version)\n");
						}
						audiostream->AddGearStack(&gearstacks[fcurstack]);
					}
					fclose(file);
				}
			}
		}
	}
	if(dui->DoButton(471, 22, "btn_svinstr", "save current instrument") && gearstacks[fcurstack].instrument!=NULL)
	{
		char filename[256];
		if(dui->SelectFileSave(filename, 2))
		{
			FILE *file=fopen(filename, "wb");
			if(file!=NULL)
			{
				gearstacks[fcurstack].instrument->Save(file);
				kfwrite2(&gearstacks[fcurstack].num_effects, 1, sizeof(int), file);
				for(int j=0;j<gearstacks[fcurstack].num_effects;j++)
					gearstacks[fcurstack].effects[j]->Save(file);
			}
			else
				//MessageBox(hWndMain, "Unable to save instrument (write protected?)", "Error", MB_ICONEXCLAMATION);
                                printf("Unable to save instrument (write protected?)\n");
			if(file!=NULL)
				fclose(file);
		}
	}
	dui->DrawText(430, 35, dui->palette[6], "remv.");
	dui->DrawText(471, 35, dui->palette[6], "new");
	if(dui->DoButtonRed(432, 44, "btn_rminstr", "remove current instrument (only allowed if no parts use it)") && gearstacks[fcurstack].instrument!=NULL)
	{
		int numstacks=0;
		for(int i=0;i<maxstacks;i++)
		{
			if(gearstacks[i].instrument!=NULL)
				numstacks++;
		}
		if(numstacks>1) // can't remove last instrument
		{
			int numused=0;
			int lastused=0;
			for(int i=0;i<maxparts;i++)
				if(mungoparts[i]!=NULL)
					if(mungoparts[i]->gearstack==&gearstacks[fcurstack])
					{
						numused++;
						lastused=i;
					}
			if(numused==0)
			{
				audiostream->RemoveGearStack(&gearstacks[fcurstack]);
				for(int i=0;i<gearstacks[fcurstack].num_effects;i++)
					delete gearstacks[fcurstack].effects[i];
				delete gearstacks[fcurstack].instrument;
				gearstacks[fcurstack].instrument=NULL;
				// find nearest existing instrument
				while(gearstacks[fcurstack].instrument==NULL)
				{
					fcurstack--;
					if(fcurstack<0)
					{
						fcurstack=0;
						break;
					}
				}
				while(gearstacks[fcurstack].instrument==NULL)
				{
					fcurstack++;
				}
			}
			else
			{
				char string[256];
				if(numused>1)
					sprintf(string, "Can't remove instrument, it's in use by %i parts", numused);
				else
					sprintf(string, "Can't remove instrument, it's in use by 1 part (%s)", mungoparts[lastused]->name);
				//MessageBox(hWndMain, string, "Info", MB_ICONEXCLAMATION);
                                printf("%s\n", string);
			}
		}
		else
			//MessageBox(hWndMain, "Can't remove all instruments, add another one first", "Info", MB_ICONEXCLAMATION);
                        printf("Can't remove all instruments, add another one first\n");
	}
	if(dui->DoButton(471, 44, "btn_adinstr", "add new instrument into current slot"))
		dui->PopupInstrumentSelect();

	// instrument area
	int ngs=0;
//	for(int i=0;i<maxstacks;i++)
	for(int i=0;i<99;i++)
//		if(gearstacks[i].instrument!=NULL)
	{
		ngs=i;
		char string1[4];
		char string2[40];
		sprintf(string1, "%i.", ngs+1);
		int color=6;
		if(gearstacks[i].instrument!=NULL)
			sprintf(string2, "%s", gearstacks[i].instrument->Name());
		else
		{
			sprintf(string2, "--");
			color=5;
		}
		if(ngs==fcurstack)
			color=9;
		int tx=500+ngs/7*100;
		int ty=3+(ngs%7)*8;
		dui->StartFlatWindow(tx, ty, 100, 8, "minwin");
		if(ngs+1<10)
			dui->DrawText(7, 0, dui->palette[color], string1);
		else
			dui->DrawText(0, 0, dui->palette[color], string1);
		dui->DrawText(18, 0, dui->palette[color], string2);
		dui->EndWindow();
	}

	if(dui->lmbclick || dui->rmbclick) // select or rename instrument
	{
//		for(int i=0;i<maxstacks;i++)
		for(int i=0;i<99;i++)
		{
//			if(gearstacks[i].instrument==NULL)
//				continue;
			int x=500+i/7*100;
			int y=3+(i%7)*8;
			if(dui->mousex>=x && dui->mousex<x+100 && dui->mousey>=y && dui->mousey<y+8)
			{
				Keyboard_AllUp();
				if(dui->lmbclick)
					fcurstack=i;
				if(dui->rmbclick)
				{
					fcurstack=i;
					if(gearstacks[i].instrument!=NULL)
						dui->EditString(gearstacks[i].instrument->Name());
				}
				if(dui->dlmbclick && gearstacks[i].instrument!=NULL)
				{
					fcurstack=i;
					if(gearstacks[i].winx<-400+16 || gearstacks[i].winx>glkitGetWidth()-16 || gearstacks[i].winy>glkitGetHeight()-16) // never shown before, or outside
					{
						gearstacks[i].winx=glkitGetWidth()-400;
						gearstacks[i].winy=60+rnd(300);
					}
					ToggleStackWindow(i);
				}
			}
		}
	}

	if(dui->mmbclick && dui->MouseInZone(0, 70, glkitGetWidth(), glkitGetHeight()-135)) // add part
	{
		if(gearstacks[fcurstack].instrument==NULL)
			//MessageBox(hWndMain, "Select an instrument first.", "Info", MB_ICONINFORMATION);
                        printf("Select an instrument first.\n");
		else
		{
			int imx=(int)(dui->mousex+thesong->iscrollx)/4*160*16;
			int imy=(int)(dui->mousey-60+thesong->iscrolly);
			
                        int i;
			for(i=0;i<maxparts;i++)
				if(mungoparts[i]==NULL)
					break;
					
			if(dui->ctrl_down && thesong->GetSelectedPart()!=NULL) // create clone of selected part
				thesong->InsertPart(thesong->GetSelectedPart(), imx, imy/10);
			else if(dui->shift_down && thesong->GetSelectedPart()!=NULL) // create copy of selected part
			{
				mungoparts[i]=new Part(thesong->GenGuid());
				mungoparts[i]->CopyOf(thesong->GetSelectedPart());
				mungoparts[i]->winx=50+rnd(500);
				mungoparts[i]->winy=300+rnd(50);
				thesong->InsertPart(mungoparts[i], imx, imy/10);
			}
			else
			{
				mungoparts[i]=new Part(thesong->GenGuid());
				mungoparts[i]->SetGearStack(&gearstacks[fcurstack], fcurstack);
				mungoparts[i]->winx=50+rnd(500);
				mungoparts[i]->winy=300+rnd(50);
				char *sid=thesong->InsertPart(mungoparts[i], imx, imy/10);
	//			thesong->SelectPart(mungoparts[i]);
				thesong->SelectPart(sid);
				mungoparts[i]->Popup();
				current_part=mungoparts[i];
			}
		}
		
		// add -> create clone of selected part, needs work in song structure
	}

	if(input->KeyPressed(SDLK_DELETE))
	{
//		Part *spart=thesong->GetSelectedPart();
		char *sid=thesong->GetSelectedSid();
		if(sid!=NULL)
		{
			Part *spart=thesong->GetSelectedPart();
			if(thesong->RemovePart(sid)) // true means the part should be deleted
			{
				for(int i=0;i<maxparts;i++)
					if(mungoparts[i]==spart)
						mungoparts[i]=NULL;
				delete spart;
			}
		}
	}

	// deselect parts if clicked outside
	Part *ppart=current_part;
	if(dui->lmbclick)
	{
		dui->DeselectWindow();
		current_part=NULL;
	}

	thesong->scrollfollow=scrollfollow;
	thesong->DoInterface(dui);

	dui->EndWindow();

	for(int i=0;i<maxstacks;i++)
		if(gearstacks[i].instrument!=NULL)
			gearstacks[i].instrument->SetGid(i);

	// part windows
	int pactive=-1;
	int ptopmost=-1;
	for(int i=0;i<maxparts;i++)
		if(mungoparts[i]!=NULL)
		{
			if(mungoparts[i]->DoInterface(dui, audiostream))
				pactive=i;
			ptopmost=i;
		}

	if(pactive!=-1 && pactive!=ptopmost) // one window is active, make sure it's topmost (last in list)
	{
		Part *temp=mungoparts[pactive];
		int prevpart=pactive;
		for(int i=pactive+1;i<=ptopmost;i++)
			if(mungoparts[i]!=NULL)
			{
				mungoparts[prevpart]=mungoparts[i];
				prevpart=i;
			}
		mungoparts[ptopmost]=temp;
	}
//	if(pactive!=-1)
//		thesong->DeselectPart();

	if(current_part!=NULL && current_part->hidden)
		current_part=NULL;

	// instrument windows
	int activestack=-1;
	bool hideswin=false;
	for(int gsi=0;gsi<stacklistnum;gsi++)
	{
		int i=stacklist[gsi];
		if(gearstacks[i].instrument!=NULL)
		{
			GearStack *gs=&gearstacks[i];
			if(!gs->winshow)
				continue;
			gs->instrument->SetGid(i);
			gs->instrument->PrepGUI();
			char windowtitle[128];
			sprintf(windowtitle, "%i: %s", i+1, gs->instrument->Name());
			dui->NameSpace(gs);
			if(glkit_resized && gs->snappedright) // maintain right-snap upon window resize
				gs->winx=glkitGetWidth()-400;
			int twx=gs->winx;
//			bool winactive=dui->StartWindow(gs->winx, gs->winy, booger, 400, 49+gs->num_effects*29, windowtitle, "iwin");
			bool winactive=dui->StartWindowSnapX(gs->winx, gs->winy, twx, 400, 49+gs->num_effects*29, windowtitle, "iwin");
			if(twx==glkitGetWidth()-400)
				gs->snappedright=true;
			else
				gs->snappedright=false;
			int twy=gs->winy;
/*			int twx=gs->winx;
			int twy=gs->winy;
			if(abs(twx-(glkitGetWidth()-400))<16) // snap to right window edge
				twx=glkitGetWidth()-400;
			if(abs(twx)<16) // snap to right window edge
				twx=0;*/
			if(winactive)
			{
//				if(i!=fcurstack && !(current_part!=NULL && current_part->gearstack==gs))
				if(ppart!=NULL && ppart->wantrelease && ppart->gearstack==gs)
					ppart->wantrelease=false;
				else if(i!=fcurstack && gearstacks[fcurstack].instrument!=NULL)
					gearstacks[fcurstack].instrument->ReleaseAll();
				fcurstack=i;
				current_part=NULL;
				activestack=gsi;
				if(dui->dlmbclick && dui->MouseInZone(twx, twy, 400, 12)) // hide
					hideswin=true;
				if(dui->rmbclick && dui->MouseInZone(twx, twy, 400, 12)) // rename
					dui->EditString(gs->instrument->Name());
			}
			else if(fcurstack==i && current_part!=NULL)
			{
				if(current_part->gearstack!=gs && gearstacks[fcurstack].instrument!=NULL)
					gearstacks[fcurstack].instrument->ReleaseAll();
			}
			if(gs->winpop)
			{
				dui->RequestWindowFocus("iwin");
				gs->winpop=false;
			}
			dui->DrawLED(389, 1, 0, gs->instrument->IsTriggered());
			gs->instrument->DefaultWindow(dui, twx, twy+11);
			for(int j=0;j<gs->num_effects;j++)
			{
				gs->effects[j]->PrepGUI();
				gs->effects[j]->DefaultWindow(dui, twx, twy+11+49+j*29);
			}
			dui->EndWindow();
		}
	}
	if(activestack!=-1 && activestack!=stacklistnum-1) // active window not topmost, make it so
	{
		int temp=stacklist[activestack];
		for(int i=activestack;i<stacklistnum-1;i++)
			stacklist[i]=stacklist[i+1];
		stacklist[stacklistnum-1]=temp;
		activestack=stacklistnum-1;
	}
	if(hideswin)
		ToggleStackWindow(stacklist[activestack]);
	for(int i=0;i<maxparts;i++)
		if(mungoparts[i]!=NULL)
		{
			if(mungoparts[i]->wantrelease && mungoparts[i]->gearstack!=&gearstacks[fcurstack])
				mungoparts[i]->gearstack->instrument->ReleaseAll();
			mungoparts[i]->wantrelease=false;
		}
/*	if(ppart!=NULL && ppart->wantrelease)
	{
		if(ppart->gearstack!=&gearstacks[fcurstack])
			ppart->gearstack->instrument->ReleaseAll();
		ppart->wantrelease=false;
	}
*/
	if(input->KeyPressed(SDLK_ESCAPE))
	{
		if(!shortkeydown)
		{
/*			if(current_part!=NULL)
				current_part->gearstack->instrument->StopChannels();
			else if(gearstacks[fcurstack].instrument!=NULL)
				gearstacks[fcurstack].instrument->StopChannels();*/
			for(int i=0;i<maxstacks;i++)
				if(gearstacks[i].instrument!=NULL)
					gearstacks[i].instrument->StopChannels();
		}
		shortkey=true;
	}

	int rcresult=-1;
	float *value=dui->DoOverlay(rcresult);
	if(rcresult!=-1)
	{
		LogPrint("main: rcmenu selected %i (%.8X)", rcresult, value);
		if(rcresult==0)
			thesong->KnobRecToggle(value);
		if(rcresult==1)
			thesong->KnobRecClear(value);
		if(rcresult==2)
			thesong->KnobRecDefault(value);

		if(rcresult>=10 && rcresult<15) // add instrument
		{
/*			for(int i=0;i<maxstacks;i++)
				if(gearstacks[i].instrument==NULL)
				{*/
			bool ok=true;
			if(gearstacks[fcurstack].instrument!=NULL)
			{
				int choice=1;//MessageBox(hWndMain, "Adding a new instrument here will replace the current one, are you sure?", "Caution", MB_ICONEXCLAMATION|MB_OKCANCEL);
				if(choice!=1)//IDOK)
					ok=false;
				else
				{
					audiostream->RemoveGearStack(&gearstacks[fcurstack]);
					for(int i=0;i<gearstacks[fcurstack].num_effects;i++)
						delete gearstacks[fcurstack].effects[i];
					delete gearstacks[fcurstack].instrument;
					gearstacks[fcurstack].instrument=NULL;
				}
			}
			int i=fcurstack;
//			if(gearstacks[i].instrument==NULL)
			if(ok)
			{
				switch(rcresult-10)
				{
				case 0: // xnes
					gearstacks[i].instrument=new gin_xnes();
					break;
				case 1: // chip
					gearstacks[i].instrument=new gin_chip();
					break;
				case 2: // vsampler
					gearstacks[i].instrument=new gin_vsampler();
					break;
				case 3: // swave
					gearstacks[i].instrument=new gin_swave();
					break;
				case 4: // protobass
					gearstacks[i].instrument=new gin_protobass();
					break;
				}
				gearstacks[i].num_effects=1;
				gearstacks[i].effects[0]=new gef_gapan();
				audiostream->AddGearStack(&gearstacks[i]);
			}
//			break;
//				}
		}
	}

	if(shortkey)
		shortkeydown=true;
	else
		shortkeydown=false;
}

int KnobRecState(float *value)
{
	return thesong->KnobRecState(value);
}

bool KnobAtMemPos(float *pos)
{
	return thesong->KnobAtMemPos(pos);
}

void KnobToMemPos(float *oldpos, float *newpos)
{
	thesong->KnobToMemPos(oldpos, newpos);
}

GearStack *GetCurrentGearstack()
{
	return &gearstacks[fcurstack];
}

/*HWND GetHwnd()
{
	return hWndMain;
}*/

bool note_playing=false;

bool glkCalcFrame()
{
	input->Update();

/*	if(input->KeyPressed(SDLK_SPACE))
		glkit_render=false;
	else
		glkit_render=true;
*/
	if(glkit_close)
		return false;

	bool kbkbhit=false;
	if(!(input->KeyPressed(SDLK_LCTRL) || input->KeyPressed(SDLK_RCTRL)))
		for(int i=0;i<33;i++)
		{
			if(input->KeyPressed(kbkbk[i]))
			{
				kbkbsomething=true;
				kbkbhit=true;
				if(!kbkbkeydown[i]) // new keystroke
				{
					kbkbkeydown[i]=true;
					Keyboard_KeyDown(kbkbm[i]+kbkboctave*12);
				}
			}
			else
			{
				if(kbkbkeydown[i])
					Keyboard_KeyUp(kbkbm[i]+kbkboctave*12);
				kbkbkeydown[i]=false;
			}
		}
	if(!kbkbhit)
	{
		if(kbkbsomething)
		{
			kbkbsomething=false;
		}
		for(int i=0;i<33;i++)
			kbkbkeydown[i]=false;
	}

//	Sleep(10);

	return true;
}

void ProgressBar()
{
	if(progressdone)
		return;
	int ctime=SDL_GetTicks();//timeGetTime();
	if(progress==0)
		progressmark=ctime;
	progress++;
	if(ctime-progressmark>50) // render at most every 50 ms to avoid slowing things down too much
	{
		progressmark=ctime;
		glkitInternalRender();
	}
}

void glkPreInit()
{
	FILE *cfg=fopen("config.txt", "r");
	if(cfg!=NULL)
	{
		char junk[32];
		fscanf(cfg, "%s %i %i %i %i %i", junk, &glkit_winx, &glkit_winy, &glkit_width, &glkit_height, &glkit_winmax);
		fscanf(cfg, "%s %i", junk, &log_on);
		fscanf(cfg, "%s %i", junk, &midi_id);
		fclose(cfg);
	}
	LogPrint("init audiostream");
	audiostream=new AudioStream();
}

void Reset()
{
	LogPrint("init instruments");

	stacklistnum=0;

	for(int i=0;i<maxstacks;i++)
	{
		gearstacks[i].instrument=NULL;
		gearstacks[i].num_effects=0;
		gearstacks[i].winshow=false;
		gearstacks[i].winpop=false;
		gearstacks[i].snappedright=false;
		gearstacks[i].winx=-999;
		gearstacks[i].winy=0;
	}

	gearstacks[0].instrument=new gin_xnes();
	gearstacks[0].num_effects=1;
	gearstacks[0].effects[0]=new gef_gapan();

	gearstacks[0].winx=glkitGetWidth()-400;
	gearstacks[0].winy=95;
	ToggleStackWindow(0);
	gearstacks[0].winpop=false;
	
	for(int i=0;i<maxparts;i++)
		mungoparts[i]=NULL;

	current_part=NULL;
	fcurstack=0;

	thesong=new Song();
	thesong->scrollfollow=scrollfollow;

	// create a part
	int pi=0;	
	mungoparts[pi]=new Part(thesong->GenGuid());
	mungoparts[pi]->SetGearStack(&gearstacks[fcurstack], fcurstack);
	mungoparts[pi]->winx=55;
	mungoparts[pi]->winy=145;
	char *sid=thesong->InsertPart(mungoparts[pi], 0, 4);
	thesong->SelectPart(sid);
	mungoparts[pi]->Popup();
	current_part=mungoparts[pi];
	
	curfilename[0]='\0';

	ftempo=0.5f;
	master_volume=0.5f;

	LogPrint("main: add gearstacks");

	for(int i=0;i<maxstacks;i++)
		if(gearstacks[i].instrument!=NULL)
			audiostream->AddGearStack(&gearstacks[i]);

	LogPrint("main: set song");

	audiostream->SetSong(thesong);
}

void glkInit()
{
	int gtime=SDL_GetTicks();//timeGetTime();

        // TODO: Replace with actually getting the path of the executable
	//GetModuleFileName(NULL, defpath, 512);
        defpath[0] = '.';
        defpath[1] = 0;
	int si=strlen(defpath)-1;
	while(si>0 && defpath[si]!='/')
	{
		si--;
	}
	defpath[si+1]='\0';
	char logpath[512];
	strcpy(logpath, defpath);
	strcat(logpath, "log.txt");
                 
	if(log_on)
		LogStart(logpath);
	else
		LogDisable();
	LogPrint("main: default directory==\"%s\"", defpath);
	LogPrint("--- init start");
	ProgressBar();
//	LogPrint("time=%i", timeGetTime()-gtime);

	LogPrint("init input");
	input=new DPInput();//hWndMain, hInstanceMain);

	ProgressBar();
//	LogPrint("time=%i", timeGetTime()-gtime);
	LogPrint("init midi");
        // TODO: Fix midi code to work on all platforms
	/*if(!midi_init(midi_id))
		LogPrint("ERROR: midi failed");*/

	ProgressBar();
//	LogPrint("time=%i", timeGetTime()-gtime);
	srand(time(NULL));

	// keyboard keyboard
	kbkbk[0]=SDLK_z; // C
	kbkbm[0]=0;
	kbkbk[1]=SDLK_s; // C#
	kbkbm[1]=1;
	kbkbk[2]=SDLK_x; // D
	kbkbm[2]=2;
	kbkbk[3]=SDLK_d; // D#
	kbkbm[3]=3;
	kbkbk[4]=SDLK_c; // E
	kbkbm[4]=4;
	kbkbk[5]=SDLK_v; // F
	kbkbm[5]=5;
	kbkbk[6]=SDLK_g; // F#
	kbkbm[6]=6;
	kbkbk[7]=SDLK_b; // G
	kbkbm[7]=7;
	kbkbk[8]=SDLK_h; // G#
	kbkbm[8]=8;
	kbkbk[9]=SDLK_n; // A
	kbkbm[9]=9;
	kbkbk[10]=SDLK_j; // A#
	kbkbm[10]=10;
	kbkbk[11]=SDLK_m; // B
	kbkbm[11]=11;
	kbkbk[12]=SDLK_COMMA; // C
	kbkbm[12]=12;
	kbkbk[13]=SDLK_l; // C#
	kbkbm[13]=13;
	kbkbk[14]=SDLK_PERIOD; // D
	kbkbm[14]=14;
	kbkbk[15]=SDLK_SLASH; // E
	kbkbm[15]=16;

	kbkbk[16]=SDLK_q; // C
	kbkbm[16]=12;
	kbkbk[17]=SDLK_2; // C#
	kbkbm[17]=13;
	kbkbk[18]=SDLK_w; // D
	kbkbm[18]=14;
	kbkbk[19]=SDLK_3; // D#
	kbkbm[19]=15;
	kbkbk[20]=SDLK_e; // E
	kbkbm[20]=16;
	kbkbk[21]=SDLK_r; // F
	kbkbm[21]=17;
	kbkbk[22]=SDLK_5; // F#
	kbkbm[22]=18;
	kbkbk[23]=SDLK_t; // G
	kbkbm[23]=19;
	kbkbk[24]=SDLK_6; // G#
	kbkbm[24]=20;
	kbkbk[25]=SDLK_y; // A
	kbkbm[25]=21;
	kbkbk[26]=SDLK_7; // A#
	kbkbm[26]=22;
	kbkbk[27]=SDLK_u; // B
	kbkbm[27]=23;
	kbkbk[28]=SDLK_i; // C
	kbkbm[28]=24;
	kbkbk[29]=SDLK_9; // C#
	kbkbm[29]=25;
	kbkbk[30]=SDLK_o; // D
	kbkbm[30]=26;
	kbkbk[31]=SDLK_0; // D#
	kbkbm[31]=27;
	kbkbk[32]=SDLK_p; // E
	kbkbm[32]=28;
	for(int i=0;i<33;i++)
		kbkbkeydown[i]=false;
	kbkbsomething=false;
	kbkboctave=2;
	
	midioctave=1;

	InitGlobals();
	tempo=44;

        // Moving this to PreInit
	//LogPrint("init audiostream");
	//audiostream=new AudioStream();

	Reset();
	
	ProgressBar();
	
	LogPrint("init dui");
	dui=new DUI();
	dui->glkmouse=&glkmouse;

	LogPrint("main: start stream");
	audiostream->StartStream(1024, &pa_callback);

	ProgressBar();
//	LogPrint("time=%i", timeGetTime()-gtime);

	// load config
	FILE *cfg=fopen("config.txt", "r");
	if(cfg!=NULL)
	{
		char junk[32];
		fscanf(cfg, "%s %i %i %i %i %i", junk, junk, junk, junk, junk, junk);
		fscanf(cfg, "%s %s", junk, junk);
		fscanf(cfg, "%s %s", junk, junk);
		fscanf(cfg, "%s %i", junk, &dui->tooltips_on);
		fscanf(cfg, "%s %i", junk, &scrollfollow);
		fscanf(cfg, "%s %f", junk, &audiostream->metronome_vol);
//		fscanf(cfg, "%s %f", junk, &master_volume);
		fscanf(cfg, "%s %s %i", junk, junk, &kbkboctave);
		fscanf(cfg, "%s %s %i", junk, junk, &midioctave);
		fclose(cfg);
	}

	ProgressBar();

	LogPrint("--- init done");
	LogPrint("total init time=%i (excluding window creation and opengl init)", SDL_GetTicks()-gtime);//timeGetTime()-gtime);

	progressdone=true;
	LogPrint("progress=%i", progress);

//	audiostream->StartFileOutput("output.raw");

//	LogDisable();
}

void glkFree()
{
//	char spath[256];
//	int bla;
//	AssocQueryString(ASSOCF_OPEN_BYEXENAME, ASSOCSTR_EXECUTABLE, "musagi.exe", NULL, spath, &bla);
//	FindExecutable("musagi.exe", "", spath);
//	LogPrint("main: exe directory==\"%s\"", spath);

	char cfgpath[512];
	strcpy(cfgpath, defpath);
	strcat(cfgpath, "config.txt");
	LogPrint("main: writing cfg to \"%s\"", cfgpath);
	FILE *cfg=fopen(cfgpath, "w");
	if(cfg!=NULL)
	{
		fprintf(cfg, "window: %i %i %i %i %i\n", glkit_winx, glkit_winy, glkit_width, glkit_height, glkit_winmax);
		fprintf(cfg, "log: %i\n", log_on);
		fprintf(cfg, "mididevice: %i\n", midi_id);
		fprintf(cfg, "tooltips: %i\n", dui->tooltips_on);
		fprintf(cfg, "scrollfollow: %i\n", scrollfollow);
		fprintf(cfg, "metronome: %.2f\n", audiostream->metronome_vol);
//		fprintf(cfg, "master: %.2f\n", master_volume);
		fprintf(cfg, "keyboard octave: %i\n", kbkboctave);
		fprintf(cfg, "midi octave: %i\n", midioctave);
		fclose(cfg);
	}

	audiostream->StopFileOutput();
	delete audiostream;

	delete input;
	delete dui;

	for(int i=0;i<maxstacks;i++)
		if(gearstacks[i].instrument!=NULL)
		{
			delete gearstacks[i].instrument;
			for(int j=0;j<gearstacks[i].num_effects;j++)
				if(gearstacks[i].effects[j]!=NULL)
					delete gearstacks[i].effects[j];
		}
	for(int i=0;i<maxparts;i++)
		if(mungoparts[i]!=NULL)
			delete mungoparts[i];

	delete thesong;

	//midi_exit();
	LogPrint("cleanup done");
}

