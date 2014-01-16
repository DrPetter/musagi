#ifndef FILESELECTOR_H
#define FILESELECTOR_H

#include <windows.h>

void FileSelectorInit();
bool FileSelectorSave(HWND hwnd, char *filename, int type);
bool FileSelectorLoad(HWND hwnd, char *filename, int type);
bool FileSelectorLoad(HWND hwnd, char *filename, int type, char* title);

#endif

