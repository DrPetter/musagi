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
//HMIDIOUT handle_out;

void CALLBACK midiCallback(HMIDIIN handle, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	unsigned char status, message_byte1, message_byte2;
	DWORD message;

	unsigned char cur_note/*, cur_vel*/;

	if(uMsg==MIM_DATA) // Received some regular(channel) MIDI message
	{
		cur_note=(unsigned char)((dwParam1 & 0xFF00)>>8);
//		cur_vel=(unsigned char)(dwParam1 & 0x00FF);

		if((dwParam1&0xFF0000)==0x640000) // Note on
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
/*
		message_byte1=cur_note;
		message_byte2=0x7F; // velocity=127
//		message_byte2=cur_vel;
		message=(status) | (message_byte1<<8) | (message_byte2<<16);
		midiOutShortMsg(handle_out, message);*/
	}
}

bool midi_init(int id)
{
	unsigned char status, message_byte1, message_byte2;
	DWORD message;

//    HMIDIIN handle_in;
//    MIDIHDR midiHdr;
	bool midi_ok;
//	CTimer *timer;

//	timer=new CTimer();
//	timer->init();

//	printf("Initializing MIDI In...");
	midi_ok=false;
    if(!midiInOpen(&handle_in, id, (DWORD)midiCallback, 0,CALLBACK_FUNCTION))
	{
		midiHdr.lpData =(char *) &SysXBuffer[0];
		midiHdr.dwBufferLength = sizeof(SysXBuffer);
		midiHdr.dwFlags = 0;
		if(!midiInPrepareHeader(handle_in, &midiHdr, sizeof(MIDIHDR)))
			midi_ok=true;
	}
	if(midi_ok)
	{
//		printf(" Success!\n");
		midiInAddBuffer(handle_in, &midiHdr, sizeof(MIDIHDR));
		midiInStart(handle_in);
	}
	else
	{
//		printf(" Failed\n");
		return false;
	}
/*
	printf("Initializing MIDI Out...");
	midi_ok=false;
	if(!midiOutOpen(&handle_out, 0, (unsigned long)NULL, 0, 0))
		midi_ok=true;
	if(midi_ok)
		printf(" Success!\n");
	else
	{
		printf(" Failed\n");
		return 0;
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
	return true;
}

void midi_exit()
{
	// Close MIDI In
	midiInReset(handle_in);
	midiInUnprepareHeader(handle_in, &midiHdr, sizeof(MIDIHDR));
	midiInClose(handle_in);
	// Close MIDI Out
//	midiOutClose(handle_out);
}

