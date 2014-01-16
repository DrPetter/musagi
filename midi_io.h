/*
#define bool unsigned int
#define true 1
#define false 0
*/

//int vk_timer=0, vk_repeat=2;

unsigned char SysXBuffer[256];
unsigned char SysXFlag=0;

unsigned char last_note=0;

MIDIHDR midiHdr;
HMIDIIN handle_in;
HMIDIOUT handle_out;

bool midi_in_ok=false;
bool midi_out_ok=false;

char midi_in_names[64][64];
int midi_in_count=0;
/*
int GetMidiInCount()
{
	return midi_in_count;
}

char** GetMidiInNames()
{
	return midi_in_names;
}
*/
bool midi_exporting;
FILE* midi_outfile;
int midi_stime;
bool midi_firstevent;
int midi_datasize;

//bool midi_channelbusy[16];
int midi_channelprogram[16];
void* midi_channelowner[16];
int midi_channelage[16];

char midi_inames[128][28]={
"Acoustic Grand Piano",
"Bright Acoustic Piano",
"Electric Grand Piano",
"Honky-tonk Piano",
"Electric Piano 1",
"Electric Piano 2",
"Harpsichord",
"Clavi",
"Celesta",
"Glockenspiel",
"Music Box",
"Vibraphone",
"Marimba",
"Xylophone",
"Tubular Bells",
"Dulcimer",
"Drawbar Organ",
"Percussive Organ",
"Rock Organ",
"Church Organ",
"Reed Organ",
"Accordion",
"Harmonica",
"Tango Accordion",
"Acoustic Guitar (nylon)",
"Acoustic Guitar (steel)",
"Electric Guitar (jazz)",
"Electric Guitar (clean)",
"Electric Guitar (muted)",
"Overdriven Guitar",
"Distortion Guitar",
"Guitar harmonics",
"Acoustic Bass",
"Electric Bass (finger)",
"Electric Bass (pick)",
"Fretless Bass",
"Slap Bass 1",
"Slap Bass 2",
"Synth Bass 1",
"Synth Bass 2",
"Violin",
"Viola",
"Cello",
"Contrabass",
"Tremolo Strings",
"Pizzicato Strings",
"Orchestral Harp",
"Timpani",
"String Ensemble 1",
"String Ensemble 2",
"SynthStrings 1",
"SynthStrings 2",
"Choir Aahs",
"Voice Oohs",
"Synth Voice",
"Orchestra Hit",
"Trumpet",
"Trombone",
"Tuba",
"Muted Trumpet",
"French Horn",
"Brass Section",
"SynthBrass 1",
"SynthBrass 2",
"Soprano Sax",
"Alto Sax",
"Tenor Sax",
"Baritone Sax",
"Oboe",
"English Horn",
"Bassoon",
"Clarinet",
"Piccolo",
"Flute",
"Recorder",
"Pan Flute",
"Blown Bottle",
"Shakuhachi",
"Whistle",
"Ocarina",
"Lead 1 (square)",
"Lead 2 (sawtooth)",
"Lead 3 (calliope)",
"Lead 4 (chiff)",
"Lead 5 (charang)",
"Lead 6 (voice)",
"Lead 7 (fifths)",
"Lead 8 (bass + lead)",
"Pad 1 (new age)",
"Pad 2 (warm)",
"Pad 3 (polysynth)",
"Pad 4 (choir)",
"Pad 5 (bowed)",
"Pad 6 (metallic)",
"Pad 7 (halo)",
"Pad 8 (sweep)",
"FX 1 (rain)",
"FX 2 (soundtrack)",
"FX 3 (crystal)",
"FX 4 (atmosphere)",
"FX 5 (brightness)",
"FX 6 (goblins)",
"FX 7 (echoes)",
"FX 8 (sci-fi)",
"Sitar",
"Banjo",
"Shamisen",
"Koto",
"Kalimba",
"Bag pipe",
"Fiddle",
"Shanai",
"Tinkle Bell",
"Agogo",
"Steel Drums",
"Woodblock",
"Taiko Drum",
"Melodic Tom",
"Synth Drum",
"Reverse Cymbal",
"Guitar Fret Noise",
"Breath Noise",
"Seashore",
"Bird Tweet",
"Telephone Ring",
"Helicopter",
"Applause",
"Gunshot"};

