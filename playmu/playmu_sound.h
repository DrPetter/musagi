struct SESound
{
	playmu_sound* sound;
	int pos;
	float volume;
	bool loop;
	bool stop;
};

SESound playmu_sesounds[64];
float* playmu_soundbank[2048];
StereoBufferP playmu_soundbuffer;

void playmu_ZeroSoundBuffer()
{
	if(playmu_soundbuffer.size==0)
		return;
	ZeroBuffer(playmu_soundbuffer.left, playmu_soundbuffer.size);
	ZeroBuffer(playmu_soundbuffer.right, playmu_soundbuffer.size);
}

void playmu_RenderSoundBuffer(SESound& se, int size)
{
	StereoBufferP& buffer=playmu_soundbuffer;
	if(buffer.size!=size) // allocate new buffer if incorrect size
	{
//		LogPrint("allocating sound effect buffer (%i samples)", size);
		free(buffer.left);
		free(buffer.right);
		buffer.size=size;
		buffer.mono=false;
		buffer.left=(float*)malloc(buffer.size*sizeof(float));
		buffer.right=(float*)malloc(buffer.size*sizeof(float));
		playmu_ZeroSoundBuffer();
	}
	for(int i=0;i<size;i++)
	{
		if(se.pos<se.sound->length)
		{
			buffer.left[i]+=se.sound->samples[se.pos]*se.volume;
			buffer.right[i]+=se.sound->samples[se.pos]*se.volume;
		}
		se.pos++;
		if(se.pos>=se.sound->length && se.loop) // restart if looping
			se.pos=0;
	}
	if(se.pos>=se.sound->length) // finished playing
		se.sound=NULL;
}

void playmu_freesound(playmu_sound& sound)
{
	for(int i=0;i<2048;i++)
		if(playmu_soundbank[i]==sound.samples) // only free if it has been loaded
		{
			free(sound.samples);
			sound.samples=NULL;
			sound.length=0;
			playmu_soundbank[i]=NULL;
			break;
		}
}

bool playmu_loadsound(playmu_sound& sound, char* filename, int channels)
{
	playmu_freesound(sound);

	FILE *file=fopen(filename, "rb");
	if(!file)
		return false;

	int wchannels=0;
	int samplebits=0;
	int samplerate=0;
	
	char string[256];
	int word=0;
	int dword=0;
	int chunksize=0;
	
	fread(string, 4, 1, file); // "RIFF"
	fread(&dword, 1, 4, file); // remaining file size
	fread(string, 4, 1, file); // "WAVE"

	fread(string, 4, 1, file); // "fmt "
	fread(&chunksize, 1, 4, file); // chunk size
	fread(&word, 1, 2, file); // compression code
	if(word!=1)
	{
//		LogPrint("swave: Unsupported compression format (%i)", word);
		return false; // not PCM
	}
	fread(&wchannels, 1, 2, file); // channels
	fread(&samplerate, 1, 4, file); // sample rate
	fread(&dword, 1, 4, file); // bytes/sec (ignore)
	fread(&word, 1, 2, file); // block align (ignore)
	fread(&samplebits, 1, 2, file); // bits per sample
	for(int i=0;i<chunksize-16;i+=2) // extra format bytes
		fread(&word, 1, 2, file);

	fread(string, 4, 1, file); // "data"
//	string[4]='\0';
//	LogPrint("\"%s\"", string);
	fread(&chunksize, 1, 4, file); // chunk size
//	LogPrint("Chunksize=%i", chunksize);

	// read sample data	
//	float peak=1.0f/65535; // not zero...
	int samplesize=samplebits/8*wchannels;
	int numsamples=chunksize/samplesize;
//	LogPrint("Reading %i samples, bits=%i, channels=%i", numsamples, samplebits, channels);
	float *data=(float*)malloc(numsamples*sizeof(float));
	int audiostart=-1; // set to 0 to disable removal of silence at the start of the sample
	int audioend=0;
	for(int i=0;i<numsamples;i++)
	{
		float value=0.0f;
		if(samplebits==8) // 8 bit unsigned
		{
			unsigned char byte=0;
			fread(&byte, 1, 1, file);
			if(wchannels==2)
			{
				int a=(int)byte;
				fread(&byte, 1, 1, file);
				byte=(unsigned char)((a+byte)/2);
			}
			value=(float)(byte-128)/0x80;
		}
		if(samplebits==16) // 16 bit signed
		{
			short word=0;
			fread(&word, 1, 2, file);
			if(wchannels==2)
			{
				int a=(int)word;
				fread(&word, 1, 2, file);
				word=(short)((a+word)/2);
			}
			value=(float)word/0x8000;
		}
		if(value!=0.0f)
		{
			audioend=i+1;
			if(audiostart==-1)
				audiostart=i;
		}
		if(audiostart!=-1)
			data[i-audiostart]=value;
//		if(fabs(value)>peak)
//			peak=fabs(value);
	}
//	LogPrint("swave: audiostart=%i, audioend=%i", audiostart, audioend);
//	LogPrint("swave: silence removal (%i -> %i)", numsamples, audioend-audiostart);
	numsamples=audioend-audiostart;

	float *finaldata;	
	int slength=0;
	if(samplerate!=44100) // resampling needed
	{
		int length=numsamples*44100/samplerate;
//		LogPrint("swave: Resampling from %i (%i -> %i)", samplerate, numsamples, length);
		finaldata=(float*)malloc(length*sizeof(float));
		slength=length;
		for(int i=0;i<length;i++)
		{
			float fpos=(float)i/(length-1)*(numsamples-1);
			int index=(int)(fpos-0.5f);
			float f=fpos-(float)index;
			if(index<0) index=0;
			if(index>numsamples-2) index=numsamples-2;
			float s1=data[index];
			float s2=data[index+1];
			float value=s1*(1.0f-f)+s2*f;
			finaldata[i]=value;
		}
		free(data);
	}
	else // matching samplerate
	{
		finaldata=data;
		slength=numsamples;
	}

	// scale
	for(int i=0;i<slength;i++)
	{
		finaldata[i]*=0.5f; // default to half maximum volume, to alleviate clipping issues
//		finaldata[i]*=0.99f/peak;
//			finaldata[i]=(float)((char)(finaldata[i]*127))/127;
	}

//	for(int i=0;i<2000;i+=100)
//		LogPrint("%.2f", samples[id].data[i]);

//	LogPrint("swave: Loaded wave \"%s\": length=%i, %i bit, %i Hz, %i channels", filename, slength, samplebits, samplerate, channels);
	
	fclose(file);
	
//	return finaldata;
	sound.samples=finaldata;
	sound.channels=channels;
	sound.length=slength;
	
	for(int i=0;i<2048;i++)
		if(playmu_soundbank[i]==NULL)
		{
//			playmu_soundbank[i]=&sound;
			playmu_soundbank[i]=sound.samples;
			break;
		}

	return true;
}

