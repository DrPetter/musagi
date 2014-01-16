// buildlog says about 230 hours spent in development from 2006-04-03 to 2008-01-19

bool abort_render=false;

#include "glkit.h"

#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

bool midithread_getlost=false;

#include "platform.h"

bool kbkbkeydown[50];
bool kbkbsomething;
int kbkboctave;

int midioctave;

void Keyboard_KeyDown(int note);
void Keyboard_KeyUp(int note);

int debug_midimsg_t=0;
int debug_midimsg_count=0;
DWORD debug_midimsg_val=0;
void Debug_MidiMsg(DWORD msg)
{
	debug_midimsg_t=60;
	debug_midimsg_val=msg;
//	LogPrint("MIDIMSG - %08X", msg);
}
void Debug_MidiInc()
{
	debug_midimsg_count++;
}

//#include "gear_instrument.h"
class gear_instrument;

gear_instrument* active_children[32];
void ChildWindowActive(gear_instrument* instr, bool value)
{
	if(value==true) // activated, add to list
	{
		for(int i=0;i<32;i++)
			if(active_children[i]==NULL || active_children[i]==instr)
			{
				active_children[i]=instr;
				return;
			}
	}
	else // deactivated, remove from list
	{
		for(int i=0;i<32;i++)
			if(active_children[i]==instr)
				active_children[i]=NULL;
	}
}
gear_instrument* InChildWindow()
{
	for(int i=0;i<32;i++)
		if(active_children[i]!=NULL)
			return active_children[i];
	return NULL;
}

#include "musagi.h"
#include "musagig.h"

#include "dui.h"

#include "audiostream.h"
#include "pa_callback.h"

AudioStream *audiostream;

VstTimeInfo* GetVstTimeInfo()
{
	return audiostream->GetVstTimeInfo();
}

#include "gin_protobass.h"
#include "gin_xnes.h"
#include "gin_chip.h"
#include "gin_swave.h"
#include "gin_vsmp.h"
#include "gin_midinst.h"
#include "gin_midperc.h"
#include "gin_vsti.h"
#include "gin_operator.h"
#include "gin_wavein.h"

#include "gef_gapan.h"

#include "part.h"

#include "song.h"

DUI *dui;

DUI* GetDUI()
{
	return dui;
}

GearStack gearstacks[128];
int maxstacks=128;

int MapFromGearStack(GearStack* ptr)
{
	for(int i=0;i<maxstacks;i++)
		if(&gearstacks[i]==ptr)
			return i;
	return -1;
}

GearStack* MapToGearStack(int i)
{
	if(i==-1)
		return NULL;
	return &gearstacks[i];
}

Part *mungoparts[2048];
int maxparts=2048;
// TODO: could use max-used value to speed up iteration, but must keep it updated
//int maxpartused=0;

void** GetAllParts(int& arraysize)
{
	arraysize=maxparts;
	return (void**)mungoparts;
}

Song *thesong;

bool diskrec=false;

int log_on=0;
int midi_id=0;
char portaudio_latency[16];

int scrollfollow=1;
char defpath[512];

char curfilename[512];
int autosave=0;

int errorcount;

float ftempo=0.5f;
float master_volume=0.5f;
float speaker_volume=0.5f;

bool shortkeydown=false;
bool fpressed=false;
int numkeysdown=0;
int fcurstack=0;
int stacklist[128];
int stacklistnum=0;

float* joystick_binding[2];

unsigned int GetTick(int mode)
{
	return audiostream->GetTick(mode);
}

float UpdateTempo()
{
	int fractempo=1945+(int)(pow(1.0f-ftempo, 2.0f)*6000);
	float bpm=44100.0f/(32*320*fractempo/1600)*60;
	bpm=(float)((int)bpm);
	float rounded_tempo=(1.0f/(bpm/60))*44100.0f/32*1600/320;
	tempo=(int)rounded_tempo;
	return bpm;
}


void BindAxisToKnob(int axis, float* value)
{
	joystick_binding[axis]=value;
}

void UpdateAxisKnobs()
{
	for(int i=0;i<2;i++)
		if(joystick_binding[i]!=NULL)
		{
			float v=input->JoyAxis(i)*0.6f+0.45f;
			if(v<0.0f) v=0.0f;
			if(v>1.0f) v=1.0f;
			*joystick_binding[i]=1.0f-v;
		}
}

void Reset();

void Keyboard_KeyDown(int note)
{
	if(dui->editstring || note<0 || note>=120)
		return;
//	if(!(input->KeyPressed(DIK_LCONTROL) || input->KeyPressed(DIK_RCONTROL)))
	if(!dui->ctrl_down)
	{
		gear_instrument* vst_plugin=InChildWindow();
		if(current_part!=NULL)
		{
			current_part->gearstack->instrument->Trigger(note, 1.0f);
			if(current_part->prec)
				current_part->RecTrigger(note, 1.0f, current_part->gearstack->instrument->Atomic());
		}
		else if(vst_plugin!=NULL)
			vst_plugin->Trigger(note, 1.0f);
		else if(gearstacks[fcurstack].instrument!=NULL)
			gearstacks[fcurstack].instrument->Trigger(note, 1.0f);
	}
	SetKeyDown(note, true);

	numkeysdown++;
	
//	midi_noteon(0, note, 127);
}

void Keyboard_KeyUp(int note)
{
	if(dui->editstring || note<0 || note>=120)
		return;
	gear_instrument* vst_plugin=InChildWindow();
	if(current_part!=NULL && !current_part->gearstack->instrument->Atomic())
	{
		current_part->gearstack->instrument->Release(note);
		if(current_part->prec)
			current_part->RecRelease(note);
	}
	else if(vst_plugin!=NULL)
		vst_plugin->Release(note);
	else if(gearstacks[fcurstack].instrument!=NULL)
		gearstacks[fcurstack].instrument->Release(note);
	SetKeyDown(note, false);

	numkeysdown--;
	if(numkeysdown<0) numkeysdown=0;
	
//	midi_noteoff(0, note, 127);
}

void Keyboard_AllUp()
{
	if(numkeysdown>0)
	{
		if(current_part!=NULL)
			current_part->gearstack->instrument->ReleaseAll();
		else if(gearstacks[fcurstack].instrument!=NULL)
			gearstacks[fcurstack].instrument->ReleaseAll();
	}
	for(int i=0;i<120;i++)
		SetKeyDown(i, false);
	numkeysdown=0;
}

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
		for(int si=0;si<stacklistnum;si++)
			if(stacklist[si]==gsi)
				break;
		for(int i=si;i<stacklistnum-1;i++)
			stacklist[i]=stacklist[i+1];
		stacklistnum--;
	}
}



