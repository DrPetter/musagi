/*

troublesome plugins
-------------------
KarmaFX - works
Kontakt 2 - crashes, due to 8,16,32 channels?
Kompakt - crashes when triggering a note
Analog Warfare - no response to events

	some might require audioMasterGetNumAutomatableParameters
	some might require audioMasterIdle
	"passthrough of MIDI events"?
	"support for sending midi to effects that aren't synth that can take midi"?
	"vst timeinfo tweaks, getoutputlatency support"?

*/


#ifndef gin_vsti_h
#define gin_vsti_h

#include "gear_instrument.h"

#include "pluginterfaces/vst2.x/aeffectx.h"

struct VstInfo
{
	bool need_idle;
};

struct vsti_params
{
	float vol;
	float pitch;
	float prev_pitch;
	bool pitchtouch;
	float mod;
	float prev_mod;
	float preamp;
	int output_channels;
	float fprogram;
	int numprograms;
	int program;
	int curprogram;
	float foctave;
	int octave;
	char* dll_path; // full path to DLL file
	char* dll_filename; // only filename
	char* plugin_name; // actual name of instrument
	AEffect* vst_plugin;
	VstInfo* vst_info;
	HANDLE editor_handle;
	HWND editor_hwnd;
	bool editor_open;
	bool editor_closed;
	gear_instrument* self;
	
//	int debug_stream_busy;
};

// -------------------- VST stuff

typedef AEffect* (*PluginEntryProc) (audioMasterCallback audioMaster);
static VstIntPtr VSTCALLBACK HostCallback (AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt);

struct map_effect_gin
{
	AEffect* effect;
	VstInfo* vst;
};
static map_effect_gin global_vst_map[128];
static bool gvm_initialized=false;

void GVM_Init()
{
	LogPrintf("GVM_Init... ");
	for(int i=0;i<128;i++)
		global_vst_map[i].vst=NULL;
	gvm_initialized=true;
	LogPrintf("ok\n");
}

void GVM_AddInstrument(AEffect* effect, VstInfo* vst_info)
{
	if(!gvm_initialized)
		GVM_Init();
	LogPrintf("GVM_Add... ");
	for(int i=0;i<128;i++)
		if(global_vst_map[i].effect==NULL)
		{
			global_vst_map[i].effect=effect;
			global_vst_map[i].vst=vst_info;
			LogPrintf("ok\n");
			return;
		}
	LogPrintf("failed\n");
}

VstInfo* GVM_GetInfo(AEffect* effect)
{
	if(!gvm_initialized)
		GVM_Init();
	else
	{
		LogPrintf("GVM_GetInfo... ");
		for(int i=0;i<128;i++)
			if(global_vst_map[i].effect==effect)
			{
				LogPrintf("ok\n");
				return global_vst_map[i].vst;
			}
		LogPrintf("not found\n");
	}
	LogPrint("GVM_GetInfo... did init");
	return NULL;
}

void GVM_RemoveInstrument(AEffect* effect)
{
	LogPrintf("GVM_Remove... ");
	for(int i=0;i<128;i++)
		if(global_vst_map[i].effect==effect)
		{
			LogPrintf("pof ");
			global_vst_map[i].effect=NULL;
		}
	LogPrintf("ok\n");
}

//static VstTimeInfo vst_timeinfo; // common to all instruments (global musagi settings)
static float vsti_junkbuffer[1024]; // a place to dump unwanted VSTi audio output, for instance...

static int debug_cb_msgcount=0;

