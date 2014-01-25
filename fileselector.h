#ifndef FILESELECTOR_H
#define FILESELECTOR_H

#include <SDL/SDL.h>

void FileSelectorInit();

bool FileSelectorSave(SDL_Surface *screen, char *filename, int type);

bool FileSelectorLoad(SDL_Surface *screen, char *filename, int type);

#endif

