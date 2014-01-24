#include "DPInput.h"
#define VK_OEM_COMMA 0xBC;
#define VK_OEM_PERIOD 0xBE;

DPInput *input;

int kbkbk[50];
int kbkbm[50];
int kbkbc[50];

HANDLE hMidiThread;

void midiplay_routine();
DWORD WINAPI midiplay_thread(LPVOID lpParam)
{
	midiplay_routine();
	return 0;
}

void platform_Sleep(int msec)
{
	Sleep(msec);
}

#define MESSAGEBOX_OK		0
#define MESSAGEBOX_OKCANCEL	1
bool platform_MessageBox(DWORD hwnd, const char* body, const char* title, int type)
{
	int result=-1;
	if(type==MESSAGEBOX_OK)
		result=MessageBox((HWND)hwnd, body, title, MB_ICONEXCLAMATION);
	if(type==MESSAGEBOX_OKCANCEL)
		result=MessageBox((HWND)hwnd, body, title, MB_ICONEXCLAMATION|MB_OKCANCEL);
	if(result==IDOK)
		return true;
	return false;
}

//HWND GetHwnd()
DWORD GetHwnd()
{
	return (DWORD)hWndMain;
}

void platform_GetExecutablePath(char* str)
{
	GetModuleFileName(NULL, str, 512);
}

void platform_set_portaudio_latency(const char* latency)
{
	SetEnvironmentVariable("PA_MIN_LATENCY_MSEC", latency);
}

void platform_init_keyboard()
{
	// keyboard keyboard
	kbkbk[0]=DIK_Z; // C
	kbkbm[0]=0;
	kbkbc[0]='Z';
	kbkbk[1]=DIK_S; // C#
	kbkbm[1]=1;
	kbkbc[1]='S';
	kbkbk[2]=DIK_X; // D
	kbkbm[2]=2;
	kbkbc[2]='X';
	kbkbk[3]=DIK_D; // D#
	kbkbm[3]=3;
	kbkbc[3]='D';
	kbkbk[4]=DIK_C; // E
	kbkbm[4]=4;
	kbkbc[4]='C';
	kbkbk[5]=DIK_V; // F
	kbkbm[5]=5;
	kbkbc[5]='V';
	kbkbk[6]=DIK_G; // F#
	kbkbm[6]=6;
	kbkbc[6]='G';
	kbkbk[7]=DIK_B; // G
	kbkbm[7]=7;
	kbkbc[7]='B';
	kbkbk[8]=DIK_H; // G#
	kbkbm[8]=8;
	kbkbc[8]='H';
	kbkbk[9]=DIK_N; // A
	kbkbm[9]=9;
	kbkbc[9]='N';
	kbkbk[10]=DIK_J; // A#
	kbkbm[10]=10;
	kbkbc[10]='J';
	kbkbk[11]=DIK_M; // B
	kbkbm[11]=11;
	kbkbc[11]='M';
	kbkbk[12]=DIK_COMMA; // C
	kbkbm[12]=12;
	kbkbc[12]=VK_OEM_COMMA;
	kbkbk[13]=DIK_L; // C#
	kbkbm[13]=13;
	kbkbc[13]='L';
	kbkbk[14]=DIK_PERIOD; // D
	kbkbm[14]=14;
	kbkbc[14]=VK_OEM_PERIOD;
	kbkbk[15]=DIK_SLASH; // E
	kbkbm[15]=16;
	kbkbc[15]=VK_DIVIDE; // doesn't seem to work, but neither does VK_SUBTRACT

	kbkbk[16]=DIK_Q; // C
	kbkbm[16]=12;
	kbkbc[16]='Q';
	kbkbk[17]=DIK_2; // C#
	kbkbm[17]=13;
	kbkbc[17]='2';
	kbkbk[18]=DIK_W; // D
	kbkbm[18]=14;
	kbkbc[18]='W';
	kbkbk[19]=DIK_3; // D#
	kbkbm[19]=15;
	kbkbc[19]='3';
	kbkbk[20]=DIK_E; // E
	kbkbm[20]=16;
	kbkbc[20]='E';
	kbkbk[21]=DIK_R; // F
	kbkbm[21]=17;
	kbkbc[21]='R';
	kbkbk[22]=DIK_5; // F#
	kbkbm[22]=18;
	kbkbc[22]='5';
	kbkbk[23]=DIK_T; // G
	kbkbm[23]=19;
	kbkbc[23]='T';
	kbkbk[24]=DIK_6; // G#
	kbkbm[24]=20;
	kbkbc[24]='6';
	kbkbk[25]=DIK_Y; // A
	kbkbm[25]=21;
	kbkbc[25]='Y';
	kbkbk[26]=DIK_7; // A#
	kbkbm[26]=22;
	kbkbc[26]='7';
	kbkbk[27]=DIK_U; // B
	kbkbm[27]=23;
	kbkbc[27]='U';
	kbkbk[28]=DIK_I; // C
	kbkbm[28]=24;
	kbkbc[28]='I';
	kbkbk[29]=DIK_9; // C#
	kbkbm[29]=25;
	kbkbc[29]='9';
	kbkbk[30]=DIK_O; // D
	kbkbm[30]=26;
	kbkbc[30]='O';
	kbkbk[31]=DIK_0; // D#
	kbkbm[31]=27;
	kbkbc[31]='0';
	kbkbk[32]=DIK_P; // E
	kbkbm[32]=28;
	kbkbc[32]='P';
}

void platform_start_midi_thread()
{
	DWORD dwThreadId;
	hMidiThread=CreateThread(NULL,  			   // default security attributes
        					0,                 // use default stack size
				            midiplay_thread,        // thread function
				            (LPVOID)0,             // argument to thread function
				            0,                 // use default creation flags
				            &dwThreadId);   // returns the thread identifier
}