VstIntPtr VSTCALLBACK HostCallback(AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
{
//	LogPrint("VSTi: HostCallback()");
/*	Session* session;
	if(effect!=NULL && effect->user)
	{
		plug=(VSTPlugin*)effect->user;
		session=&plug->session();
	}
	else
	{
		plug=0;
		session=0;
	}*/

	if(effect==NULL)
	{
		LogPrint("VSTi: Received callback %i with NULL pointer", opcode);
	}

//	LogPrint("VSTi: Callback opcode %i", opcode);
	VstIntPtr result=NULL;
	switch(opcode)
	{
		case audioMasterVersion:
			LogPrint("VSTi: [op%i=version]", opcode);
//			result=kVstVersion;
			result=2400;
			// this isn't called for here... but very early NeedIdle calls might be missed
/*			if(effect!=NULL)
			{
				LogPrint("VSTi: calling effIdle in audioMasterVersion");
				effect->dispatcher(effect, DECLARE_VST_DEPRECATED(effIdle), 0, 0, 0, 0);
			}*/
			LogPrint("VSTi: audioMasterVersion returns");
			break;
		case audioMasterGetTime: // yikes, this is called a lot, try to avoid calculations if at all possible (use some position_changed flag from audiostream, or maybe even keep the whole struct there)
		{
//			LogPrint("VSTi: [op%i=time]", opcode);
/*			double cursample=(double)GetTick(1)*8.0;
			double playsample=(double)GetTick(2)*8.0;
			vst_timeinfo.samplePos=cursample; // ok
			vst_timeinfo.sampleRate=44100.0;
			vst_timeinfo.nanoSeconds=cursample*(1000000.0/44.1); // ok
			int samples_per_beat=32*320*GetTempo()/1600;
			vst_timeinfo.ppqPos=playsample/(double)samples_per_beat; // ok
			vst_timeinfo.tempo=(double)UpdateTempo(); // ok
			vst_timeinfo.barStartPos=floor(vst_timeinfo.ppqPos); // ok
			vst_timeinfo.cycleStartPos=0.0; // ? (what is cycle?)
			vst_timeinfo.cycleEndPos=1000000.0*16.0; // ? (what is cycle?)
			vst_timeinfo.timeSigNumerator=GetBeatLength(); // ok
			vst_timeinfo.timeSigDenominator=4; // ok
			double sample_to_subframe=30.0*80.0/44100.0;
			vst_timeinfo.smpteOffset=(int)(playsample*sample_to_subframe); // ok
			vst_timeinfo.smpteFrameRate=30; // constant 30 fps (arbitrary)
			vst_timeinfo.samplesToNextClock=(int)(cursample-(double)vst_timeinfo.smpteOffset/sample_to_subframe); // ok
			vst_timeinfo.flags=0; // ok?
			result=(VstIntPtr)&vst_timeinfo;*/
			result=(VstIntPtr)GetVstTimeInfo();
			break;
		}
		case audioMasterUpdateDisplay:
			if(effect!=NULL)
				effect->dispatcher(effect, effEditIdle, 0, 0, 0, 0);
			break;
		case audioMasterGetVendorString:
			LogPrint("VSTi: [op%i=vendor]", opcode);
			strcpy((char*)ptr, "DrPetter");
			result=1;
			break;
		case audioMasterGetProductString:
			LogPrint("VSTi: [op%i=product]", opcode);
			strcpy((char*)ptr, "musagi");
			result=1;
			break;
		case audioMasterGetVendorVersion:
			LogPrint("VSTi: [op%i=vendorversion]", opcode);
			result=100;
			break;
		case audioMasterCanDo:
		{
			LogPrint("VSTi: [op%i=cando]", opcode);
			char* str=(char*)ptr;
			if(0==strcmp(str, "sendVstEvents")
			|| 0==strcmp(str, "sendVstMidiEvent")
			|| 0==strcmp(str, "sendVstMidiEvents")
			|| 0==strcmp(str, "supplyIdle")
			|| 0==strcmp(str, "receiveVstEvents")
			|| 0==strcmp(str, "receiveVstMidiEvent")
			|| 0==strcmp(str, "receiveVstMidiEvents")
			|| 0==strcmp(str, "sendVstMidiEventFlagIsRealtime"))
//					strcmp(ptr, "sizeWindow")
		    {
				result=1;
			}
//			result=(VstIntPtr)host_cando;
			break;
		}
		case DECLARE_VST_DEPRECATED(audioMasterNeedIdle):
		{
//			OutputDebugString("musagi: audioMasterNeedIdle start");
			LogPrint("VSTi: [op%i=needidle, deprecated]", opcode);
			if(effect!=NULL)
			{
				VstInfo* vst_info=GVM_GetInfo(effect);
				if(vst_info!=NULL)
					vst_info->need_idle=true;
				else
					printf("VSTi: ###### couldn't set need_idle");
//				effect->dispatcher(effect, DECLARE_VST_DEPRECATED(effIdle), 0, 0, 0, 0);
//				effect->dispatcher(effect, effEditIdle, 0, 0, 0, 0);
			}
			else
				LogPrint("VSTi: ###### NeedIdle while NULL!");
//			OutputDebugString("musagi: audioMasterNeedIdle end");
			result=1;
			break;
		}
		case DECLARE_VST_DEPRECATED(audioMasterWantMidi):
			LogPrint("VSTi: [op%i=wantmidi, deprecated] (not sure what to do with it, ignoring...)", opcode);
			result=1;
			break;
		case 23:
		case 0:
			// suppress annoying messages
			break;
		default:
			debug_cb_msgcount++;
			if(debug_cb_msgcount<100)
				LogPrint("VSTi: ###### unhandled callback opcode %i", opcode);
			if(debug_cb_msgcount==100)
				LogPrint("VSTi: ###### [too many callbacks, won't log further]");
			break;
	}

	return result;
}

struct PluginLoader
{
	void* module;

	PluginLoader() : module(0)
	{}

	~PluginLoader()
	{
		LogPrint("VSTi: ~PluginLoader()");
		freePlugin();
	}

	void freePlugin()
	{
		if(module)
			FreeLibrary((HMODULE)module);
	}

	bool loadPlugin(const char* fileName)
	{
		module=LoadLibrary(fileName);
		return module!=0;
	}

	PluginEntryProc getMainEntry()
	{
		LogPrint("VSTi: PluginLoader->getMainEntry()");
		PluginEntryProc mainProc=0;
		mainProc=(PluginEntryProc)GetProcAddress((HMODULE)module, "VSTPluginMain");
		if(!mainProc)
			mainProc=(PluginEntryProc)GetProcAddress((HMODULE)module, "main");
		return mainProc;
	}
};

// Editor thread

struct MyDLGTEMPLATE : DLGTEMPLATE
{
	WORD ext[3];
	MyDLGTEMPLATE()
	{
		memset(this, 0, sizeof(*this));
	};
};

static INT_PTR CALLBACK EditorProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

//vsti_params* opening_params=NULL; // used for storing hwnd when editor window is opened

DWORD WINAPI editor_thread(LPVOID lpParam)
{
	vsti_params* params=(vsti_params*)lpParam;
//	AEffect* effect=(AEffect*)lpParam;

	params->editor_hwnd=NULL;
	
	// allow any other thread to finish its window opening procedure before we proceed
//	while(opening_params!=NULL) { LogPrint("VSTi: Thread waiting for mutex..."); Sleep(1); }
//	opening_params=params;

	LogPrint("VSTi: Thread editor opening");

	MyDLGTEMPLATE t;	
	t.style=WS_POPUPWINDOW|WS_DLGFRAME|DS_MODALFRAME/*|DS_CENTER*/;
	t.cx=100;
	t.cy=100;
//	DialogBoxIndirectParam(GetModuleHandle(0), &t, GetHwnd(), (DLGPROC)EditorProc, (LPARAM)effect); // popup steals focus from musagi
//	DialogBoxIndirectParam(GetModuleHandle(0), &t, 0, (DLGPROC)EditorProc, (LPARAM)params->vst_plugin); // plugin drops behind musagi when not in focus
	DialogBoxIndirectParam(GetModuleHandle(0), &t, 0, (DLGPROC)EditorProc, (LPARAM)params);

	// signal editor closed
	params->editor_open=false;
	params->editor_closed=true;

	LogPrint("VSTi: Thread editor closed");

	return 0;
}

bool StartEffectEditor(vsti_params* params)
{
	if((params->vst_plugin->flags&effFlagsHasEditor)==0)
	{
		LogPrint("VSTi: This plugin does not have an editor!");
		return false;
	}

//	theEffect = effect;

	params->editor_open=true;
	params->editor_closed=false;

	// create thread and call it with effect as parameter
	DWORD dwThreadId;
	HANDLE hThread=CreateThread(NULL,  			// default security attributes
        					0,                  // use default stack size  
				            editor_thread,    // thread function 
				            (LPVOID)params,    // argument to thread function 
				            0,                  // use default creation flags 
				            &dwThreadId);       // returns the thread identifier

	params->editor_handle=hThread;

//	CloseHandle(hMidiThread);

/*	MyDLGTEMPLATE t;	
	t.style=WS_POPUPWINDOW|WS_DLGFRAME|DS_MODALFRAME|DS_CENTER;
	t.cx=100;
	t.cy=100;
	DialogBoxIndirectParam(GetModuleHandle (0), &t, 0, (DLGPROC)EditorProc, (LPARAM)effect);
*/
//	theEffect = 0;

	return true;
}

struct Map_HWND_Params
{
	HWND hwnd;
	vsti_params* params;
};
Map_HWND_Params win_map[64];
bool win_map_initialized=false;

void InitWinMapping()
{
	for(int i=0;i<64;i++)
		win_map[i].hwnd=0;
	win_map_initialized=true;
}

vsti_params* GetWinMapping(HWND hwnd)
{
	if(hwnd==0)
		return NULL;
	for(int i=0;i<64;i++)
		if(win_map[i].hwnd==hwnd)
			return win_map[i].params;
	return NULL;
}

void AddWinMapping(HWND hwnd, vsti_params* params)
{
	for(int i=0;i<64;i++)
		if(win_map[i].hwnd==0)
		{
			win_map[i].hwnd=hwnd;
			win_map[i].params=params;
			return;
		}
	LogPrint("###ERROR### VSTi: HWND/params mapping full");
}

void RemoveWinMapping(HWND hwnd)
{
	for(int i=0;i<64;i++)
		if(win_map[i].hwnd==hwnd)
			win_map[i].hwnd=0;
}

HWND callback_mutex_owner=0;
int callback_calldepth=0;

INT_PTR CALLBACK EditorProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
//	AEffect* effect=(AEffect*)lParam; // safe assumption? (from DialogBoxIndirectParam above?)

/*	vsti_params* params=(vsti_params*)lParam;

	LogPrint("callback params ptr: %.8X", params);

	AEffect* effect=params->vst_plugin;
*/
//	LogPrintf("CB ");

	if(callback_mutex_owner==0 && callback_calldepth!=0)
		LogPrint("### MUTEX TERROR!");

	while(callback_mutex_owner!=0 && callback_mutex_owner!=hwnd) { LogPrint("mutex wait [hwnd=%.8X] (want msg=%.4X)", hwnd, msg); Sleep(1); }
	callback_mutex_owner=hwnd;
	callback_calldepth++;
	

//	LogPrintf("Callback init[hwnd=%.8X msg=%.4X ", hwnd, msg);

	if(!win_map_initialized)
		InitWinMapping();

	vsti_params* params=GetWinMapping(hwnd); // get stored pointer
	AEffect* effect=NULL;
	if(params!=NULL)
	{
		effect=params->vst_plugin;
//		LogPrintf("ok ptr: %.8X] ", params);
	}
//	else
//		LogPrintf("params=NULL] ");


//	LogPrint("callback params ptr: %.8X", params);

	switch(msg)
	{
		//-----------------------
		case WM_INITDIALOG:
		{
//			LogPrintf("[SWT] ");
			SetWindowText(hwnd, "VSTi Editor");
			SetTimer(hwnd, 1, 20, 0);

//			LogPrintf("[MAP] ");
			AddWinMapping(hwnd, (vsti_params*)lParam); // store pointer to relevant vsti_params
			vsti_params* params=GetWinMapping(hwnd); // get stored pointer again (now that it exists...)
			effect=params->vst_plugin; // ...and set proper effect pointer
//			LogPrintf("[new ptr: %.8X]\n", params);
			if(params==NULL)
				LogPrintf("[VERY NULL ERROR!!!]\n");

			if(effect)
			{
				LogPrint("VSTi: Open editor...");
				effect->dispatcher(effect, effEditOpen, 0, 0, hwnd, 0);

				LogPrint("VSTi: Get editor rect...");
				ERect* eRect=0;
				effect->dispatcher(effect, effEditGetRect, 0, 0, &eRect, 0);
				if(eRect)
				{
					int width=eRect->right-eRect->left;
					int height=eRect->bottom-eRect->top;
					if(width<100)
						width=100;
					if(height<100)
						height=100;

					RECT wRect;
					SetRect(&wRect, 0, 0, width, height);
					AdjustWindowRectEx(&wRect, GetWindowLong(hwnd, GWL_STYLE), FALSE, GetWindowLong(hwnd, GWL_EXSTYLE));
					width=wRect.right-wRect.left;
					height=wRect.bottom-wRect.top;

					SetWindowPos(hwnd, HWND_TOPMOST, 100, 100, width, height, 0/*SWP_NOMOVE*/);
				}
			}
			params->editor_hwnd=hwnd;
//			opening_params->editor_hwnd=hwnd;
//			opening_params=NULL; // window opening done, allow other editor windows to open

			ChildWindowActive(params->self, true);

			LogPrint("VSTi: Editor initialized");
			break;
		}

//		case WM_SETCURSOR:
//			LogPrintf("[SC] ");
//			break;

		case WM_ACTIVATE:
			switch(wParam)
			{
			case WA_ACTIVE:
			case WA_CLICKACTIVE:
//				LogPrintf("VSTi: WM_ACTIVATE (active)");
				ChildWindowActive(params->self, true);
				break;
			case WA_INACTIVE:
//				LogPrintf("VSTi: WM_ACTIVATE (inactive)");
				ChildWindowActive(params->self, false);
				break;
			}
			break;
/*
		case WM_KEYDOWN:
			LogPrintf("VSTi: WM_KEYDOWN");
			Keyboard_KeyDown(48);
			break;

		case WM_KEYUP:
			Keyboard_KeyUp(48);
			break;
*/
		case WM_TIMER:
//			LogPrintf("idle[");
			if(effect)
				effect->dispatcher(effect, effEditIdle, 0, 0, 0, 0);
//			LogPrintf("ok] ");
			break;

		case WM_CLOSE:
		{
			KillTimer(hwnd, 1);

			LogPrint("VSTi: Close editor...");
			if(effect)
				effect->dispatcher(effect, effEditClose, 0, 0, 0, 0);

			RemoveWinMapping(hwnd); // remove vsti_params pointer

			ChildWindowActive(params->self, false);

			EndDialog(hwnd, IDOK);
			break;
		}
	}

//	LogPrintf("rm ");

	callback_calldepth--;
	if(callback_calldepth<=0)
	{
		callback_calldepth=0;
		callback_mutex_owner=0;
	}

//	LogPrintf("*\n");

	return 0;
}