char midi_pnames[47][22]={
"Acoustic Bass Drum",
"Bass Drum 1",
"Side Stick",
"Acoustic Snare",
"Hand Clap",
"Electric Snare",
"Low Floor Tom",
"Closed Hi-Hat",
"High Floor Tom",
"Pedal Hi-Hat",
"Low Tom",
"Open Hi-Hat",
"Low-Mid Tom",
"Hi-Mid Tom",
"Crash Cymbal 1",
"High Tom",
"Ride Cymbal 1",
"Chinese Cymbal",
"Ride Bell",
"Tambourine",
"Splash Cymbal",
"Cowbell",
"Crash Cymbal 2",
"Vibraslap",
"Ride Cymbal 2",
"Hi Bongo",
"Low Bongo",
"Mute Hi Conga",
"Open Hi Conga",
"Low Conga",
"High Timbale",
"Low Timbale",
"High Agogo",
"Low Agogo",
"Cabasa",
"Maracas",
"Short Whistle",
"Long Whistle",
"Short Guiro",
"Long Guiro",
"Claves",
"Hi Wood Block",
"Low Wood Block",
"Mute Cuica",
"Open Cuica",
"Mute Triangle",
"Open Triangle"};

void CALLBACK midiCallback(HMIDIIN handle, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	unsigned char status, message_byte1, message_byte2;
	DWORD message;

	unsigned char cur_note/*, cur_vel*/;

	if(uMsg==MIM_DATA) // Received some regular(channel) MIDI message
	{
		Debug_MidiMsg(dwParam1);

		cur_note=(unsigned char)((dwParam1 & 0xFF00)>>8);
//		cur_vel=(unsigned char)(dwParam1 & 0x00FF);

		if((dwParam1&0xF0)==0x90 || (dwParam1&0xF0)==0x80)
		{
			if((dwParam1&0xF0)==0x90 && (dwParam1&0xFF0000)!=0x000000) // Note on (should I actually be checking the LSByte for 0x90/on and 0x80/off? it seems to always register at 0x90
			{
	//			if(recording)
	//				AddNote(trackpos, cur_note, 127, true);
	//			last_note=cur_note;
	//			StartNote(cur_note-24);
	//			LogPrint("MIDI note on %i [%.6i] \"%s\"", cur_note-24, globaltick, note_string[cur_note-24]);
				Keyboard_KeyDown(cur_note-36+midioctave*12);
	//			curinstr->Trigger(cur_note-24, 0.2f);
				status=0x90;
			}
			else // Note off
			{
	//			if(recording)
	//				AddNote(trackpos, cur_note, 127, false);
	//			LogPrint("MIDI note off %i [%.6i]", cur_note-24, globaltick);
	//			if(cur_note==last_note)
	//			{
	//				StopNote();
	//				LogPrint("(real note off)");
	//			}
				Keyboard_KeyUp(cur_note-36+midioctave*12);
	//			curinstr->Release(cur_note-24);
				status=0x80;
			}
		}
/*
		message_byte1=cur_note;
		message_byte2=0x7F; // velocity=127
//		message_byte2=cur_vel;
		message=(status) | (message_byte1<<8) | (message_byte2<<16);
		midiOutShortMsg(handle_out, message);*/
	}

	Debug_MidiInc();
}

void midi_writedtime(int dtime, FILE* file)
{
	// split dtime value into 7-bit packets
	unsigned char dbytes[4];
	int dbi=0;
	bool done=false;
	while(!done)
	{
		unsigned char byte=dtime&0x7F;
		if(dbi!=0)
			byte|=0x80;
		dbytes[dbi++]=byte;
		dtime>>=7;
		if(dtime==0)
			done=true;
	}
	// write them out in reverse order
	for(int i=dbi-1;i>=0;i--)
	{
		fwrite(&dbytes[i], 1, 1, file);
		midi_datasize++;
	}
}

void midi_writemessage(int message, int numbytes, FILE* file)
{
	if(midi_firstevent)
	{
		midi_stime=0;
		midi_firstevent=false;
	}
	int dtime=(int)((float)midi_stime/44.100f); // milliseconds
	midi_writedtime(dtime, file);
	unsigned char byte;
	for(int i=0;i<numbytes;i++)
	{
		byte=message&0xFF;
		fwrite(&byte, 1, 1, file);
		message>>=8;
		midi_datasize++;
	}
	midi_stime=0;
}

void midi_noteon(int channel, int note, int velocity)
{
	if(!midi_out_ok)
		return;
	unsigned char status=0x90|channel;
	unsigned char message_byte1=note;
	unsigned char message_byte2=velocity;
	int message=(status) | (message_byte1<<8) | (message_byte2<<16);
	if(midi_exporting)
	{
		// write message to midi file stream
		midi_writemessage(message, 3, midi_outfile);
		return;
	}
	midiOutShortMsg(handle_out, message);
//	LogPrint("midi_noteon(%i)", note);
}