void midiplay_tick(int ticksize)
{
	int subsize=ticksize;
	AudioStream* as=audiostream;

	if(as->song!=NULL)
		as->song->PlayStep(subsize*1600/GetTempo(), as);

	for(int i=0;i<as->numparts;i++)
	{
		Part *part=as->parts[i].part;
		gear_instrument *instr=part->gearstack->instrument;
		if(!instr->Midi()) // only handle parts with midi instruments
			continue;
		part->playtime+=subsize*1600/GetTempo();
		int curtime=part->playtime;
		Trigger *trig=part->GetTrigger(curtime);
		while(trig!=NULL) // consume all due triggers
		{				
			instr->Trigger(trig->note, trig->volume, trig->duration, trig->slidenote, trig->slidelength);

			if(part->Ended())
				as->parts[i].expired=true;
			
			trig=part->GetTrigger(curtime);
		}
	}

	for(int gi=0;gi<as->num_gearstacks;gi++)
	{
		GearStack *gs=as->gearstacks[gi];
		if(gs==NULL || gs->instrument==NULL) // this stack has recently been removed, ignore it
			continue;
		if(!gs->instrument->Midi()) // only handle midi instruments
			continue;
		gs->instrument->RenderBuffer(subsize); // RenderBuffer takes care of releasing notes with known duration
	}
}

void update_midimode() // CRASH: this could crash if gearstack pointers aren't set to NULL when removed... but they should be?
{
	// see if there are any midi instruments present
	bool midi_in_the_house=false;
	for(int i=0;i<maxstacks;i++)
	{
		if(gearstacks[i].instrument!=NULL)
			if(gearstacks[i].instrument->Midi())
			{
				midi_in_the_house=true;
				break;
			}
	}
	// activate midi mode if there were, meaning only midi sounds will be heard
	if(midi_in_the_house)
		audiostream->midi_mode=true;
	else
		audiostream->midi_mode=false;
}

void midiplay_routine()
{
	DWORD ms_start=timeGetTime();
	DWORD ms_cur=0;
	int cursample=0;
	int moderecheck=0;
	
	for(;;)
	{
		int nextsample=0;
		if(!midi_exporting)
		{
			platform_Sleep(0);
			DWORD ms=timeGetTime()-ms_start;

			if(ms==ms_cur) continue; // no measurable time has passed, sleep again
			if(midithread_getlost) // main program is closing
			{
				midithread_getlost=false; // acknowledge that we got the message
				LogPrint("midi thread closing");
				return;
			}
			if(!audiostream->midi_mode)
			{
				platform_Sleep(20); // make sure not to waste too much cpu time when there are no midi instruments present
				update_midimode();
				continue;
			}
			ms_cur=ms;
			nextsample=(int)((float)ms_cur*44.1f);
		}
		while(nextsample>=cursample+128 || midi_exporting)
		{
			midiplay_tick(128);
			if(midi_exporting)
			{
				midi_stime+=128;
				ms_start=timeGetTime();
				ms_cur=0;
				cursample=0;
				nextsample=0;
			}
			cursample+=128;
			// check every once in a while to see if we've left midi mode
			if(!midi_exporting)
			{
				moderecheck++;
				if(moderecheck>20)
				{
					moderecheck=0;
					update_midimode();
				}
			}
		}
	}
}



#include "main_fileops.h"


//----------------------------------------

int progress=0;
int progressmax=7;
int progressmark;
bool progressdone=false;

bool firstrender=true;