// -------------------- VST stuff

//int debug_stream_busy=0;

class vsti_channel : gear_ichannel
{
public:
	// parameters
	vsti_params *params;

	int curnote;
	int curvel;
	bool do_render;
	int pnprogram;
	int progage;
	int idle_tick;
	int num_idles;
	bool processing;

	vsti_channel()
	{
		curnote=0;
		curvel=0;
		params=NULL;
		do_render=false;
		pnprogram=0;
		progage=0;
		idle_tick=0;
		num_idles=0;
		processing=false;
	};

	~vsti_channel()
	{
	};

	int RenderBuffer(StereoBufferP *buffer, int size)
	{
		if(!do_render)
			return 0; // no output

		// rebound on pitch bend knob
		if(!params->pitchtouch)
		{
			params->pitch-=(params->pitch-0.5f)*0.08f;
			if(fabs(params->pitch-0.5f)<0.01f) params->pitch=0.5f;
		}

		processing=true;
		AEffect* vst_plugin=params->vst_plugin; // latch this in case it's NULLified by destroying the plugin while we compute

		if(vst_plugin==NULL)
		{
			processing=false;
			return 0; // no output
		}

		if(params->output_channels<1)
		{
			processing=false;
			return 0; // no output
		}

		// --- beyond this point we're in the single render channel and a plugin is loaded, with 1-16 output channels

//		if(debug_stream_busy==0)
//			debug_stream_busy=1;
/*
		if(params->vst_info!=NULL && params->vst_info->need_idle)
		{
			vst_plugin->dispatcher(vst_plugin, DECLARE_VST_DEPRECATED(effIdle), 0, 0, 0, 0);
//			vst_plugin->dispatcher(vst_plugin, DECLARE_VST_DEPRECATED(effIdle), 0, 0, 0, 0);
//			vst_plugin->dispatcher(vst_plugin, DECLARE_VST_DEPRECATED(effIdle), 0, 0, 0, 0);
		}


		// TODO: processEvents here, eating all queued midi events (or none)
//		VstEvents tevents;
//		tevents.numEvents=0;
//		tevents.reserved=0;
//		tevents.events[0]=NULL;
		DispatchEvents();
//		DispatchEvents(); // twice for good measure?
//		params->self->vstevents.numEvents=0;

//		Sleep(0); // allow plugin to actually process our events? this smells like powerfully bad performance...
*/

		DispatchEvents();

		float* pbuffers[32];
		pbuffers[0]=buffer->left;
		pbuffers[1]=buffer->right;
		for(int i=2;i<32;i++)
			pbuffers[i]=vsti_junkbuffer;

		// let vsti fill buffer
//		vst_plugin->processReplacing(vst_plugin, NULL, pbuffers, size); // no input buffers given
		if(vst_plugin->flags&effFlagsCanReplacing)
			vst_plugin->processReplacing(vst_plugin, pbuffers, pbuffers, size);
//			vst_plugin->processReplacing(vst_plugin, NULL, pbuffers, size);
		else
			vst_plugin->DECLARE_VST_DEPRECATED(process)(vst_plugin, pbuffers, pbuffers, size);

//		if(params->vst_info!=NULL && params->vst_info->need_idle)
//			vst_plugin->dispatcher(vst_plugin, DECLARE_VST_DEPRECATED(effIdle), 0, 0, 0, 0);
		EmptyEventQueue();

		// apply preamp
		float amp=(float)pow(params->preamp, 2.0f)*15;
		for(int i=0;i<size;i++)
			buffer->left[i]*=amp;
		if(params->output_channels==2) // only do right side if it's a stereo instrument
			for(int i=0;i<size;i++)
				buffer->right[i]*=amp;

		// some calls don't need to occur on every single buffer
		idle_tick++;
		if(idle_tick>15)
			idle_tick=0;

		// handle program change		
		if(params->program!=params->curprogram)
		{
			if(params->program==pnprogram)
				progage++;
			else
				progage=0;
			pnprogram=params->program;
			if(progage>(44100/128)/5) // 1/5 sec has passed since we last changed VST program, so set the new one
			{
				LogPrint("VSTi: Program change - ReleaseAll()");
				((gear_instrument*)params->self)->ReleaseAll(true);
				params->curprogram=params->program;
				LogPrint("VSTi: Program change - dispatcher");
				vst_plugin->dispatcher(vst_plugin, effSetProgram, 0, params->curprogram, 0, 0);
				LogPrint("VSTi: Program change - done");
			}
		}
		else if(/*params->editor_open && */idle_tick==0) // if it didn't change, check periodically with plugin to see if user changed it through the editor
		{
			int actual_program=vst_plugin->dispatcher(vst_plugin, effGetProgram, 0, 0, 0, 0);
			if(params->program!=actual_program) // we've been misled, correct that
			{
				LogPrint("VSTi: Editor program change - update knob");
				params->program=actual_program;
				params->curprogram=params->program;
				params->fprogram=(float)params->program/(params->numprograms-1);
			}
		}
		
		// handle pitch bend change
		if(params->pitch!=params->prev_pitch)
		{
			LogPrint("VSTi: pitch change");
			params->prev_pitch=params->pitch;
			int ipitch=(int)((params->pitch-0.5f)*0x1FFF)+0x2000;
			SendVSTMidiEvent(0xE0, ((unsigned char)ipitch)&0x7F, ((unsigned char)(ipitch>>7))&0x7F);
			LogPrint("VSTi: pitch change - done");
		}
		// handle mod wheel change
		if(params->mod!=params->prev_mod)
		{
			LogPrint("VSTi: Mod change");
			params->prev_mod=params->mod;
			SendVSTMidiEvent(0xB0, 1, (unsigned char)(params->mod*127));
			LogPrint("VSTi: Mod change - done");
		}

		// handle proper closing of the editor thread
		if(params->editor_closed)
		{
			LogPrint("VSTi: CloseHandle()");
			CloseHandle(params->editor_handle);
			params->editor_closed=false;
			LogPrint("VSTi: CloseHandle() done");
		}

//		LogPrintf("$");

		if(params->vst_info!=NULL && params->vst_info->need_idle && idle_tick==0) // send idle messages every now and then if the plugin needs them
		{
			if(num_idles<30)
				LogPrintf("VSTi: Dispatch idle... ");
			vst_plugin->dispatcher(vst_plugin, DECLARE_VST_DEPRECATED(effIdle), 0, 0, 0, 0);
			if(num_idles<30)
				LogPrintf("ok\n");
			num_idles++;
		}

		processing=false;

		if(params->output_channels==1)
			return 1; // mono output
		return 2; // assume stereo output (additional channels are discarded)
	};

