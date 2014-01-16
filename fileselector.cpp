#include "fileselector.h"
#include "musagi.h"

char chosen_filename[6][512];
char chosen_file[6][512];
char fsel_wave[128]="Wave Files (*.wav)\0*.wav\0All Files (*.*)\0*.*\0";
char fsel_inst[128]="Instrument Files (*.imu)\0*.imu\0All Files (*.*)\0*.*\0";
char fsel_part[128]="Part Files (*.pmu)\0*.pmu\0All Files (*.*)\0*.*\0";
char fsel_song[128]="Song Files (*.smu)\0*.smu\0All Files (*.*)\0*.*\0";
char fsel_midi[128]="Midi Files (*.mid)\0*.mid\0All Files (*.*)\0*.*\0";
char fsel_vsti[128]="VSTi Files (*.dll)\0*.dll\0All Files (*.*)\0*.*\0";
char fsel_defext[8];

void FileSelectorInit()
{
	for(int i=0;i<6;i++)
	{
		chosen_filename[i][0]='\0';
		chosen_file[i][0]='\0';
	}
//	strcpy(chosen_filename[0], "E:\\dpdev\\OpenGL\\musagi\\documentation\\*.smu");
//	strcpy(chosen_file[0], "E:\\dpdev\\OpenGL\\musagi\\documentation\\");
}

char* GetFilter(int type)
{
	switch(type)
	{
	case 0:
		return fsel_song;
	case 1:
		return fsel_part;
	case 2:
		return fsel_inst;
	case 3:
		return fsel_wave;
	case 4:
		return fsel_midi;
	case 5:
		return fsel_vsti;
	}
	return NULL;
}

bool FileSelectorSave(HWND hwnd, char *filename, int type)
{
	char *filter=GetFilter(type);
	static OPENFILENAME dia;
	dia.lStructSize = sizeof(OPENFILENAME);
	dia.hwndOwner = hwnd;
	dia.lpstrFile = chosen_filename[type];
	dia.nMaxFile = _MAX_DIR;
	dia.lpstrFileTitle = chosen_file[type];
	dia.nMaxFileTitle = _MAX_FNAME;
	dia.lpstrInitialDir = GetCurDir(type);
	dia.lpstrFilter = filter;
	dia.lpstrDefExt = fsel_defext;
	dia.lpstrTitle = "Save As";
	dia.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT;
	if(!GetSaveFileName(&dia))
		return false;
	strcpy(filename, chosen_filename[type]);
	SetCurDir(type, chosen_filename[type]);
	return true;
}

bool FileSelectorLoad(HWND hwnd, char *filename, int type, char* title)
{
	char *filter=GetFilter(type);
	static OPENFILENAME dia;
	dia.lStructSize = sizeof(OPENFILENAME);
	dia.hwndOwner = hwnd;
	dia.lpstrFile = chosen_filename[type];
	dia.nMaxFile = _MAX_DIR;
	dia.lpstrFileTitle = chosen_file[type];
	dia.nMaxFileTitle = _MAX_FNAME;
	dia.lpstrInitialDir = GetCurDir(type);
	dia.lpstrFilter = filter;
	dia.lpstrDefExt = fsel_defext;
	dia.lpstrTitle = title;
	dia.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST;
	if(!GetOpenFileName(&dia))
		return false;
	strcpy(filename, chosen_filename[type]);
	SetCurDir(type, chosen_filename[type]);
	return true;
}

bool FileSelectorLoad(HWND hwnd, char *filename, int type)
{
	return FileSelectorLoad(hwnd, filename, type, "Load");
}