void midi_noteoff(int channel, int note, int velocity)
{
	if(!midi_out_ok)
		return;
	unsigned char status=0x80|channel;
	unsigned char message_byte1=note;
	unsigned char message_byte2=velocity;
	int message=(status) | (message_byte1<<8) | (message_byte2<<16);
	if(midi_exporting)
	{
		// write message to midi file stream
		midi_writemessage(message, 3, midi_outfile);
		return;
	}
	midiOutShortMsg(handle_out, message);
//	LogPrint("midi_noteoff(%i)", note);
}

void midi_changeprogram(int channel, int program)
{
	if(!midi_out_ok)
		return;
	unsigned char status=0xC0|channel;
	unsigned char message_byte1=program;
	int message=(status) | (message_byte1<<8);
	if(midi_exporting)
	{
		// write message to midi file stream
		midi_writemessage(message, 2, midi_outfile);
		return;
	}
	midiOutShortMsg(handle_out, message);
	midi_channelprogram[channel]=program;
}

int midi_allocatechannel(int preferred, int program, void* owner)
{
	// age all channels on every call
	for(int i=0;i<16;i++)
		midi_channelage[i]++;
		
	if(midi_channelowner[preferred]==owner)
	{
		if(midi_channelprogram[preferred]!=program)
		{
			midi_changeprogram(preferred, program);
//			midi_channelprogram[preferred]=program;
		}
		return preferred; // nobody has stolen the channel, so keep using it
	}
	
	// find the least-recently-used channel and steal it
	int oldest=0;
	int maxage=-1;
	for(int i=0;i<16;i++)
	{
		if(i==9) continue; // don't use drum channel
		if(midi_channelage[i]>maxage)
		{
			maxage=midi_channelage[i];
			oldest=i;
		}
/*		if(!midi_channelbusy[i])
		{
			midi_channelbusy[i]=true;
			return i;
		}*/
	}
	midi_channelowner[oldest]=owner; // register owner
	midi_changeprogram(oldest, program); // set new program for this channel
//	midi_channelprogram[oldest]=program;
	midi_channelage[oldest]=0; // rejuvenate this channel
	return oldest;
}

void midi_freechannel(int i)
{
//	midi_channelbusy[i]=false;
	midi_channelage[i]+=10000;
	midi_channelprogram[i]=-1;
	midi_channelowner[i]=NULL;
}

char* midi_getiname(int i)
{
	return midi_inames[i];
}

char* midi_getpname(int i)
{
	i-=35;
	if(i<0) i=0;
	if(i>46) i=46;
	return midi_pnames[i];
}

void midi_printerror(MMRESULT id)
{
	char string[256];
	midiInGetErrorText(id, string, 256);
	LogPrint("MIDI ERROR %i: %s", id, string);
}

void midi_exit()
{
	DWORD stime=timeGetTime();
	MMRESULT result=MMSYSERR_NOERROR;
	if(midi_in_ok)
	{
		LogPrint("midi_exit: midiInStop() @ %i ms", timeGetTime()-stime);
		if(midiInStop(handle_in)!=MMSYSERR_NOERROR)
		{
			midi_printerror(result);
			return;
		}
		// Close MIDI In
		LogPrint("midi_exit: midiInReset() @ %i ms", timeGetTime()-stime);
		if(midiInReset(handle_in)!=MMSYSERR_NOERROR)
		{
			midi_printerror(result);
			return;
		}
//		Sleep(1);
		LogPrint("midi_exit: midiInUnprepareHeader() @ %i ms", timeGetTime()-stime);
		if(result=midiInUnprepareHeader(handle_in, &midiHdr, sizeof(MIDIHDR))!=MMSYSERR_NOERROR)
		{
			midi_printerror(result);
			return;
		}
//		Sleep(1);
/*		LogPrint("midi_exit: midiInClose() @ %i ms", timeGetTime()-stime);
		if(midiInClose(handle_in)!=MMSYSERR_NOERROR) // TODO: should probably be calling this... but it tends to cause a 1-second delay which is no fun (might depend on device)
		{
			midi_printerror(result);
			return;
		}*/
	}
	if(midi_out_ok)
	{
		// Close MIDI Out
		LogPrint("midi_exit: midiOutReset() @ %i ms", timeGetTime()-stime);
		if(result=midiOutReset(handle_out)!=MMSYSERR_NOERROR)
			midi_printerror(result);
		else
		{
//			Sleep(1);
			LogPrint("midi_exit: midiOutClose() @ %i ms", timeGetTime()-stime);
			if(midiOutClose(handle_out)!=MMSYSERR_NOERROR)
				midi_printerror(result);
		}
	}
	LogPrint("midi_exit: return @ %i ms", timeGetTime()-stime);
}