	void DispatchEvents();

	void EmptyEventQueue();

	void SendVSTMidiEvent(unsigned char b1, unsigned char b2, unsigned char b3);

	void Trigger(int tnote, float tvolume)
	{
		LogPrintf("VSTi: Trigger... ");

		LogPrintf("ptr=%.8X ", params->vst_plugin);

		active=true;

		curnote=tnote+params->octave*12;
		if(curnote<0) curnote=0;
		if(curnote>127) curnote=127;
		curvel=(int)(tvolume*params->vol*127);
		SendVSTMidiEvent(0x90, (unsigned char)curnote, (unsigned char)curvel);

		LogPrintf("ok\n");
	};
	
	void Release()
	{
		LogPrintf("VSTi: Release... ");

		LogPrintf("ptr=%.8X ", params->vst_plugin);

		active=false;
		SendVSTMidiEvent(0x80, (unsigned char)curnote, (unsigned char)curvel);

		LogPrintf("ok\n");
	};

	void StopChannels()
	{
		LogPrintf("VSTi: ### StopChannels ");
		SendVSTMidiEvent(123, 0, 0); // all notes off
		SendVSTMidiEvent(120, 0, 0); // all sound off

		LogPrintf("ok\n");
	};

	void Slide(int tnote, int length)
	{
	};
};

class gin_vsti : public gear_instrument
{
private:
	vsti_params params;
	PluginLoader* loader;
	HANDLE hEditorThread;
	char** prognames;
	char queued_filename[512];
	VstInfo vst_info;
	VstEvents* vst_events[2]; // double-buffered to reduce thread problems
	int vst_curevents;
	bool busy_loading;
	bool busy_adding_event;