void glkRenderFrame(bool disabled)
{
	if(firstrender) LogPrint("render: begin");

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

	if(disabled) // if this is true here, that means last render didn't end properly, so we're probably suspended showing a popup - ignore keyboard input for this render pass
		input->Disable();
	disabled=true;

	dui->Update(input);

	UpdateAxisKnobs();

	dui->NameSpace((void*)1);
	dui->StartFlatWindow(0, 0, glkitGetWidth(), glkitGetHeight(), "mainw");

	dui->DrawTexturedBar(0, 0, 422, 60, dui->palette[10], dui->brushed_metal, &dui->vglare);
	dui->DrawBar(422, 0, 1, 60, dui->palette[6]);
	dui->DrawTexturedBar(423, 0, glkitGetWidth()-423, 60, dui->palette[16], dui->brushed_metal, NULL);
	dui->DrawTexturedBar(0, glkitGetHeight()-58, glkitGetWidth(), 58, dui->palette[16], dui->brushed_metal, NULL);
	dui->DrawSpriteAlpha(dui->logo, glkitGetWidth()-250, -2, 0.15f);

	if(audiostream->midi_mode)
	{
		dui->DrawText(120, 22, dui->palette[6], "MIDI Output");

		// TODO: make the code below not crash...

		dui->DrawText(124, 37, dui->palette[6], "export to .mid");
		if(dui->DoButton(100, 36, "songb_xmid", "export song to .mid file (can't import, so remember to save the song in .smu format as well)"))
		{
			char filename[256];
			bool ok=false;
			ok=dui->SelectFileSave(filename, 4);
			if(ok)
			{
				if(!ExportMID(filename))
//					MessageBox((DWORD)hWndMain, "Unable to export song (write protected?)", "Error", MB_ICONEXCLAMATION);
					platform_MessageBox((DWORD)hWndMain, "Unable to export song (write protected?)", "Error", MESSAGEBOX_OK);
			}
		}
	}
	else
	{
		// peak/level indicators
		int level=(int)(audiostream->peak_left*10);
		for(int i=0;i<10;i++)
		{
			if(i==9 && audiostream->clip_left>0)
				dui->DrawLED(95+i*7, 14, 0, true);
			else
				dui->DrawLED(95+i*7, 14, 0, i<level);
		}
		level=(int)(audiostream->peak_right*10);
		for(int i=0;i<10;i++)
		{
			if(i==9 && audiostream->clip_right>0)
				dui->DrawLED(95+i*7, 25, 0, true);
			else
				dui->DrawLED(95+i*7, 25, 0, i<level);
		}
	
		dui->DrawText(168, 15, dui->palette[6], "left");
		dui->DrawText(168, 26, dui->palette[6], "right");

		dui->DrawBar(100, 48, 80, 1, dui->palette[7]);
	
		dui->DrawText(121-10, 39, dui->palette[6], "master");
		dui->DoKnob(95, 40, master_volume, -1, "knob_mvol", "master volume (adjust to avoid clipping)");
		audiostream->master_volume=pow(master_volume, 2.0f)*0.5f;

		dui->DrawText(144-13, 49, dui->palette[6], "speaker");
		dui->DoKnob(176, 40, speaker_volume, -1, "knob_spk", "speaker volume (adjust for pleasant listening)");
		audiostream->speaker_volume=pow(speaker_volume, 2.0f);
	}

	dui->DrawText(95, 3, dui->palette[5], "dissonance:");
	dui->DrawLED(95+70, 2, 1, EarLevel()>0);
	dui->DrawLED(95+77, 2, 1, EarLevel()>2);
	dui->DrawLED(95+84, 2, 1, EarLevel()>4);

	int cpu_tempo_xpos=15;

	// cpu indicator
	int cpuu=(int)((audiostream->cpu_usage/100)*15);
	dui->DrawBar(cpu_tempo_xpos+190, 9, 8, 46, dui->palette[6]);
	for(int i=0;i<15;i++)
	{
		if(i<=cpuu)
		{
			int color=13;
			if(i>=13)
				color=11;
			dui->DrawBar(cpu_tempo_xpos+191, 10+42-i*3, 6, 2, dui->palette[color]);
			dui->DrawBar(cpu_tempo_xpos+193, 10+42-i*3, 2, 2, dui->palette[9]);
		}
		else
		{
			dui->DrawBar(cpu_tempo_xpos+191, 10+42-i*3, 1, 2, dui->palette[4]);
			dui->DrawBar(cpu_tempo_xpos+196, 10+42-i*3, 1, 2, dui->palette[4]);
		}
	}
	dui->DrawText(cpu_tempo_xpos+200, 9, dui->palette[6], "cpu %2.1f %%", audiostream->cpu_usage);
	dui->DrawBar(cpu_tempo_xpos+197, 20, 61, 1, dui->palette[6]);
	dui->DrawBar(cpu_tempo_xpos+197+61, 13, 1, 8, dui->palette[6]);
	dui->DrawBar(cpu_tempo_xpos+197+61, 13, 422-(cpu_tempo_xpos+197+61), 1, dui->palette[6]);

	// tempo knob
	dui->DrawText(cpu_tempo_xpos+207, 23, dui->palette[6], "tempo:");
	dui->DoKnob(cpu_tempo_xpos+215, 40, ftempo, -1, "knob_tempo", "playback speed");
	float bpm=UpdateTempo();
	int tx=0;
	if(bpm<100) tx=7;
	dui->DrawText(cpu_tempo_xpos+201+tx, 31, dui->palette[6], "%2i bpm", (int)bpm);

	// play button	
	dui->DrawText(38, 9, dui->palette[6], "play");
	dui->DrawLED(5, 8, 0, thesong->playing);
	bool quickplay=false;
	if(input->KeyPressed(DIK_F5))
	{
		if(!shortkeydown)
			quickplay=true;
		shortkey=true;
	}
	if(dui->DoButton(15, 8, "songb_play", "(F5) play song from selected position") || quickplay)
	{
		thesong->PlayButton();

		EarClear();
	}

	// loop button
	dui->DrawText(38, 21, dui->palette[6], "loop");
	dui->DrawLED(5, 20, 2, thesong->loop);
	if(dui->DoButton(15, 20, "songb_loop", "toggle song loop (set using shift+leftclick)"))
		thesong->loop=!thesong->loop;

	// songrec button
	static bool suspend_loop=false;
	dui->DrawText(38, 33, dui->palette[6], "rec song");
	dui->DrawLED(5, 32, 1, audiostream->record_song);
	if(dui->DoButton(15, 32, "btn_srec", "record song to .wav file (automatic play/stop, loop off by default)"))
	{
		char filename[256];
		if(!audiostream->file_output)
		{
			if(dui->SelectFileSave(filename, 3))
			{
				diskrec=true;
				audiostream->StartFileOutput(filename, true);
			}

			thesong->PlayButton();
			if(!thesong->playing)
				thesong->PlayButton();
			if(thesong->loop)
				suspend_loop=true;
			thesong->loop=false;
		}
	}

	if(!audiostream->record_song && suspend_loop)
	{
		thesong->loop=true;
		suspend_loop=false;
	}

	// diskrec button
	dui->DrawText(38, 45, dui->palette[6], "disk rec");
	dui->DrawLED(5, 44, 0, audiostream->file_output);
	if(dui->DoButton(15, 44, "btn_drec", "record audio to .wav file"))
	{
		char filename[256];
		if(!audiostream->file_output)
		{
			if(dui->SelectFileSave(filename, 3))
				audiostream->StartFileOutput(filename, false);
		}
		else
			audiostream->StopFileOutput();
	}

	int kr_load_save_xpos=5;

	// knobrec button
	dui->DrawText(kr_load_save_xpos+318, 19, dui->palette[6], "record knobs");
	dui->DrawLED(kr_load_save_xpos+318-33, 18, 0, thesong->recknobs);
	bool quickkrec=false;
	if(input->KeyPressed(DIK_F6))
	{
		if(!shortkeydown)
			quickkrec=true;
		shortkey=true;
	}
	if(dui->DoButton(kr_load_save_xpos+318-23, 18, "btn_krec", "(F6) record movement of selected knobs during playback (right click on knobs first)") || quickkrec)
	{
		if(!thesong->recknobs && !thesong->playing)
		{
			thesong->recknobs=true;
			thesong->PlayButton();
		}
		else if(thesong->recknobs && thesong->playing)
			thesong->PlayButton();
	}

	// save button
	if(autosave>0)
		autosave--;
	if(autosave==0 && dui->ctrl_down && input->KeyPressed(DIK_S))
		autosave=40;
	if(autosave>20)
		dui->DrawText(kr_load_save_xpos+318, 32, dui->palette[9], "save song");
	else
		dui->DrawText(kr_load_save_xpos+318, 32, dui->palette[6], "save song");
	if(dui->DoButton(kr_load_save_xpos+318-23, 31, "btn_save", "save everything") || autosave==40)
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
//				MessageBox((DWORD)hWndMain, "Unable to save song (write protected?)", "Error", MB_ICONEXCLAMATION);
				platform_MessageBox((DWORD)hWndMain, "Unable to save song (write protected?)", "Error", MESSAGEBOX_OK);
		}
	}

	if(firstrender) LogPrint("render: 4");

	// load button
	dui->DrawText(kr_load_save_xpos+318, 45, dui->palette[6], "load song");
	if(dui->DoButton(kr_load_save_xpos+318-23, 44, "btn_load", "load song - all changes will be lost!") || (dui->ctrl_down && (input->KeyPressed(DIK_L) || input->KeyPressed(DIK_O))))
	{
		LogPrint("main: loadbutton");
		thesong->Stop();
		char filename[256];
		if(dui->SelectFileLoad(filename, 0))
		{
			curfilename[0]='\0';
			if(!LoadSong(filename))
//				MessageBox((DWORD)hWndMain, "Unknown song format (maybe incompatible version)", "Error", MB_ICONEXCLAMATION);
				platform_MessageBox((DWORD)hWndMain, "Unknown song format (maybe incompatible version)", "Error", MESSAGEBOX_OK);
			else
				strcpy(curfilename, filename);
		}
	}

	// --- preferences ---
	dui->DrawBar(glkitGetWidth()-200, glkitGetHeight()-60, 1, 60, dui->palette[6]);
	dui->DrawTexturedBar(glkitGetWidth()-199, glkitGetHeight()-58, 198, 58, dui->palette[10], dui->brushed_metal, NULL);
	
	dui->DrawText(glkitGetWidth()-200+35, glkitGetHeight()-53, dui->palette[6], "metronome");
	dui->DrawLED(glkitGetWidth()-200+5, glkitGetHeight()-54, 0, audiostream->metronome_on);
	dui->DoKnob(glkitGetWidth()-200+15, glkitGetHeight()-58, audiostream->metronome_vol, -1, "knob_metvol", "metronome volume (0=off)");
	audiostream->metronome_on=audiostream->metronome_vol>0.0f;

	dui->DrawText(glkitGetWidth()-200+38, glkitGetHeight()-38, dui->palette[6], "tooltips");
	dui->DrawLED(glkitGetWidth()-200+5, glkitGetHeight()-39, 0, dui->tooltips_on==1);
	if(dui->DoButton(glkitGetWidth()-200+15, glkitGetHeight()-39, "btn_toolt", "toggle tooltips (like this one)"))
		dui->tooltips_on=!dui->tooltips_on;

	dui->DrawText(glkitGetWidth()-90+38, glkitGetHeight()-50, dui->palette[6], "MIDI in");
	if(dui->DoButton(glkitGetWidth()-90+15, glkitGetHeight()-51, "btn_midin", "select MIDI input device"))
		dui->PopupMenu(9, 0);

	dui->DrawText(glkitGetWidth()-90+38, glkitGetHeight()-38, dui->palette[6], "follow");
	dui->DrawLED(glkitGetWidth()-90+5, glkitGetHeight()-39, 0, scrollfollow==1);
	if(dui->DoButton(glkitGetWidth()-90+15, glkitGetHeight()-39, "btn_scrfo", "scroll to follow play position"))
		scrollfollow=!scrollfollow;

	dui->DrawText(glkitGetWidth()-90+38, glkitGetHeight()-26, dui->palette[6], "3/4");
	dui->DrawLED(glkitGetWidth()-90+5, glkitGetHeight()-27, 0, GetBeatLength()==3);
	if(dui->DoButton(glkitGetWidth()-90+15, glkitGetHeight()-27, "btn_btln", "highlight every 3rd bar instead of every 4th"))
	{
		if(GetBeatLength()==3)
			SetBeatLength(4);
		else
			SetBeatLength(3);
	}

	dui->DrawText(glkitGetWidth()-200+5, glkitGetHeight()-20, dui->palette[6], "%i", kbkboctave);

	bool quickoctup=false;