bool midi_init(int id)
{
	unsigned char status, message_byte1, message_byte2;
	DWORD message;

	MMRESULT result=MMSYSERR_NOERROR;

//    HMIDIIN handle_in;
//    MIDIHDR midiHdr;
//	CTimer *timer;

//	timer=new CTimer();
//	timer->init();

	midi_exit();

/*	if(midi_in_ok)
	{
		midi_in_ok=false;
//		Sleep(200);
	}
*/
	MIDIINCAPS devicecaps;

//	printf("Initializing MIDI In...");
	midi_in_ok=false;
	if(id==-1)
		return false;
		
	int num_in_devices=midiInGetNumDevs();
	if(id>num_in_devices-1)
		id=0;
	if(num_in_devices>0)
	{
		LogPrint("detected %i midi input devices", num_in_devices);

		midi_in_count=num_in_devices;
		for(int i=0;i<num_in_devices;i++)
		{
			memset(&devicecaps, 0, sizeof(devicecaps));
			midiInGetDevCaps(i, &devicecaps, sizeof(devicecaps));
			LogPrint("%i - %s", i, devicecaps.szPname);
			strcpy(midi_in_names[i], devicecaps.szPname);
		}

		LogPrint("opening midi input device %i", id);
	    if(result=midiInOpen(&handle_in, id, (DWORD)midiCallback, 0, CALLBACK_FUNCTION)==MMSYSERR_NOERROR)
		{
			midiHdr.lpData =(char *) &SysXBuffer[0];
			midiHdr.dwBufferLength = sizeof(SysXBuffer);
			midiHdr.dwFlags = 0;
			if(midiInPrepareHeader(handle_in, &midiHdr, sizeof(MIDIHDR))==MMSYSERR_NOERROR)
				midi_in_ok=true;
		}
		else
			midi_printerror(result);
		if(midi_in_ok)
		{
			if(result=midiInAddBuffer(handle_in, &midiHdr, sizeof(MIDIHDR))!=MMSYSERR_NOERROR)
			{
				midi_printerror(result);
				midi_in_ok=false;
			}
			else if(result=midiInStart(handle_in)!=MMSYSERR_NOERROR)
			{
				midi_printerror(result);
				midi_in_ok=false;
			}
		}
	}

	if(midi_in_ok)
		LogPrint("midi in ok");
	else
		LogPrint("midi in failed");

/*	else
	{
//		printf(" Failed\n");
		return false;
	}*/

//	printf("Initializing MIDI Out...");
	midi_out_ok=false;
	if(midiOutOpen(&handle_out, 0, (unsigned long)NULL, 0, 0)==MMSYSERR_NOERROR)
		midi_out_ok=true;
	if(midi_out_ok)
	{
		LogPrint("midi out ok");
/*		status=0xC0;
		message_byte1=3;
		message_byte2=0;
		message=(status) | (message_byte1<<8) | (message_byte2<<16);
		midiOutShortMsg(handle_out, message);*/
	}
	else
		LogPrint("midi out failed");
/*	if(!midi_out_ok)
	{
//		printf(" Failed\n");
		// Close MIDI In
		midiInReset(handle_in);
		midiInUnprepareHeader(handle_in, &midiHdr, sizeof(MIDIHDR));
		midiInClose(handle_in);
		return false;
	}
*/
/*	// Set instruments
	status=0xC1;
//	message_byte1=115;
	message_byte1=116;
	message_byte2=0;
	message=(status) | (message_byte1<<8) | (message_byte2<<16);
	midiOutShortMsg(handle_out, message);

	status=0xC2;
	message_byte1=113;
	message_byte2=0;
	message=(status) | (message_byte1<<8) | (message_byte2<<16);
	midiOutShortMsg(handle_out, message);
*/
	for(int i=0;i<16;i++)
	{
//		midi_channelbusy[i]=false;
		midi_channelprogram[i]=-1;
		midi_channelowner[i]=NULL;
		midi_channelage[i]=0;
	}

	midi_exporting=false;

	if(!midi_in_ok && !midi_out_ok)
		return false;
	return true;
}