	bool LoadPlugin(char* filename)
	{
		LogPrint("VSTi: Loading plugin \"%s\"", filename);
		// init VST plugin
		params.vst_plugin=NULL;
//		params.dll_path[0]='\0';
//		params.dll_filename[0]='\0';
		if(!loader->loadPlugin(filename))
		{
			LogPrint("VSTi: Failed to load VST Plugin library!");
			return false;
		}
		PluginEntryProc mainEntry=loader->getMainEntry();
		if(!mainEntry)
		{
			LogPrint("VSTi: VST Plugin main entry not found!");
			return false;
		}
		LogPrint("VSTi: Create effect...");
		AEffect* effect=mainEntry(HostCallback);
		if(!effect)
		{
			LogPrint("VSTi: Failed to create effect instance!");
			return false;
		}
		
		GVM_AddInstrument(effect, &vst_info);

		effect->dispatcher(effect, effCanDo, 0, 0, "bypass", 0);

/*
struct VstSpeakerArrangement
{	
	VstInt32 type;						///< e.g. #kSpeakerArr51 for 5.1  @see VstSpeakerArrangementType
	VstInt32 numChannels;				///< number of channels in this speaker arrangement
	VstSpeakerProperties speakers[8];	///< variable sized speaker array
};
struct VstSpeakerProperties
{
	float azimuth;		///< unit: rad, range: -PI...PI, exception: 10.f for LFE channel
	float elevation;	///< unit: rad, range: -PI/2...PI/2, exception: 10.f for LFE channel
	float radius;		///< unit: meter, exception: 0.f for LFE channel
	float reserved;		///< zero (reserved for future use)
	char name[kVstMaxNameLen];	///< for new setups, new names should be given (L/R/C... won't do)
	VstInt32 type;		///< @see VstSpeakerType
	char future[28];	///< reserved for future use
};
	kSpeakerL,						///< Left (L)
	kSpeakerR,						///< Right (R)
*/

		VstSpeakerArrangement sparr;
		sparr.type=kSpeakerArrStereo;
		sparr.numChannels=2;
		sparr.speakers[0].azimuth=0.0f;
		sparr.speakers[0].elevation=0.0f;
		sparr.speakers[0].radius=0.0f;
		sparr.speakers[0].reserved=0.0f;
		strcpy(sparr.speakers[0].name, "Left");
		sparr.speakers[0].type=kSpeakerL;
		sparr.speakers[1].azimuth=0.0f;
		sparr.speakers[1].elevation=0.0f;
		sparr.speakers[1].radius=0.0f;
		sparr.speakers[1].reserved=0.0f;
		strcpy(sparr.speakers[1].name, "Right");
		sparr.speakers[1].type=kSpeakerR;
		effect->dispatcher(effect, effGetSpeakerArrangement, 0, (VstIntPtr)&sparr, &sparr, 0);

//		effect->dispatcher(effect, effAutomate, 0, 0, 0, 0);
		effect->dispatcher(effect, effOpen, 0, 0, 0, 0);
		effect->dispatcher(effect, effSetSampleRate, 0, 0, 0, 44100.0f); // should be read from audiostream
		effect->dispatcher(effect, effSetBlockSize, 0, 128, 0, 0); // should be read from audiostream
		
		LogPrint("VSTi: relevant plugin capabilities:");
		if(effect->dispatcher(effect, effCanDo, 0, 0, "receiveVstMidiEvent", 0)==1)
			LogPrint("VSTi:   can receiveVstMidiEvent");
		else
			LogPrint("VSTi: can't receiveVstMidiEvent");
/*		if(effect->dispatcher(effect, effCanDo, 0, 0, "receiveVstEvents", 0)==1)
			LogPrint("VSTi:   can receiveVstEvents");
		else
			LogPrint("VSTi: can't receiveVstEvents (### this is a problem)");*/


		effect->dispatcher(effect, effMainsChanged, 0, 1, 0, 0); // resume
/*		effect->dispatcher(effect, effMainsChanged, 0, 0, 0, 0); // resume
		effect->dispatcher(effect, effSetBlockSize, 0, 128, 0, 0); // should be read from audiostream
		effect->dispatcher(effect, effMainsChanged, 0, 1, 0, 0); // resume

		char tempstring[256];
		effect->dispatcher(effect, effGetPlugCategory, 0, 0, 0, 0);
		effect->dispatcher(effect, effGetProductString, 0, 0, tempstring, 0);
		effect->dispatcher(effect, effGetProgramName, 0, 0, tempstring, 0);
		effect->dispatcher(effect, effGetProgram, 0, 0, 0, 0);
		effect->dispatcher(effect, effGetPlugCategory, 0, 0, 0, 0);
		effect->dispatcher(effect, effGetProgramName, 0, 0, tempstring, 0);
		effect->dispatcher(effect, effGetProgram, 0, 0, 0, 0);
		effect->dispatcher(effect, effStartProcess, 0, 0, 0, 0);
		effect->dispatcher(effect, effGetProgramName, 0, 0, tempstring, 0);
		effect->dispatcher(effect, effGetProgram, 0, 0, 0, 0);
*/
//		strcpy(params.plugin_name, "some name");


		effect->dispatcher(effect, effGetEffectName, 0, 0, params.plugin_name, 0);

		params.output_channels=effect->numOutputs;
		LogPrint("VSTi: Plugin channels: %i in, %i out", effect->numInputs, effect->numOutputs);
		if(params.output_channels>32)
		{
			char mstring[128];
			sprintf(mstring, "Can't handle %i output channels", params.output_channels);
			MessageBox(hWndMain, mstring, "Warning", MB_ICONEXCLAMATION);
			return false;
		}

		// effect->numPrograms, effect->numParams, effect->numInputs, effect->numOutputs
		// Iterate programs...
		params.numprograms=effect->numPrograms;
		prognames=(char**)malloc(params.numprograms*sizeof(char*));
		for(VstInt32 progIndex=0;progIndex<effect->numPrograms;progIndex++)
		{
			prognames[progIndex]=(char*)malloc(256*sizeof(char));
			prognames[progIndex][0]='\0';
			if(!effect->dispatcher(effect, effGetProgramNameIndexed, progIndex, 0, prognames[progIndex], 0))
			{
				effect->dispatcher(effect, effSetProgram, 0, progIndex, 0, 0);
				effect->dispatcher(effect, effGetProgramName, 0, 0, prognames[progIndex], 0);
			}
		}
		effect->dispatcher(effect, effSetProgram, 0, 0, 0, 0);
//		prognames[0][3]='\0';
		
//		effect->dispatcher(effect, effMainsChanged, 0, 1, 0, 0); // resume
//		effect->dispatcher(effect, effStartProcess, 0, 0, 0, 0); // start audio processing

		// store dll path
		strcpy(params.dll_path, filename);
		LogPrint("VSTi: dll_path=\"%s\"", params.dll_path);
		// store dll name
		int sl=strlen(filename);
		int namepos=0;
		for(int i=0;i<sl;i++)
			if(filename[i]=='\\')
				namepos=i+1;
		for(int i=namepos;i<=sl;i++)
			params.dll_filename[i-namepos]=filename[i];
		LogPrint("VSTi: dll_filename=\"%s\"", params.dll_filename);

		params.vst_plugin=effect;

		LogPrint("VSTi: Plugin \"%s\" successfully loaded!", params.plugin_name);

		return true;
	};

	void ResetParams()
	{
		params.vol=1.0f;
		params.pitch=0.5f;
		params.prev_pitch=params.pitch;
		params.pitchtouch=false;
		params.mod=0.0f;
		params.prev_mod=params.mod;
		params.preamp=0.5f;
		params.fprogram=0.0f;
		params.program=0;
		params.curprogram=0;
		params.numprograms=0;
		params.foctave=0.5f;
		params.octave=0;
		params.output_channels=2;
		params.vst_plugin=NULL;
		params.vst_info=&vst_info;
		params.editor_open=false;
		params.editor_closed=false;
		params.self=this;

		vst_info.need_idle=false;
	};

