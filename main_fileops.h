#include <SDL/SDL.h>

bool SaveSong(char *filename)
{
	LogPrint("main: save song");
	int gtime=SDL_GetTicks();//timeGetTime();

	FILE *file=kfopen(filename, "wb");
	if(file==NULL)
		return false;
	char vstring[6];
	strcpy(vstring, "SMU04");
	kfwrite2(vstring, 5, 1, file);
	// header
	kfwrite(&ftempo, 1, sizeof(float), file);
	kfwrite(&master_volume, 1, sizeof(float), file);
	// ?
	// save gearstacks
	int numstacks=0;
	for(int i=0;i<maxstacks;i++)
		if(gearstacks[i].instrument!=NULL)
			numstacks++;
	kfwrite2(&numstacks, 1, sizeof(int), file);
	for(int i=0;i<maxstacks;i++)
		if(gearstacks[i].instrument!=NULL)
		{
			kfwrite2(&i, 1, sizeof(int), file);
			kfwrite2(&gearstacks[i], 1, sizeof(GearStack), file);
			gearstacks[i].instrument->Save(file);
			kfwrite2(&gearstacks[i].num_effects, 1, sizeof(int), file);
			for(int j=0;j<gearstacks[i].num_effects;j++)
				gearstacks[i].effects[j]->Save(file);

			LogPrint("main: saved gearstack %i (%s)", i, gearstacks[i].instrument->Name());

			LogPrint("main: filesize=%i", ftell(file));
/*			// calibrate knobrec pointers by running a pass through the GUI
			gearstacks[index].instrument->DefaultWindow(dui, -1000, 0, 0);
			for(int j=0;j<gearstacks[index].num_effects;j++)
				gearstacks[index].effects[j]->DefaultWindow(dui, -1000, 0, 0);*/
		}
	// save stacklist
	kfwrite2(&stacklistnum, 1, sizeof(int), file);
	for(int i=0;i<stacklistnum;i++)
		kfwrite2(&stacklist[i], 1, sizeof(int), file);
	// save parts
	int numparts=0;
	for(int i=0;i<maxparts;i++)
		if(mungoparts[i]!=NULL)
			numparts++;
	kfwrite2(&numparts, 1, sizeof(int), file);
	for(int i=0;i<maxparts;i++)
		if(mungoparts[i]!=NULL)
		{
			kfwrite2(&i, 1, sizeof(int), file);
			mungoparts[i]->Save(file);
		}
	LogPrint("main: saved parts");
	LogPrint("main: filesize=%i", ftell(file));
	// save song
	thesong->Save(file);
	kfsaveknobs(file);
	LogPrint("main: saved knobs");
	LogPrint("main: filesize=%i", ftell(file));
	kfclose(file);

	LogPrint("main: save took %i ms", SDL_GetTicks()-gtime);//timeGetTime()-gtime);
	
	return true;
}

bool LoadInstrument(GearStack *gs, FILE *file, bool songload)
{
	char gsname[64];
	kfread2(gsname, 1, 64, file);
	gsname[63]='\0'; // in case the string is garbage
//	LogPrint("main: loading instrument \"%s\"", gsname);
	gear_instrument *instrument=NULL;
	if(strcmp(gsname, "xnes")==0)
		instrument=new gin_xnes();
	if(strcmp(gsname, "chip")==0)
		instrument=new gin_chip();
	if(strcmp(gsname, "protobass")==0)
		instrument=new gin_protobass();
	if(strcmp(gsname, "swave")==0)
		instrument=new gin_swave();
	if(strcmp(gsname, "vsampler")==0)
		instrument=new gin_vsampler();
	if(instrument==NULL) // unknown instrument
	{
		if(songload && errorcount<5)
		{
			char string[128];
			sprintf(string, "Unknown instrument \"%s\" skipped", gsname);
			//MessageBox(hWndMain, string, "Warning", MB_ICONEXCLAMATION);
                        printf("%s\n",string);
			errorcount++;
		}
		else
			return false; // only trying to load this instrument, so no need to read past it
		LogPrint("CAUTION: unknown instrument \"%s\" - not loaded", gsname);
		// skip parameter data
		int numbytes=0;
		kfread2(&numbytes, 1, sizeof(int), file);
		ReadAndDiscard(file, numbytes);
		return false;
	}
	else
	{
//		LogPrint("main: delete old instrument");
		delete gs->instrument;
//		LogPrint("main: reassign instrument");
		gs->instrument=instrument;
//		LogPrint("main: load instrument");
		bool success=gs->instrument->Load(file);
		if(!success && errorcount<5)
		{
			char string[128];
			sprintf(string, "Unable to load instrument \"%s\" - unsupported version?", gsname);
			//MessageBox(hWndMain, string, "Warning", MB_ICONEXCLAMATION);
                        printf("%s\n", string);
			errorcount++;
		}
		return success;
	}
}

bool LoadEffect(GearStack *gs, int index, FILE *file, bool songload, int fversion)
{
	char gsname[64];
	kfread2(gsname, 1, 64, file);
	gsname[63]='\0'; // in case the string is garbage
	gear_effect *effect=NULL;
	if(strcmp(gsname, "gapan")==0)
		effect=new gef_gapan();
	if(effect==NULL) // unknown effect
	{
		if(songload && errorcount<5)
		{
			char string[128];
			sprintf(string, "Unknown effect \"%s\" skipped", gsname);
			//MessageBox(hWndMain, string, "Warning", MB_ICONEXCLAMATION);
                        printf("%s\n", string);
			errorcount++;
		}
		else
			return false; // only trying to load this effect, so no need to read past it
		LogPrint("CAUTION: unknown effect \"%s\" - not loaded", gsname);
		// skip parameter data
		int numbytes=0;
		kfread2(&numbytes, 1, sizeof(int), file);
		ReadAndDiscard(file, numbytes);
		return false;
	}
	else
	{
//		delete gs->instrument;
		gs->effects[index]=effect;
		bool success=gs->effects[index]->Load(file, fversion);
		if(!success && errorcount<5)
		{
			char string[128];
			sprintf(string, "Unable to load effect \"%s\" - unsupported version?", gsname);
			//MessageBox(hWndMain, string, "Warning", MB_ICONEXCLAMATION);
                        printf("%s\n", string);
			errorcount++;
		}
		return success;
	}
}

