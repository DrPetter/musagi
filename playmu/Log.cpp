#include "Log.h"

bool commonlib_logactive;
char logfilename[512];

void LogStart(char *filename)
{
	strcpy(logfilename, filename);
	FILE *file;
	file=fopen(logfilename, "w");
//	fprintf(file,"Log started");
	fclose(file);
	commonlib_logactive=true;
}

void LogPrint(char *string, ...)
{
	if(!commonlib_logactive)
		return;
		
	FILE *file;
	char temp[1024];
	va_list args;

	va_start(args, string);
	vsprintf(temp, string, args);
	va_end(args);

	file=fopen(logfilename, "a");
	fprintf(file,"%s\n", temp);
	fclose(file);
}

void LogPrintf(char *string, ...)
{
	if(!commonlib_logactive)
		return;
		
	FILE *file;
	char temp[1024];
	va_list args;

	va_start(args, string);
	vsprintf(temp, string, args);
	va_end(args);

	file=fopen(logfilename, "a");
	fprintf(file,"%s", temp);
	fclose(file);
}

void LogEnable()
{
	LogPrint("Log enabled");
	commonlib_logactive=true;
}

void LogDisable()
{
	LogPrint("Log disabled");
	commonlib_logactive=false;
}