	void DestroyPlugin()
	{
		LogPrint("VSTi: DestroyPlugin()");
		if(params.vst_plugin!=NULL)
		{
			if(params.editor_open)
			{
				LogPrint("VSTi: Close window...");
				SendMessage(params.editor_hwnd, WM_CLOSE, 0, 0);
				for(int i=0;i<20;i++)
				{
					if(!params.editor_open)
						break;
					Sleep(10);
				}
				if(params.editor_open)
					LogPrint("VSTi: [Warning] Editor window didn't close properly");
				// if(params.editor_open) CloseThread(params->editor_thread);
				CloseHandle(params.editor_handle);
				params.editor_closed=false;
			}
			LogPrint("VSTi: Close effect...");
			AEffect* plugin=params.vst_plugin;
			params.vst_plugin=NULL; // NULLify this first to prevent stream from borking things during destruction
			while(((vsti_channel*)channels[0])->processing) // wait for stream to "get it"
			{
				Sleep(0);
			}
//			Sleep(200);
			plugin->dispatcher(plugin, effMainsChanged, 0, 0, 0, 0); // stop
			plugin->dispatcher(plugin, effStopProcess, 0, 0, 0, 0); // stop audio processing
			plugin->dispatcher(plugin, effClose, 0, 0, 0, 0);
			loader->freePlugin();
			GVM_RemoveInstrument(plugin);
		}
		ResetParams();
		LogPrint("VSTi: Plugin destroyed");
	}

public:
	gin_vsti()
	{
		strcpy(name, "VSTi");
		strcpy(visname, name);

		LogPrint("VSTi: Init...");
		loader=new PluginLoader();

		for(int ei=0;ei<2;ei++)
		{
			vst_events[ei]=(VstEvents*)malloc(sizeof(VstEvents)+64*sizeof(VstEvent*)); // a few bytes overkill, but that's ok
			for(int i=0;i<64;i++)
				vst_events[ei]->events[i]=(VstEvent*)malloc(sizeof(VstEvent));
			vst_events[ei]->reserved=0;
			vst_events[ei]->numEvents=0;
		}
		vst_curevents=0;
		busy_adding_event=false;

		prognames=NULL;

		ResetParams();

		params.dll_path=(char*)malloc(512);
		params.dll_filename=(char*)malloc(128);
		params.plugin_name=(char*)malloc(128);
		
//		params.debug_stream_busy=0;

		queued_filename[0]='\0';
		busy_loading=false;

/*
VstTimeInfo
{
	double samplePos;				///< current Position in audio samples (always valid)
	double sampleRate;				///< current Sample Rate in Herz (always valid)
	double nanoSeconds;				///< System Time in nanoseconds (10^-9 second)
	double ppqPos;					///< Musical Position, in Quarter Note (1.0 equals 1 Quarter Note)
	double tempo;					///< current Tempo in BPM (Beats Per Minute)
	double barStartPos;				///< last Bar Start Position, in Quarter Note
	double cycleStartPos;			///< Cycle Start (left locator), in Quarter Note
	double cycleEndPos;				///< Cycle End (right locator), in Quarter Note
	VstInt32 timeSigNumerator;		///< Time Signature Numerator (e.g. 3 for 3/4)
	VstInt32 timeSigDenominator;	///< Time Signature Denominator (e.g. 4 for 3/4)
	VstInt32 smpteOffset;			///< SMPTE offset (in SMPTE subframes (bits; 1/80 of a frame)). The current SMPTE position can be calculated using #samplePos, #sampleRate, and #smpteFrameRate.
	VstInt32 smpteFrameRate;		///< @see VstSmpteFrameRate
	VstInt32 samplesToNextClock;	///< MIDI Clock Resolution (24 Per Quarter Note), can be negative (nearest clock)
	VstInt32 flags;					///< @see VstTimeInfoFlags
};
*/

//		memset(&vst_timeinfo, 0, sizeof(vst_timeinfo)); // uh oh, bad idea
/*		vst_timeinfo.samplePos=0.0f; // REQUEST: from audiostream
		vst_timeinfo.sampleRate=44100.0;
		vst_timeinfo.nanoSeconds=0.0f; // from samplePos
		vst_timeinfo.ppqPos=0.0f; // REQUEST: from samplepos_play
		vst_timeinfo.tempo=120.0f; // REQUEST: ok
		vst_timeinfo.barStartPos=0; // ?
		vst_timeinfo.cycleStartPos=0; // ?
		vst_timeinfo.cycleEndPos=0; // ?
		vst_timeinfo.timeSigNumerator=4; // assuming 4/4 (should probably read from global setting)
		vst_timeinfo.timeSigDenominator=4; // assuming 4/4 (should probably read from global setting)
		vst_timeinfo.smpteOffset=0.0f; // REQUEST: from samplepos_play
		vst_timeinfo.smpteFrameRate=0.0f; // constant (based on 44100 Hz)
		vst_timeinfo.samplesToNextClock=0.0f; // constant (based on 44100 Hz)
		vst_timeinfo.flags=0; // ?
*/

		numchannels=16;
		for(int i=0;i<numchannels;i++)
		{
			channels[i]=(gear_ichannel*)(new vsti_channel());
			((vsti_channel*)channels[i])->params=&params;
		}
		((vsti_channel*)channels[0])->do_render=true; // only use one channel for rendering since the VSTi plugin does internal channel mixing

		LogPrint("VSTi: Init done!");

		LogPrint("init params ptr: %.8X", &params);
		
		LogPrint("--- AudioMasterOpcodes ---");
		LogPrint("opcode %i: audioMasterAutomate", audioMasterAutomate);
		LogPrint("opcode %i: audioMasterVersion", audioMasterVersion);
		LogPrint("opcode %i: audioMasterCurrentId", audioMasterCurrentId);
		LogPrint("opcode %i: audioMasterIdle", audioMasterIdle);
		LogPrint("--- AudioMasterOpcodesX ---");
		LogPrint("opcode %i: audioMasterProcessEvents", audioMasterProcessEvents);
		LogPrint("opcode %i: audioMasterIOChanged", audioMasterIOChanged);
		LogPrint("opcode %i: audioMasterGetSampleRate", audioMasterGetSampleRate);
		LogPrint("opcode %i: audioMasterGetBlockSize", audioMasterGetBlockSize);
		LogPrint("opcode %i: audioMasterGetInputLatency", audioMasterGetInputLatency);
		LogPrint("opcode %i: audioMasterGetOutputLatency", audioMasterGetOutputLatency);
		LogPrint("opcode %i: audioMasterGetCurrentProcessLevel", audioMasterGetCurrentProcessLevel);
		LogPrint("opcode %i: audioMasterGetAutomationState", audioMasterGetAutomationState);
		LogPrint("opcode %i: audioMasterOfflineStart", audioMasterOfflineStart);
		LogPrint("opcode %i: audioMasterOfflineRead", audioMasterOfflineRead);
		LogPrint("opcode %i: audioMasterOfflineWrite", audioMasterOfflineWrite);
		LogPrint("opcode %i: audioMasterOfflineGetCurrentPass", audioMasterOfflineGetCurrentPass);
		LogPrint("opcode %i: audioMasterOfflineGetCurrentMetaPass", audioMasterOfflineGetCurrentMetaPass);
		LogPrint("opcode %i: audioMasterGetVendorString", audioMasterGetVendorString);
		LogPrint("opcode %i: audioMasterGetProductString", audioMasterGetProductString);
		LogPrint("opcode %i: audioMasterGetVendorVersion", audioMasterGetVendorVersion);
		LogPrint("opcode %i: audioMasterVendorSpecific", audioMasterVendorSpecific);
		LogPrint("opcode %i: audioMasterCanDo", audioMasterCanDo);
		LogPrint("opcode %i: audioMasterGetLanguage", audioMasterGetLanguage);
		LogPrint("opcode %i: audioMasterGetDirectory", audioMasterGetDirectory);
		LogPrint("opcode %i: audioMasterUpdateDisplay", audioMasterUpdateDisplay);
		LogPrint("opcode %i: audioMasterBeginEdit", audioMasterBeginEdit);
		LogPrint("opcode %i: audioMasterEndEdit", audioMasterEndEdit);
		LogPrint("opcode %i: audioMasterOpenFileSelector", audioMasterOpenFileSelector);
		LogPrint("opcode %i: audioMasterCloseFileSelector", audioMasterCloseFileSelector);
		LogPrint("--- AudioMasterOpcodesX (deprecated) ---");
		LogPrint("opcode %i: audioMasterWantMidi", DECLARE_VST_DEPRECATED(audioMasterWantMidi));
		LogPrint("opcode %i: audioMasterSetTime", DECLARE_VST_DEPRECATED(audioMasterSetTime));
		LogPrint("opcode %i: audioMasterTempoAt", DECLARE_VST_DEPRECATED(audioMasterTempoAt));
		LogPrint("opcode %i: audioMasterGetNumAutomatableParameters", DECLARE_VST_DEPRECATED(audioMasterGetNumAutomatableParameters));
		LogPrint("opcode %i: audioMasterGetParameterQuantization", DECLARE_VST_DEPRECATED(audioMasterGetParameterQuantization));
		LogPrint("opcode %i: audioMasterNeedIdle", DECLARE_VST_DEPRECATED(audioMasterNeedIdle));
		LogPrint("opcode %i: audioMasterGetPreviousPlug", DECLARE_VST_DEPRECATED(audioMasterGetPreviousPlug));
		LogPrint("opcode %i: audioMasterGetNextPlug", DECLARE_VST_DEPRECATED(audioMasterGetNextPlug));
		LogPrint("opcode %i: audioMasterWillReplaceOrAccumulate", DECLARE_VST_DEPRECATED(audioMasterWillReplaceOrAccumulate));
		LogPrint("opcode %i: audioMasterSetOutputSampleRate", DECLARE_VST_DEPRECATED(audioMasterSetOutputSampleRate));
		LogPrint("opcode %i: audioMasterGetOutputSpeakerArrangement", DECLARE_VST_DEPRECATED(audioMasterGetOutputSpeakerArrangement));
		LogPrint("opcode %i: audioMasterSetIcon", DECLARE_VST_DEPRECATED(audioMasterSetIcon));
		LogPrint("opcode %i: audioMasterOpenWindow", DECLARE_VST_DEPRECATED(audioMasterOpenWindow));
		LogPrint("opcode %i: audioMasterCloseWindow", DECLARE_VST_DEPRECATED(audioMasterCloseWindow));
		LogPrint("opcode %i: audioMasterEditFile", DECLARE_VST_DEPRECATED(audioMasterEditFile));
		LogPrint("opcode %i: audioMasterGetChunkFile", DECLARE_VST_DEPRECATED(audioMasterGetChunkFile));
		LogPrint("opcode %i: audioMasterGetInputSpeakerArrangement", DECLARE_VST_DEPRECATED(audioMasterGetInputSpeakerArrangement));
	};