bool LoadSong(char *filename)
{
	errorcount=0;
	LogPrint("main: load song");
	int gtime=SDL_GetTicks();//timeGetTime();

        printf("Song filename: %s\n", filename);

	FILE *file=kfopen(filename, "rb");
	if(file==NULL)
		return false;
	kfloadknobs(file);
	char vstring[6];
	vstring[5]='\0';
	kfread2(vstring, 5, 1, file);
	int fversion=-1;
	if(strcmp(vstring, "SMU01")==0)
		fversion=0;
	if(strcmp(vstring, "SMU02")==0)
		fversion=1;
	if(strcmp(vstring, "SMU03")==0)
		fversion=2;
	if(strcmp(vstring, "SMU04")==0)
		fversion=3;
	if(fversion==-1) // incompatible file format
	{
		fclose(file);
		return false;
	}
	// clear all data
	audiostream->Flush();
	for(int i=0;i<maxstacks;i++)
		if(gearstacks[i].instrument!=NULL)
		{
			for(int j=0;j<gearstacks[i].num_effects;j++)
				delete gearstacks[i].effects[j];
			delete gearstacks[i].instrument;
			gearstacks[i].instrument=NULL;
//			audiostream->RemoveGearStack(&gearstacks[i]);
		}
	for(int i=0;i<maxparts;i++)
		if(mungoparts[i]!=NULL)
		{
			delete mungoparts[i];
			mungoparts[i]=NULL;
		}

	// header
	kfread(&ftempo, 1, sizeof(float), file);
//	tempo=24+(int)((1.0f-ftempo)*40);
	tempo=20+(int)(pow(1.0f-ftempo, 2.0f)*60);
	kfread(&master_volume, 1, sizeof(float), file);
	
	// ?
	// load gearstacks
	int numstacks=0;
	fcurstack=0;
	kfread2(&numstacks, 1, sizeof(int), file);
	LogPrint("main: numstacks=%i", numstacks);
	for(int i=0;i<numstacks;i++)
	{
		int index=0;
		kfread2(&index, 1, sizeof(int), file);
		LogPrint("main: loading stack %i -> slot %i", i, index);
		kfread2(&gearstacks[index], 1, sizeof(GearStack), file);
		gearstacks[index].instrument=NULL;
		LogPrint("main: loading instrument");
		if(!LoadInstrument(&gearstacks[index], file, true))
		{
			LogPrint("main: failed to load instrument %i", index);
//			gearstacks[index].instrument=new gin_xnes(); // if instrument load failed, replace it with xnes (hmmm, dubious practice?)
		}
		LogPrint("main: loaded instrument [%.8X]", gearstacks[index].instrument);
		LogPrint("main: loading effects");
		gearstacks[index].num_effects=0;
		kfread2(&gearstacks[index].num_effects, 1, sizeof(int), file);
		for(int j=0;j<gearstacks[index].num_effects;j++)
		{
			if(!LoadEffect(&gearstacks[index], j, file, true, fversion))
				LogPrint("main: failed to load effect %i for instrument %i", j, index);
		}
/*		// calibrate knobrec pointers by running a pass through the GUI
		gearstacks[index].instrument->DefaultWindow(dui, -1000, 0, 0);
		for(int j=0;j<gearstacks[index].num_effects;j++)
			gearstacks[index].effects[j]->DefaultWindow(dui, -1000, 0, 0);*/
	}
	// load stacklist
	LogPrint("main: loading stacklist");
	kfread2(&stacklistnum, 1, sizeof(int), file);
	for(int i=0;i<stacklistnum;i++)
		kfread2(&stacklist[i], 1, sizeof(int), file);
	// load parts
	int numparts=0;
	kfread2(&numparts, 1, sizeof(int), file);
	LogPrint("main: numparts=%i", numparts);
	int maxp=0;
	for(int i=0;i<numparts;i++)
	{
		int index=0;
		kfread2(&index, 1, sizeof(int), file);
//		LogPrint("loading part %i", index);
		mungoparts[index]=new Part(index);
		LogPrint("main: loading part %i to slot %i", i, index);
		mungoparts[index]->Load(file, gearstacks, fversion);
		maxp=index;
	}
	// load song
	LogPrint("main: loading song");
	thesong->Load(file, mungoparts, maxp+1);
	kfclose(file);

	for(int i=0;i<maxstacks;i++)
		if(gearstacks[i].instrument!=NULL)
		{
			LogPrint("main: adding gearstack %i", i);
			audiostream->AddGearStack(&gearstacks[i]);
		}

	LogPrint("main: load took %i ms", SDL_GetTicks()-gtime);//timeGetTime()-gtime);
	
	if(errorcount>=5)
		//MessageBox(hWndMain, "Too many errors. If you want more details - quit the program, enable logging in config.txt, reload the song, then read log.txt", "Warning", MB_ICONEXCLAMATION);
                printf("Too many errors. If you want more details - quit the program, enable logging in config.txt, reload the song, then read log.txt\n");
	errorcount=0;
	
	glkit_resized=true; // to allow snapped windows to adjust
	
	return true;
}