//	if(input->KeyPressed(DIK_PRIOR))
	if(dui->pageup)
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
//	if(input->KeyPressed(DIK_NEXT))
	if(dui->pagedown)
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
		if((dui->lmbclick || dui->rmbclick) && dui->MouseInZone(glkitGetWidth()-450, glkitGetHeight()-53+y, 245, 10))
			dui->EditString(thesong->info_text[i]);
	}
	dui->EndWindow();
	dui->DrawBox(glkitGetWidth()-450, glkitGetHeight()-53, 246, 41, dui->palette[6]);




/*
	dui->DrawText(glkitGetWidth()-730, glkitGetHeight()-52, dui->palette[6], "MIDI msg:");
	dui->DrawText(glkitGetWidth()-740, glkitGetHeight()-42, dui->palette[6], "MIDI count:");
	int dmmcol=9;
	if(debug_midimsg_t<58)
		dmmcol=6;
	if(debug_midimsg_t==0)
		dmmcol=5;
	dui->DrawText(glkitGetWidth()-675, glkitGetHeight()-52, dui->palette[dmmcol], "%08X", debug_midimsg_val);
	if(debug_midimsg_t>0)
		debug_midimsg_t--;
	dui->DrawText(glkitGetWidth()-675, glkitGetHeight()-42, dui->palette[6], "%08X", debug_midimsg_count);
*/



	// new song
	dui->DrawBar(glkitGetWidth()-613, glkitGetHeight()-58, 2, 58, dui->palette[6]);
	dui->DrawBar(glkitGetWidth()-522, glkitGetHeight()-58, 2, 58, dui->palette[6]);
	dui->DrawText(glkitGetWidth()-582, glkitGetHeight()-52, dui->palette[6], "new song");
	if(dui->DoButtonRed(glkitGetWidth()-605, glkitGetHeight()-53, "btn_snew", "erase everything and start a new song (unsaved data will be lost)"))
	{
//		int choice=MessageBox(hWndMain, "Are you sure you want to erase everything and start from scratch?", "Caution", MB_ICONEXCLAMATION|MB_OKCANCEL);
		if(platform_MessageBox((DWORD)hWndMain, "Are you sure you want to erase everything and start from scratch?", "Caution", MESSAGEBOX_OKCANCEL))
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
	dui->DrawText(glkitGetWidth()-480, glkitGetHeight()-13, dui->palette[6], "hide");
	dui->DrawLED(glkitGetWidth()-503-10, glkitGetHeight()-14, 0, !thesong->showsong);
	if(dui->DoButton(glkitGetWidth()-503, glkitGetHeight()-14, "btn_sshow", "(F9) hide song area to conserve processing power when editing parts or instruments on a slow machine"))
		thesong->showsong=!thesong->showsong;
	if(input->KeyPressed(DIK_F9))
	{
		if(!shortkeydown)
			thesong->showsong=!thesong->showsong;
		shortkey=true;
	}

	dui->DrawText(glkitGetWidth()-480, glkitGetHeight()-25, dui->palette[6], "skin");
	if(dui->DoButton(glkitGetWidth()-503, glkitGetHeight()-26, "btn_sskin", "select skin for user interface"))
		dui->PopupMenu(6, 0);

	// gearstacks
	dui->DrawText(353, 3, dui->palette[5], "instruments:");
	if(dui->CheckRCResult(51) || dui->CheckRCResult(32))
	{
		char filename[256];
		if(dui->SelectFileLoad(filename, 2))
		{
			FILE *file=fopen(filename, "rb");
			if(file==NULL)
				platform_MessageBox((DWORD)hWndMain, "File not found", "Error", MESSAGEBOX_OK);
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
					platform_MessageBox((DWORD)hWndMain, "Unknown instrument format (maybe incompatible version)", "Error", MESSAGEBOX_OK);
				else
				{
					gearstacks[fcurstack].num_effects=0;
					kfread2(&gearstacks[fcurstack].num_effects, 1, sizeof(int), file);
					for(int j=0;j<gearstacks[fcurstack].num_effects;j++)
					{
						if(!LoadEffect(&gearstacks[fcurstack], j, file, false, 3))
							platform_MessageBox((DWORD)hWndMain, "Unknown instrument format (maybe incompatible version)", "Error", MESSAGEBOX_OK);
					}
					audiostream->AddGearStack(&gearstacks[fcurstack]);
				}
				fclose(file);
			}
		}
	}
	if(dui->CheckRCResult(33))
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
				platform_MessageBox((DWORD)hWndMain, "Unable to save instrument (write protected?)", "Error", MESSAGEBOX_OK);
			if(file!=NULL)
				fclose(file);
		}
	}
	if(dui->CheckRCResult(34))
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
			for(int i=fcurstack;i>=0;i--)
				if(gearstacks[i].instrument!=NULL)
				{
					fcurstack=i;
					break;
				}
			for(int i=fcurstack;i<99;i++)
				if(gearstacks[i].instrument!=NULL)
				{
					fcurstack=i;
					break;
				}
		}
		else
		{
			char string[256];
			if(numused>1)
				sprintf(string, "Can't remove instrument, it's in use by %i parts", numused);
			else
				sprintf(string, "Can't remove instrument, it's in use by 1 part (%s)", mungoparts[lastused]->name);
			platform_MessageBox((DWORD)hWndMain, string, "Info", MESSAGEBOX_OK);
		}
	}
	if(dui->CheckRCResult(50))
		dui->PopupInstrumentSelect();

	// instrument area
	int ngs=0;
	for(int i=0;i<99;i++)
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
		int tx=427+ngs/7*100;
		int ty=3+(ngs%7)*8;
		if(gearstacks[i].instrument!=NULL)
		{
			int ofsx=0;
			if(ngs+1<=7)
				ofsx=7;
			dui->DrawBar(tx-8+ofsx, ty+2, 6, 6, ColorFromHSV(gearstacks[i].instrument->gui_hue, 0.45f, 0.7f));
			dui->DrawBox(tx-8+ofsx, ty+2, 6, 6, dui->palette[6]);
		}
		dui->StartFlatWindow(tx, ty, 100, 8, "minwin");
		if(ngs+1<10)
			dui->DrawText(7, 0, dui->palette[color], string1);
		else
			dui->DrawText(0, 0, dui->palette[color], string1);
		if(color==9)
			color=18;
		if(gearstacks[i].instrument!=NULL && gearstacks[i].instrument->IsTriggered())
			color=3;
		dui->DrawText(18, 0, dui->palette[color], string2);
		dui->EndWindow();
	}

	bool deselect_instrument_window=false;
	if(dui->lmbclick || dui->rmbclick) // select or rename instrument
	{
		for(int i=0;i<99;i++)
		{
			int x=427+i/7*100;
			int y=3+(i%7)*8;
			if(dui->mousex>=x && dui->mousex<x+100 && dui->mousey>=y && dui->mousey<y+8)
			{
				Keyboard_AllUp();
				fcurstack=i;
				if(dui->rmbclick)
				{
					if(gearstacks[i].instrument!=NULL)
						dui->PopupMenu(3, i);
					else
						dui->PopupMenu(5, i);
					deselect_instrument_window=true;
				}
				if(dui->dlmbclick && gearstacks[i].instrument!=NULL)
				{
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

	// rename instrument	
	if(dui->CheckRCResult(30))
		dui->EditString(gearstacks[dui->rcresult_param].instrument->Name());

	// rename spart by keyboard shortcut
	Part* spart=thesong->GetSelectedPart();
	if(spart!=NULL && dui->f2_down && !dui->editstring && dui->NoActiveWindow())
		dui->EditString(spart->name);

	bool multiple_selected=(thesong->NumSelectedParts()>1);

	int copyclone_choice=0;
	if(dui->CheckRCResult(21) || (current_part==NULL && dui->ctrl_down && input->KeyPressed(DIK_C))) // copy spart
	{
		copyclone_choice=1;
		dui->CloseMenu();
	}
	if(dui->CheckRCResult(22) || (current_part==NULL && dui->ctrl_down && input->KeyPressed(DIK_D))) // clone spart
	{
		copyclone_choice=2;
		dui->CloseMenu();
	}
	if(multiple_selected)
		copyclone_choice=0;
	if(copyclone_choice>0)
	{
		Part* spart=thesong->GetSelectedPart();
		if(spart!=NULL)
		{
			float zoom=0.1f/64; // yikes, let's just conclude that I'm not really using it ;)
			thesong->ghostpart_width=(int)(spart->Length()*zoom);
			thesong->ghostpart_mode=copyclone_choice;
		}
	}

	bool mouse_in_song_area=dui->MouseInZone(0, 70, glkitGetWidth(), glkitGetHeight()-135);
	bool menu_create_part=false;
	if(dui->CheckRCResult(40))
		menu_create_part=true;
	char* keep_part_selected=NULL;
	if(menu_create_part || (((dui->lmbclick && thesong->ghostpart_mode>0) || dui->mmbclick) && mouse_in_song_area)) // add part
	{
		int rmx=dui->mousex;
		int rmy=dui->mousey;
		if(menu_create_part)
		{
			rmx=dui->rcresult_mousex;
			rmy=dui->rcresult_mousey;
		}
		int imx=(int)(rmx+2+thesong->iscrollx)/4*160*16;
		int imy=(int)(rmy-60+thesong->iscrolly);
		
		for(int i=0;i<maxparts;i++)
			if(mungoparts[i]==NULL)
				break;

		char* sid=NULL;				
		if(!multiple_selected && (dui->ctrl_down || thesong->ghostpart_mode==2) && thesong->GetSelectedPart()!=NULL) // create clone of selected part
			sid=thesong->InsertPart(thesong->GetSelectedPart(), imx, imy/10);
		else if(!multiple_selected && (dui->shift_down || thesong->ghostpart_mode==1) && thesong->GetSelectedPart()!=NULL) // create copy of selected part
		{
			mungoparts[i]=new Part(thesong->GenGuid());
			mungoparts[i]->CopyOf(thesong->GetSelectedPart());
			mungoparts[i]->winx=50+rnd(500);
			mungoparts[i]->winy=300+rnd(50);
			sid=thesong->InsertPart(mungoparts[i], imx, imy/10);
		}
		else if(thesong->ghostpart_mode==0 && !dui->ctrl_down && !dui->shift_down) // create new part
		{
			if(gearstacks[fcurstack].instrument==NULL)
				platform_MessageBox((DWORD)hWndMain, "Select an instrument first.", "Info", MESSAGEBOX_OK);
			else
			{
				mungoparts[i]=new Part(thesong->GenGuid());
				mungoparts[i]->SetGearStack(&gearstacks[fcurstack], fcurstack);
				mungoparts[i]->winx=50+rnd(500);
				mungoparts[i]->winy=300+rnd(50);
				sid=thesong->InsertPart(mungoparts[i], imx, imy/10);
				mungoparts[i]->Popup();
				current_part=mungoparts[i];
			}
		}
		if(sid!=NULL)
		{
			thesong->DeselectPart();
			thesong->SelectPart(sid);
			keep_part_selected=sid;
		}
	}

	if(dui->CheckRCResult(23) || (dui->delete_pressed && dui->NoActiveWindow())) // delete part (sometimes crashes!)
	{
		char *sid=thesong->GetSelectedSid();
		while(sid!=NULL)
		{
			Part *spart=thesong->GetSelectedPart();
			if(spart==NULL)
				LogPrint("APOCALYPTIC ERROR: spart is NULL!"); // can't happen, since GetSelectedPart() does the same thing as GetSelectedSid()
			if(thesong->RemovePart(sid)) // true means the part should be deleted
			{
				LogPrint("main: removing references to deleted part");
				audiostream->RemovePart(spart);
				if(current_part==spart)
					current_part=NULL;
				for(int i=0;i<maxparts;i++)
					if(mungoparts[i]==spart)
						mungoparts[i]=NULL;
				LogPrint("main: delete part object");
				// CRASH
				delete spart;                           // TODO: try deferring this delete for later, to avoid any possibility of the stream accessing it after deletion (maybe use some flag that is raised when stream is done with the next buffer)
				LogPrint("main: part deletion done");
			}
			sid=thesong->GetSelectedSid();
		}
	}

	// see if user tried to set active instrument for a spart
	if(current_part==NULL && dui->CheckRCResult(31))
	{
		Part* spart=thesong->GetSelectedPart();
		if(spart!=NULL)
		{
			GearStack *newgs=GetCurrentGearstack();
			if(newgs->instrument!=NULL)
			{
				spart->gearstack=newgs;
				spart->gearid=spart->gearstack->instrument->GetGid();
			}
		}
	}

	// deselect parts if clicked outside (unless we click in area beneath song view)
	if(dui->lmbclick/* && !dui->MouseInZone(0, glkitGetHeight()-135, glkitGetWidth(), glkitGetHeight())*/)
	{
		dui->DeselectWindow();
		current_part=NULL;
	}

	thesong->scrollfollow=scrollfollow;
	thesong->DoInterface(dui, keep_part_selected);
	Part *ppart=current_part;

	if(keep_part_selected!=NULL)
	{
		thesong->SelectPart(keep_part_selected);
		thesong->selectbox=false; // it's the ugly-squad intervening!
	}

	// show and handle hovering ghost part (for copy/clone)
	if(thesong->ghostpart_mode>0 && mouse_in_song_area)
	{
		int rmx=dui->mousex;
		int rmy=dui->mousey;
		int imx=(int)(rmx+2+thesong->iscrollx)/4*4+thesong->iscrollx;
		int imy=rmy/10*10;
		dui->DrawBox(imx, imy, thesong->ghostpart_width, 10, dui->palette[9]);
		if(thesong->ghostpart_mode==1)
			dui->DrawText(imx+5, imy, dui->palette[10], "copy");
		else
			dui->DrawText(imx+5, imy, dui->palette[10], "clone");
	}
	if(dui->lmbclick || dui->rmbclick || dui->mmbclick)
		thesong->ghostpart_mode=0;

	dui->EndWindow();

	for(int i=0;i<maxstacks;i++)
		if(gearstacks[i].instrument!=NULL)
			gearstacks[i].instrument->SetGid(i);

	// part windows
	int pactive=-1;
	int ptopmost=-1;
	for(int i=0;i<maxparts;i++) // TODO: could use max-used value
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
			if(deselect_instrument_window)
				dui->DropWindowFocus("iwin");
			bool winactive=dui->StartWindowSnapX(gs->winx, gs->winy, twx, 400, 49+gs->num_effects*29, windowtitle, "iwin");

			// color selector
			for(int cx=0;cx<96;cx+=4)
			{
				Color scol=ColorFromHSV((float)cx/98, 0.3f, 0.6f);
				dui->DrawBar(275+cx, 2, 4, 7, scol);
				dui->DrawBar(275+cx, 2, 1, 7, dui->palette[6]);
			}
			dui->DrawBar(276+(int)(gs->instrument->gui_hue*93), 2, 2, 7, dui->palette[9]);
			dui->DrawBox(275, 2, 97, 8, dui->palette[6]);
			dui->DrawBox(275, 3, 97, 6, dui->palette[6]);
			bool colsel_hover=dui->MouseInZone(gs->winx+275, gs->winy, 97, 12);
			if(dui->lmbclick && colsel_hover)
				gs->instrument->dragging_icolor=true;
			if(!dui->lmbdown)
				gs->instrument->dragging_icolor=false;
			if(gs->instrument->dragging_icolor)
			{
				gs->instrument->gui_hue=(float)(dui->mousex-(gs->winx+276))/93;
				if(gs->instrument->gui_hue<0.0f) gs->instrument->gui_hue=0.0f;
				if(gs->instrument->gui_hue>1.0f) gs->instrument->gui_hue=1.0f;
			}
			Color icol=ColorFromHSV(gs->instrument->gui_hue, 0.4f, 0.6f);
			dui->DrawBar(377, 2, 8, 7, icol);
			dui->DrawBox(376, 2, 10, 8, dui->palette[6]);

			if(twx==glkitGetWidth()-400)
				gs->snappedright=true;
			else
				gs->snappedright=false;
			int twy=gs->winy;
			if(winactive)
			{
				if(ppart!=NULL && ppart->wantrelease && ppart->gearstack==gs)
					ppart->wantrelease=false;
				else if(numkeysdown>0 && i!=fcurstack && gearstacks[fcurstack].instrument!=NULL)
				{
					gearstacks[fcurstack].instrument->ReleaseAll();
					numkeysdown=0;
				}
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
				if(numkeysdown>0 && current_part->gearstack!=gs && gearstacks[fcurstack].instrument!=NULL)
				{
					gearstacks[fcurstack].instrument->ReleaseAll();
					numkeysdown=0;
				}
			}
			if(gs->winpop)
			{
				dui->RequestWindowFocus("iwin");
				gs->winpop=false;
			}
			dui->DrawLED(389, 1, 0, gs->instrument->IsTriggered());
			gs->instrument->DefaultWindow(dui, twx, twy+11);
			if(abort_render)
			{
				LogPrint("main: bailing from render");

				if(disabled)
					input->Enable();
				disabled=false;

				return;
			}
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
	for(int i=0;i<maxparts;i++) // TODO: could use max-used value
		if(mungoparts[i]!=NULL)
		{
			if(mungoparts[i]->wantrelease && mungoparts[i]->gearstack!=&gearstacks[fcurstack])
				mungoparts[i]->gearstack->instrument->ReleaseAll();
			mungoparts[i]->wantrelease=false;
		}
	if(input->KeyPressed(DIK_ESCAPE))
	{
		if(!shortkeydown)
		{
			for(int i=0;i<maxstacks;i++)
				if(gearstacks[i].instrument!=NULL)
					gearstacks[i].instrument->StopChannels();

			EarClear();
		}
		shortkey=true;
	}

	static part_play_key=false;
	if(input->KeyPressed(DIK_SPACE)) // space to play part, conflicts with space to scroll/pan
	{
		if(!shortkeydown)
		{
			if(current_part!=NULL)
				part_play_key=true;
		}
		shortkey=true;
	}
	else if(part_play_key) // space pressed and released without mouse click -> play part
	{
		current_part->PlayButton(audiostream);
		part_play_key=false;
	}
	if(part_play_key && dui->lmbdown) // mouse clicked -> cancel request to play part, since we're probably scrolling
		part_play_key=false;

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
		if(rcresult==3)
			BindAxisToKnob(0, value);
		if(rcresult==4)
			BindAxisToKnob(1, value);
		if(rcresult==5)
		{
			BindAxisToKnob(0, NULL);
			BindAxisToKnob(1, NULL);
		}

		int rctype=rcresult/10;

		if(rctype>=6 && rctype<=8) // change skin
		{
			int skin_id=rcresult-rctype*10;
			char path[512];
			strcpy(path, defpath);
			strcat(path, "skins/");
			strcat(path, dui->skin_names[skin_id]);
			dui->LoadSkin(path);
		}

		if(rctype>=9 && rctype<=11 && midi_id!=-1) // change midi input device (unless disabled)
		{
			midi_id=rcresult-rctype*10-1;
			midi_init(midi_id);
		}

		if(rctype==1) // add instrument
		{
			bool ok=true;
			if(gearstacks[fcurstack].instrument!=NULL)
			{
				if(platform_MessageBox((DWORD)hWndMain, "Adding a new instrument here will replace the current one, are you sure?", "Caution", MESSAGEBOX_OKCANCEL))
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
			if(ok)
			{
				bool has_gapan=true;
				switch(rcresult-10)
				{
				case 0: // xnes
					gearstacks[i].instrument=new gin_xnes();
					break;
				case 1: // chip
					gearstacks[i].instrument=new gin_chip();
					break;
				case 2: // vsmp
					gearstacks[i].instrument=new gin_vsmp();
					break;
				case 3: // swave
					gearstacks[i].instrument=new gin_swave();
					break;
				case 4: // protobass
					gearstacks[i].instrument=new gin_protobass();
					break;
				case 5: // midinst
					gearstacks[i].instrument=new gin_midinst();
					has_gapan=false;
					break;
				case 6: // midperc
					gearstacks[i].instrument=new gin_midperc();
					has_gapan=false;
					break;
				case 7: // VSTi
					gearstacks[i].instrument=new gin_vsti();
					break;
				case 8: // operator
					gearstacks[i].instrument=new gin_operator();
					break;
				case 9: // wavein
					gearstacks[i].instrument=new gin_wavein();
					break;
				}
				if(has_gapan)
				{
					gearstacks[i].num_effects=1;
					gearstacks[i].effects[0]=new gef_gapan();
				}
				else
					gearstacks[i].num_effects=0;
				audiostream->AddGearStack(&gearstacks[i]);

				// popup instrument window
				fcurstack=i;
				if(!gearstacks[i].winshow)
				{
					if(gearstacks[i].winx<-400+16 || gearstacks[i].winx>glkitGetWidth()-16 || gearstacks[i].winy>glkitGetHeight()-16) // never shown before, or outside
					{
						gearstacks[i].winx=100+rnd(glkitGetWidth()-400-200);
						gearstacks[i].winy=60+rnd(300);
					}
					ToggleStackWindow(i);
				}
			}
		}
	}

	if(shortkey)
		shortkeydown=true;
	else
		shortkeydown=false;

	disabled=false;
	input->Enable();

	if(firstrender) LogPrint("render: end");
	firstrender=false;
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

bool note_playing=false;

DWORD exit_time=0;

bool glkCalcFrame()
{
	input->Update();

	if(glkit_close)
	{
		exit_time=timeGetTime();
		return false;
	}

	bool kbkbhit=false;
	for(int i=0;i<33;i++)
	{
		if(input->KeyPressed(kbkbk[i]) || (InChildWindow()!=NULL && (GetAsyncKeyState(kbkbc[i])&KF_UP)))
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
			{
				Keyboard_KeyUp(kbkbm[i]+kbkboctave*12);
			}
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

	return true;
}

void ProgressBar()
{
	if(progressdone)
		return;
	int ctime=timeGetTime();
	if(progress==0)
		progressmark=ctime;
	progress++;
	if(ctime-progressmark>50) // render at most every 50 ms to avoid slowing things down too much
	{
		progressmark=ctime;
		glkitInternalRender(false);
	}
}

void glkPreInit()
{
	strcpy(portaudio_latency, "100");

	FILE *cfg=fopen("config.txt", "r");
	if(cfg!=NULL)
	{
		char junk[32];
		int doublesize=0;
		fscanf(cfg, "%s %i %i %i %i %i", junk, &glkit_winx, &glkit_winy, &glkit_width, &glkit_height, &glkit_winmax);
		fscanf(cfg, "%s %i", junk, &log_on);
		fscanf(cfg, "%s %s", junk, portaudio_latency);
		fscanf(cfg, "%s %i", junk, &midi_id);
		fscanf(cfg, "%s %i", junk, &doublesize);
		if(junk[0]=='t') // old config version, no doublesize (this line was actually tooltips) ************* temporary compatibility hack (remove when previous version is unlikely to circulate)
			doublesize=0;
		fclose(cfg);
		if(doublesize==1)
			glkitSetHalfRes(true);
	}

	platform_GetExecutablePath(defpath);
	int si=strlen(defpath)-1;
	while(si>0 && defpath[si]!='\\')
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

char rpath_buffer[512];
char* RPath(char* filename)
{
	strcpy(rpath_buffer, defpath);
	strcat(rpath_buffer, filename);
	return rpath_buffer;
}

void glkInit(char* cmd)
{
	int gtime=timeGetTime();

	SetMusagiDir(defpath);
                 
	LogPrint("main: default directory==\"%s\"", defpath);
	LogPrint("--- init start");
	ProgressBar();

	LogPrint("init input");
	input=new DPInput(hWndMain, hInstanceMain);

	ProgressBar();
	LogPrint("init midi");
	if(midi_id==-1)
		LogPrint("midi disabled in config");
	else if(!midi_init(midi_id))
		LogPrint("ERROR: midi failed");

	LogPrint("portaudio environment variable");
	platform_set_portaudio_latency(portaudio_latency);

	ProgressBar();
	srand(time(NULL));

	platform_init_keyboard();
	for(int i=0;i<33;i++)
		kbkbkeydown[i]=false;
	kbkbsomething=false;
	kbkboctave=2;
	
	midioctave=1;

	LogPrint("InitGlobals()");
	InitGlobals();
	tempo=3500;

	for(int i=0;i<2;i++)
		joystick_binding[i]=NULL;

	LogPrint("init audiostream");
	audiostream=new AudioStream();

	LogPrint("Reset()");
	Reset();
	
	ProgressBar();
	
	LogPrint("init dui");
	dui=new DUI();
	dui->glkmouse=&glkmouse;

	if(midi_in_count>0)
	{
		dui->num_midiin=midi_in_count;
		for(int i=0;i<midi_in_count;i++)
			strcpy(dui->midiin_names[i], midi_in_names[i]);
	}

	LogPrint("audiostream->StartStream()");
	audiostream->StartStream(1024, &pa_callback);

	ProgressBar();

	// reset path strings
	for(int pi=0;pi<5;pi++)
		dirstrings[pi][0]='\0';

	// load config
	LogPrint("main: loading config");
	FILE *cfg=fopen("config.txt", "r");

	if(cfg!=NULL)
	{
		char junk[32];
		fscanf(cfg, "%s %i %i %i %i %i", junk, junk, junk, junk, junk, junk);
		fscanf(cfg, "%s %s", junk, junk); // log
		fscanf(cfg, "%s %s", junk, portaudio_latency); // portaudio latency (surprise)
		fscanf(cfg, "%s %s", junk, junk); // midi
		char temp[128];
		fscanf(cfg, "%s %s", junk, temp); // doublesize
		if(junk[0]=='t') // old config version, no doublesize (this line was actually tooltips) ************* temporary compatibility hack (remove when previous version is unlikely to circulate)
			sscanf(temp, "%i", &dui->tooltips_on);
		else
			fscanf(cfg, "%s %i", junk, &dui->tooltips_on);
		fscanf(cfg, "%s %i", junk, &scrollfollow);
		fscanf(cfg, "%s %f", junk, &speaker_volume);
		fscanf(cfg, "%s %f", junk, &audiostream->metronome_vol);
		fscanf(cfg, "%s %s %i", junk, junk, &kbkboctave);
		fscanf(cfg, "%s %s %i", junk, junk, &midioctave);
		// read stored path strings
		char ch=' ';
		fread(&ch, 1, 1, cfg); // read '\n'
		for(int pi=0;pi<6;pi++)
		{
			int si=0;
			fread(&ch, 1, 1, cfg);
			while(ch!='\n')
			{
				dirstrings[pi][si++]=ch;
				fread(&ch, 1, 1, cfg);
			}
			dirstrings[pi][si]='\0';
			LogPrint("main: loaded dir for type %i: \"%s\"", pi, dirstrings[pi]);
		}
		fclose(cfg);
	}

	ProgressBar();

	// dispatch midi thread
	LogPrint("main: start midi thread");
	platform_start_midi_thread();

	ProgressBar();

	LogPrint("--- init done");
	LogPrint("total init time=%i (excluding window creation and opengl init)", timeGetTime()-gtime);

	progressdone=true;
	LogPrint("progress=%i", progress);
	
	if(cmd[0]!='\0') // load song given on command line, if any
	{
		if(cmd[0]=='\"')
			cmd++;
		int length=strlen(cmd);
		for(int i=0;i<length;i++)
			if(cmd[i]=='\"')
			{
				cmd[i]='\0';
				break;
			}
		curfilename[0]='\0';
		if(!LoadSong(cmd))
			platform_MessageBox((DWORD)hWndMain, "Unknown song format (maybe incompatible version)", "Error", MESSAGEBOX_OK);
		else
			strcpy(curfilename, cmd);
	}
}

void glkFree()
{
	LogPrint("glkFree()");

	DWORD free_time=timeGetTime();

	char cfgpath[512];
	strcpy(cfgpath, defpath);
	strcat(cfgpath, "config.txt");
	LogPrint("main: writing cfg to \"%s\"", cfgpath);
	FILE *cfg=fopen(cfgpath, "w");
	if(cfg!=NULL)
	{
		int doublesize=0;
		if(glkitHalfRes())
			doublesize=1;
		fprintf(cfg, "window: %i %i %i %i %i\n", glkit_winx, glkit_winy, glkit_width, glkit_height, glkit_winmax);
		fprintf(cfg, "log: %i\n", log_on);
		fprintf(cfg, "latency: %s\n", portaudio_latency);
		fprintf(cfg, "mididevice: %i\n", midi_id);
		fprintf(cfg, "doublesize: %i\n", doublesize);
		fprintf(cfg, "tooltips: %i\n", dui->tooltips_on);
		fprintf(cfg, "scrollfollow: %i\n", scrollfollow);
		fprintf(cfg, "speaker: %.2f\n", speaker_volume);
		fprintf(cfg, "metronome: %.2f\n", audiostream->metronome_vol);
		fprintf(cfg, "keyboard octave: %i\n", kbkboctave);
		fprintf(cfg, "midi octave: %i\n", midioctave);
		// write stored path strings
		for(int pi=0;pi<6;pi++)
			fprintf(cfg, "%s\n", dirstrings[pi]);
		fclose(cfg);
	}

	LogPrint("close midi thread");
	DWORD cmt=timeGetTime();
	midithread_getlost=true;
	while(midithread_getlost)
	{
		Sleep(20);
	}
	Sleep(20);
	CloseHandle(hMidiThread);
	LogPrint("spent %i ms closing midi", timeGetTime()-cmt);

	LogPrint(" - timestamp: %i ms", timeGetTime()-free_time);

	LogPrint("delete audiostream");
	audiostream->StopFileOutput();
	delete audiostream;

	LogPrint(" - timestamp: %i ms", timeGetTime()-free_time);

	LogPrint("delete input");
	delete input;
	LogPrint("delete dui");
	delete dui;

	LogPrint(" - timestamp: %i ms", timeGetTime()-free_time);

	LogPrint("delete instruments and gearstacks");
	for(int i=0;i<maxstacks;i++)
		if(gearstacks[i].instrument!=NULL)
		{
			LogPrint("%s", gearstacks[i].instrument->Name());
			delete gearstacks[i].instrument;
			for(int j=0;j<gearstacks[i].num_effects;j++)
				if(gearstacks[i].effects[j]!=NULL)
					delete gearstacks[i].effects[j];
		}

	LogPrint(" - timestamp: %i ms", timeGetTime()-free_time);

	LogPrint("delete parts"); // CRASH: here, once
	for(int i=0;i<maxparts;i++)
		if(mungoparts[i]!=NULL)
			delete mungoparts[i];

	LogPrint(" - timestamp: %i ms", timeGetTime()-free_time);

	LogPrint("delete song");
	delete thesong;

	LogPrint(" - timestamp: %i ms", timeGetTime()-free_time);

	LogPrint("midi_exit()");
	midi_exit();

	LogPrint(" - cleanup took %i ms", timeGetTime()-free_time);

	LogPrint("cleanup done (%i ms after requesting exit)", timeGetTime()-exit_time);
}

