#ifndef Log_h
#define Log_h

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>


void LogStart(char *filename);

void LogPrint(char *string, ...);

void LogPrintf(char *string, ...);

void LogEnable();
void LogDisable();

#endif
