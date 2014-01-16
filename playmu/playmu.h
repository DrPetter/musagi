struct playmu_sound
{
	float* samples;
	int length;
	int channels;
};

void playmu_init();
void playmu_close();
void playmu_playsong(int fadein_milliseconds);
void playmu_stopsong();
void playmu_songvolume(int percent);
void playmu_mastervolume(int percent);
void playmu_fadeout(int milliseconds);
bool playmu_loadsong(char *filename);
void playmu_playsound(playmu_sound& sound, int volume, bool loop);
void playmu_playsound(playmu_sound& sound, int volume);
void playmu_stopsound(playmu_sound& sound);
void playmu_soundvolume(playmu_sound& sound, int volume);
bool playmu_loadsound(playmu_sound& sound, char* filename, int channels);
void playmu_freesound(playmu_sound& sound);

void playmu_disable();

