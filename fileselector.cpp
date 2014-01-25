#include <SDL/SDL.h>

#include "fileselector.h"

char chosen_filename[5][512];
char chosen_file[5][512];
char fsel_wave[128]="Wave Files (*.wav)\0*.wav\0All Files (*.*)\0*.*\0";
char fsel_inst[128]="Instrument Files (*.imu)\0*.imu\0All Files (*.*)\0*.*\0";
char fsel_part[128]="Part Files (*.pmu)\0*.pmu\0All Files (*.*)\0*.*\0";
char fsel_song[128]="Song Files (*.smu)\0*.smu\0All Files (*.*)\0*.*\0";
char fsel_defext[8];

void FileSelectorInit()
{
	for(int i=0;i<5;i++)
	{
		chosen_filename[i][0]='\0';
		chosen_file[i][0]='\0';
	}
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
	}
	return NULL;
}

bool FileSelectorSave(SDL_Surface *screen, char *filename, int type)
{
	/*char *filter=GetFilter(type);
	static OPENFILENAME dia;
	dia.lStructSize = sizeof(OPENFILENAME);
	dia.hwndOwner = hwnd;
	dia.lpstrFile = chosen_filename[type];
	dia.nMaxFile = _MAX_DIR;
	dia.lpstrFileTitle = chosen_file[type];
	dia.nMaxFileTitle = _MAX_FNAME;
	dia.lpstrInitialDir = NULL;
	dia.lpstrFilter = filter;
	dia.lpstrDefExt = fsel_defext;
	dia.lpstrTitle = "Save As";
	dia.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT;
	if(!GetSaveFileName(&dia))
		return false;
	strcpy(filename, chosen_filename[type]);
	return true;*/
        return false;
}

bool FileSelectorLoad(SDL_Surface *screen, char *filename, int type)
{
	/*char *filter=GetFilter(type);
	static OPENFILENAME dia;
	dia.lStructSize = sizeof(OPENFILENAME);
	dia.hwndOwner = hwnd;
	dia.lpstrFile = chosen_filename[type];
	dia.nMaxFile = _MAX_DIR;
	dia.lpstrFileTitle = chosen_file[type];
	dia.nMaxFileTitle = _MAX_FNAME;
	dia.lpstrInitialDir = NULL;
	dia.lpstrFilter = filter;
	dia.lpstrDefExt = fsel_defext;
	dia.lpstrTitle = "Load";
	dia.Flags = OFN_EXPLORER;
	if(!GetOpenFileName(&dia))
		return false;
	strcpy(filename, chosen_filename[type]);
	return true;*/
        return false;
}