	~gin_vsti()
	{
		LogPrint("VSTi: ~gin_vsti()");
		DestroyPlugin();
		for(int i=0;i<params.numprograms;i++)
			free(prognames[i]);
		free(prognames);
		delete loader;
		for(int ei=0;ei<2;ei++)
		{
			for(int i=0;i<64;i++)
				free(vst_events[ei]->events[i]);
			free(vst_events[ei]);
		}
		free(params.dll_path);
		free(params.dll_filename);
		free(params.plugin_name);
		LogPrint("VSTi: *gone*");
		// CRASH/TODO: crashes here sometimes... maybe some thread lingers?
	};

	void VEQ_AddMidiEvent(char b1, char b2, char b3)
	{
		char message[4];
		message[0]=b1;
		message[1]=b2;
		message[2]=b3;
		message[3]=0;

		busy_adding_event=true;

		VstEvent* vst_event=vst_events[vst_curevents]->events[vst_events[vst_curevents]->numEvents++];

		vst_event->type=kVstMidiType;
//		vst_event->byteSize=sizeof(VstMidiEvent);
		vst_event->byteSize=24;
		vst_event->deltaFrames=0; // trigger event at start of next buffer
		vst_event->flags=kVstMidiEventIsRealtime;
		VstMidiEvent* midi_event=(VstMidiEvent*)vst_event;
		midi_event->noteLength=0;
		midi_event->noteOffset=0;
		midi_event->detune=0;
//		if(b1==0x80)
//			midi_event->noteOffVelocity=127;
//		else
			midi_event->noteOffVelocity=0;
		midi_event->reserved1=0;
		midi_event->reserved2=0;
		for(int i=0;i<4;i++)
			midi_event->midiData[i]=(char)message[i];
//		VstEvents tevents;
//		vst_events.numEvents=1;
//		vst_events.reserved=0;
//		vst_events.events[0]=&vst_event;

		busy_adding_event=false;
	};
	
	void VEQ_EmptyQueue()
	{
		// not needed...
//		vst_events[vst_curevents]->numEvents=0;
	};

	VstEvents* VEQ_GetEvents()
	{
		while(busy_adding_event) // thread lock
		{
			Sleep(0);
		}
		vst_curevents=1-vst_curevents; // swap buffers
		vst_events[vst_curevents]->numEvents=0; // clear old buffer so we can fill it with new events
		return vst_events[1-vst_curevents]; // return new buffer with latest events in it
	};

	void DoInterface(DUI *dui)
	{
/*		if(queued_filename[0]!='\0' && !busy_loading) // chose to load a plugin last frame
		{
			busy_loading=true;
			DestroyPlugin(); // destroy previous plugin (if any)
			if(!LoadPlugin(queued_filename))
				MessageBox(hWndMain, "Couldn't load VSTi plugin", "Warning", MB_ICONEXCLAMATION);
			queued_filename[0]='\0';
			busy_loading=false;
		}
*/		
		dui->DrawBar(1, 0, 400, 5, dui->palette[4]);
		dui->DrawBar(1, 45, 400, 5, dui->palette[4]);
		dui->DrawBar(1, 5, 400, 40, dui->palette[7]);
		dui->DrawBox(0, 1, 400, 49, dui->palette[6]);

		dui->DrawText(372, 36, dui->palette[6], "VSTi");
		dui->DrawText(372, 35, dui->palette[9], "VSTi");

		dui->DrawText(5, 10, dui->palette[6], "plugin:");
		if(dui->DoButton(45, 9, "btn_vstip", "select VSTi plugin") && !busy_loading)
		{
			busy_loading=true;
			char filename[256];
			FinishRender(); // glFlush, drop this partial frame
			if(dui->SelectFileLoad(filename, 5))
			{
//				strcpy(queued_filename, filename); // will load on next frame (and display pretty message)

				DestroyPlugin(); // destroy previous plugin (if any)
//				if(!LoadPlugin(queued_filename))
				if(!LoadPlugin(filename))
					MessageBox(hWndMain, "Couldn't load VSTi plugin", "Warning", MB_ICONEXCLAMATION);
//				queued_filename[0]='\0';
			}
			busy_loading=false;
			AbortRender(); // rendering will be handled through WM_PAINT while we're selecting a file, so this frame is scrapped
			return;
		}
		if(busy_loading)
			dui->DrawText(70, 10, dui->palette[9], "loading...");
		else if(params.vst_plugin==NULL)
			dui->DrawText(70, 10, dui->palette[9], "[none loaded]");
		else
			dui->DrawText(70, 10, dui->palette[9], params.plugin_name);

		dui->DrawText(331+4, 10, dui->palette[6], "preamp:");
		dui->DoKnob(331+46, 5, params.preamp, -1, "knob_pamp", "amplification before effects");

//		dui->DrawText(290+5, 20, dui->palette[6], "interface");
//		if(dui->DoButton(290+56, 19, "btn_vstgui", "toggle showing VSTi interface/editor"))
		dui->DrawText(250+5, 10, dui->palette[6], "interface:");
		if(dui->DoButton(250+59, 9, "btn_vstgui", "toggle showing VSTi interface/editor"))
		{
			if(params.vst_plugin!=NULL)
			{
				if(!params.editor_open)
					StartEffectEditor(&params); // returns bool, maybe check value...
				else
					SendMessage(params.editor_hwnd, WM_CLOSE, 0, 0);
			}
		}

		dui->DrawText(24, 22, dui->palette[6], "pitch");
		params.pitchtouch=dui->DoKnob(5, 20, params.pitch, -1, "knob_pitch", "pitch bend");

		dui->DrawText(35, 36, dui->palette[6], "mod");
		dui->DoKnob(18, 32, params.mod, -1, "knob_mod", "modulation wheel");
//		dui->DrawText(44, 30, dui->palette[10], "%i%%", (int)(params.vol*100));

		dui->DrawText(61, 30, dui->palette[6], "vel:");
		dui->DoKnob(61+18/*70*/, 25, params.vol, -1, "knob_vel", "velocity scale");

		dui->DrawText(75+24, 30, dui->palette[6], "oct:");
		dui->DoKnob(75+44, 25, params.foctave, -1, "knob_oct", "octave offset");
		params.octave=(int)((params.foctave-0.5f)*6);
		if(params.octave==0)
			dui->DrawText(75+64, 30, dui->palette[10], " 0");
		if(params.octave>0)
			dui->DrawText(75+64, 30, dui->palette[10], "+%i", params.octave);
		if(params.octave<0)
			dui->DrawText(75+64, 30, dui->palette[10], "%i", params.octave);

		dui->DrawText(158, 30, dui->palette[6], "program:");
		dui->DoKnob(158+47, 25, params.fprogram, -1, "knob_pr", "program/preset");
		params.program=(int)(params.fprogram*(params.numprograms-1));
		if(params.program<0) params.program=0;
		if(params.numprograms>0)
			dui->DrawText(158+68, 30, dui->palette[9], "%.3i %s", params.program+1, prognames[params.program]);
		else
			dui->DrawText(158+68, 30, dui->palette[9], "%.3i", params.program+1);


/*
		params.debug_stream_busy++;
		LogPrint("sb[%.8X]:%i", &params, params.debug_stream_busy);
		if(params.debug_stream_busy==100)
			LogPrint("VSTi: stream frozen [%.8X]", &params);*/
	};