void playmu_stopsound(playmu_sound& sound)
{
	for(int i=0;i<64;i++)
		if(playmu_sesounds[i].sound!=NULL && playmu_sesounds[i].sound->samples==sound.samples)
			playmu_sesounds[i].stop=true;
}

void playmu_soundvolume(playmu_sound& sound, int volume) // set volume of playing sound
{
	for(int i=0;i<64;i++)
		if(playmu_sesounds[i].sound!=NULL && playmu_sesounds[i].sound->samples==sound.samples)
			playmu_sesounds[i].volume=(float)volume/100;
}

void playmu_playsound(playmu_sound& sound, int volume)
{
	playmu_playsound(sound, volume, false);
}

void playmu_playsound(playmu_sound& sound, int volume, bool loop)
{
//	LogPrintf("playsound (%.8X)", sound.samples);

	if(sound.samples==NULL || sound.length==0)
	{
//		LogPrintf(" - empty sample\n");
		return;
	}
	int oldest=0;
	int channels=0;
	int i=0;
	// see if we need to override an old instance of the same sound
	for(i=0;i<64;i++)
		if(playmu_sesounds[i].sound!=NULL && playmu_sesounds[i].sound->samples==sound.samples)
		{
			channels++;
			oldest=i;
		}
	if(channels>=sound.channels)
	{
		i=oldest;
//		LogPrintf(" - override old channel");
	}
	else // ok to add new channel, search for spot (or override oldest playing)
	{
		int oldest_pos=0;
		for(i=0;i<64;i++)
		{
			if(playmu_sesounds[i].sound==NULL)
				break;
			else if(playmu_sesounds[i].pos>=oldest_pos)
			{
				oldest=i;
				oldest_pos=playmu_sesounds[i].pos;
			}
		}
		if(i==64)
		{
			i=oldest;
//			LogPrintf(" - channel overflow");
		}
	}
	playmu_sesounds[i].sound=&sound;
	playmu_sesounds[i].pos=0;
	playmu_sesounds[i].volume=(float)volume/100;
	playmu_sesounds[i].loop=loop;
	playmu_sesounds[i].stop=false;

//	LogPrintf(" @ channel=%i, volume=%.2f\n", i, playmu_sesounds[i].volume);

//	LogPrint("---channels:");

	channels=0;
	for(int ci=0;ci<64;ci++)
		if(playmu_sesounds[ci].sound!=NULL)
		{
			channels++;
//			LogPrint("ch%.2i: (%.8X) - v=%.2f pos=%.2i%%", ci, playmu_sesounds[ci].sound->samples, playmu_sesounds[ci].volume, playmu_sesounds[ci].pos*100/playmu_sesounds[ci].sound->length);
		}
//	LogPrint("---");
}

