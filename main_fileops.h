bool SaveSong(char *filename)
{
	LogPrint("main: save song");
	int gtime=timeGetTime();

	FILE *file=kfopen(filename, "wb");
	if(file==NULL)
		return false;
	char vstring[6];
	strcpy(vstring, "SMU05");
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
			kfwrite2(&gearstacks[i].instrument->gui_hue, 1, sizeof(float), file);
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

	LogPrint("main: save took %i ms", timeGetTime()-gtime);
	
	return true;
}

bool LoadInstrument(GearStack *gs, FILE *file, bool songload)
{
	char gsname[64];
	kfread2(gsname, 1, 64, file);
	gsname[63]='\0'; // in case the string is garbage
	LogPrint("main: loading instrument \"%s\"", gsname);
	gear_instrument *instrument=NULL;
	if(strcmp(gsname, "xnes")==0)
		instrument=new gin_xnes();
	if(strcmp(gsname, "chip")==0)
		instrument=new gin_chip();
	if(strcmp(gsname, "protobass")==0)
		instrument=new gin_protobass();
	if(strcmp(gsname, "swave")==0)
		instrument=new gin_swave();
	if(strcmp(gsname, "vsmp")==0)
		instrument=new gin_vsmp();
	if(strcmp(gsname, "midinst")==0)
		instrument=new gin_midinst();
	if(strcmp(gsname, "midperc")==0)
		instrument=new gin_midperc();
	if(strcmp(gsname, "VSTi")==0)
		instrument=new gin_vsti();
	if(strcmp(gsname, "operator")==0)
		instrument=new gin_operator();
	if(strcmp(gsname, "wavein")==0)
		instrument=new gin_wavein();
	LogPrint("main: identified (or not)");
	if(instrument==NULL) // unknown instrument
	{
		LogPrint("main: unknown");
		if(songload && errorcount<5)
		{
			char string[128];
			sprintf(string, "Unknown instrument \"%s\" skipped", gsname);
			MessageBox(hWndMain, string, "Warning", MB_ICONEXCLAMATION);
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
		LogPrint("main: delete old instrument");
		delete gs->instrument;
		LogPrint("main: reassign instrument");
		gs->instrument=instrument;
		LogPrint("main: load instrument");
		bool success=gs->instrument->Load(file);
		if(!success && errorcount<5)
		{
			char string[128];
			sprintf(string, "Unable to load instrument \"%s\" - unsupported version?", gsname);
			MessageBox(hWndMain, string, "Warning", MB_ICONEXCLAMATION);
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
			MessageBox(hWndMain, string, "Warning", MB_ICONEXCLAMATION);
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
			MessageBox(hWndMain, string, "Warning", MB_ICONEXCLAMATION);
			errorcount++;
		}
		return success;
	}
}

bool LoadSong(char *filename)
{
	errorcount=0;
	LogPrint("main: load song");
	int gtime=timeGetTime();

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
	if(strcmp(vstring, "SMU05")==0)
		fversion=4;
	if(fversion==-1) // incompatible file format
	{
		fclose(file);
		return false;
	}
	SetFileVersion(fversion); // communicate file version to any other loading code
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
	if(fversion<4)
		ftempo*=3445.0f/3500.0f; // compensate for slightly different tempo formula
	UpdateTempo();
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
		if(fversion>=4)
			kfread2(&gearstacks[index].instrument->gui_hue, 1, sizeof(float), file);
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
	current_part=NULL;
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

	LogPrint("main: load took %i ms", timeGetTime()-gtime);
	
	if(errorcount>=5)
		MessageBox(hWndMain, "Too many errors. If you want more details - quit the program, enable logging in config.txt, reload the song, then read log.txt", "Warning", MB_ICONEXCLAMATION);
	errorcount=0;
	
	glkit_resized=true; // to allow snapped windows to adjust
	
	return true;
}

bool ExportMID(char *filename)
{
	LogPrint("main: export midi");

	FILE *file=fopen(filename, "wb");
	if(file==NULL)
		return false;

	midi_outfile=file;

	// write header
	// start song playback, causing midi triggers to be written
	// poll for thesong->playing==false
	// write footer (if any)
	// done

	// -- header chunk

	// id string
	char hstring[5]="MThd";
	fwrite(hstring, 4, 1, file);
	// size
	unsigned char byte;
	byte=0;
	fwrite(&byte, 1, 1, file);
	fwrite(&byte, 1, 1, file);
	fwrite(&byte, 1, 1, file);
	byte=6;
	fwrite(&byte, 1, 1, file);
	// format
	byte=0;
	fwrite(&byte, 1, 1, file);
	fwrite(&byte, 1, 1, file);
	// numtracks
	byte=0;
	fwrite(&byte, 1, 1, file);
	byte=1;
	fwrite(&byte, 1, 1, file);
	// time division (one tick per millisecond)
/*	byte=0xE7;
	fwrite(&byte, 1, 1, file);
	byte=0x28;
	fwrite(&byte, 1, 1, file);*/

	// 4/4, 75 bpm, 50 tpqn (if 16 qn per beat)
	byte=0x00;
	fwrite(&byte, 1, 1, file);
	byte=50;
	fwrite(&byte, 1, 1, file);
	
	// -- track chunk

	// id string
	char tstring[5]="MTrk";
	fwrite(tstring, 4, 1, file);
	// size (unknown for now, will be filled in later)
	byte=0;
	int sizepos=ftell(file);
	fwrite(&byte, 1, 1, file);
	fwrite(&byte, 1, 1, file);
	fwrite(&byte, 1, 1, file);
	fwrite(&byte, 1, 1, file);

	// tempo
	byte=0x00;
	fwrite(&byte, 1, 1, file);
	byte=0xFF;
	fwrite(&byte, 1, 1, file);
	byte=0x51;
	fwrite(&byte, 1, 1, file);
	byte=0x03;
	fwrite(&byte, 1, 1, file);
	byte=0x00;
	fwrite(&byte, 1, 1, file);
	byte=0xC3;
	fwrite(&byte, 1, 1, file);
	byte=0x50;
	fwrite(&byte, 1, 1, file);

	// start playback and wait for it to finish
	bool loopsetting=thesong->loop;
	thesong->loop=false;
	midi_datasize=0;
	midi_stime=0;
	midi_firstevent=true;
	midi_exporting=true;
	// free midi channels to enforce program changes at the start of the song
	for(int i=0;i<16;i++)
		midi_freechannel(i);
	thesong->PlayButton();
	if(!thesong->playing)
		thesong->PlayButton();
	while(thesong->playing)
	{
		Sleep(50);
	}
	thesong->loop=loopsetting;
	midi_exporting=false;

	// end of track event
	byte=0x00;
	fwrite(&byte, 1, 1, file);
	midi_datasize++;
	byte=0xFF;
	fwrite(&byte, 1, 1, file);
	midi_datasize++;
	byte=0x2F;
	fwrite(&byte, 1, 1, file);
	midi_datasize++;
	byte=0x00;
	fwrite(&byte, 1, 1, file);
	midi_datasize++;

	// seek back to track data size bytes and fill them in
	fseek(file, sizepos, SEEK_SET);
	byte=(midi_datasize>>24)&0xFF;
	fwrite(&byte, 1, 1, file);
	byte=(midi_datasize>>16)&0xFF;
	fwrite(&byte, 1, 1, file);
	byte=(midi_datasize>>8)&0xFF;
	fwrite(&byte, 1, 1, file);
	byte=(midi_datasize)&0xFF;
	fwrite(&byte, 1, 1, file);
	
//	LogPrint("midi track data size: %i [%.8X]", midi_datasize, midi_datasize);

	fclose(file);
	return true;
}