	bool Save(FILE *file)
	{
		kfwrite2(name, 64, 1, file);
		// get size of program chunk
		VstInt32 chunksize=0;
		void* chunkdata=NULL;
		if(params.vst_plugin!=NULL)
			chunksize=params.vst_plugin->dispatcher(params.vst_plugin, effGetChunk, 1, 0, &chunkdata, 0);
		int numbytes=sizeof(vsti_params)+64+sizeof(VstInt32)+chunksize;
		kfwrite2(&numbytes, 1, sizeof(int), file);
		kfwrite2(visname, 64, 1, file);
		kfwrite(&params, 1, sizeof(vsti_params)-4, file);
		kfwrite2(params.dll_path, 512, 1, file);
		kfwrite2(params.dll_filename, 128, 1, file);
		// save program chunk
		LogPrint("VSTi: saving program chunk...");
		kfwrite2(&chunksize, 1, sizeof(VstInt32), file);
		LogPrint("VSTi: %i bytes of data", chunksize);
		if(chunkdata==NULL || chunksize==0)
			LogPrint("VSTi: effGetChunk failed, can't save program");
		else
			kfwrite2(chunkdata, chunksize, 1, file);

/*		// save all vst parameters
		int numparams=params.vst_plugin->numParams;
		kfwrite2(&numparams, 1, sizeof(int), file);
		for(VstInt32 i=0;i<numparams;i++)
		{
			float value=params.vst_plugin->getParameter(params.vst_plugin, i);
			kfwrite2(&value, 1, sizeof(float), file);
		}*/
		return true;
	};

	bool Load(FILE *file)
	{
//		LogPrint("VSTi: Load()");
		int numbytes=0;
		kfread2(&numbytes, 1, sizeof(int), file);
//		LogPrint("VSTi: numbytes=%i", numbytes);
		kfread2(visname, 64, 1, file);
//		LogPrint("VSTi: visname=\"%s\"", visname);
		// store string pointers before overwriting them from file
		char* dll_path=params.dll_path;
		char* dll_filename=params.dll_filename;
		char* plugin_name=params.plugin_name;
		kfread2(&params, 1, sizeof(vsti_params)-4, file);
		// restore string pointers again
		params.dll_path=dll_path;
		params.dll_filename=dll_filename;
		params.plugin_name=plugin_name;
		// restore vst_info pointer
		params.vst_info=&vst_info;
//		LogPrint("VSTi: paths...");
		kfread2(params.dll_path, 512, 1, file);
		kfread2(params.dll_filename, 128, 1, file);
		params.self=this;
//		LogPrint("VSTi: Restore");
		// clear vst pointer and editor info, destroy prev_mod to force update
		params.prev_mod=-1.0f;
		params.curprogram=0;
		params.vst_plugin=NULL;
		params.editor_open=false;
		params.editor_closed=false;
		// load plugin (1. full path  2. last used vst folder  3. registry vst folder  4. file requester  5. NULL)
//		LogPrint("VSTi: Load plugin");
		bool manual_cancel=false;
		if(!LoadPlugin(params.dll_path))
		{
			char filename[512];
			strcpy(filename, GetCurDir(5));
//			strcpy(filename, GetMusagiDir());
			strcat(filename, "\\");
			strcat(filename, params.dll_filename);
			if(!LoadPlugin(filename))
			{
				// should look up system vst dir from registry here before asking user...
				char string[128];
				sprintf(string, "Find %s", params.dll_filename);
				if(GetDUI()->SelectFileLoad(filename, 5, string))
				{
					if(strcmp(filename, params.dll_filename)!=-1) // only load if filename matches the expected plugin
						LoadPlugin(filename);
				}
				else
					manual_cancel=true;
			}
		}
		if(params.vst_plugin==NULL)
		{
			ResetParams();

			if(!manual_cancel)
			{
				char mstring[128];
				sprintf(mstring, "Couldn't load VSTi plugin: \"%s\"", params.dll_filename);
				MessageBox(hWndMain, mstring, "Warning", MB_ICONEXCLAMATION);
			}
			LogPrint("VSTi: Couldn't load \"%s\"", params.dll_filename);
		}
		else
		{
			// set correct program before loading preset to avoid override
			params.curprogram=params.program;
			params.vst_plugin->dispatcher(params.vst_plugin, effSetProgram, 0, params.curprogram, 0, 0);
		}

		if(numbytes==sizeof(vsti_params)+64)
		{
			// this was an old version of the instrument without saved program chunk, so stop reading
			return true;
		}

		// load program chunk
		LogPrint("VSTi: loading program chunk...");
		VstInt32 chunksize=0;
		void* chunkdata=NULL;
		kfread2(&chunksize, 1, sizeof(VstInt32), file);
		LogPrint("VSTi: %i bytes of data", chunksize);
		if(chunksize>0)
		{
			char* chunkdata=(char*)malloc(chunksize);
			kfread2(chunkdata, chunksize, 1, file);
			if(params.vst_plugin!=NULL)
				params.vst_plugin->dispatcher(params.vst_plugin, effSetChunk, 1, chunksize, chunkdata, 0);
			free(chunkdata);
		}
			
/*			// load all vst parameters
			int numparams=0;
			kfread2(&numparams, 1, sizeof(int), file);
			for(VstInt32 i=0;i<numparams;i++)
			{
				float value=0.0f;
				kfread2(&value, 1, sizeof(float), file);
				params.vst_plugin->setParameter(params.vst_plugin, i, value);
			}*/
		return true;
	};
};

void vsti_channel::DispatchEvents()
{
	gin_vsti* instrument=(gin_vsti*)params->self;
	params->vst_plugin->dispatcher(params->vst_plugin, effProcessEvents, 0, 0, instrument->VEQ_GetEvents(), 0);
}

void vsti_channel::EmptyEventQueue()
{
	gin_vsti* instrument=(gin_vsti*)params->self;
	instrument->VEQ_EmptyQueue();
}

void vsti_channel::SendVSTMidiEvent(unsigned char b1, unsigned char b2, unsigned char b3)
{
	if(params->vst_plugin==NULL)
		return;

	LogPrintf("[midi:%.2X%.2X%.2X] ", b1, b2, b3);

	((gin_vsti*)params->self)->VEQ_AddMidiEvent(b1, b2, b3);
}

#endif

