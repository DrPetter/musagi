#ifndef Texture_h
#define Texture_h

#include <GL/gl.h>
#include <stdio.h>

// Parameters for "mode"
#define TEXTURE_NORMAL		0x00
#define TEXTURE_NOFILTER	0x01
#define TEXTURE_NOREPEAT	0x02
#define TEXTURE_NOMIPMAPS	0x04
#define TEXTURE_ANISOTROPIC	0x08

class Texture
{
private:
	GLuint	handle;

public:
	int width;
	int height;
	
	Texture();
	~Texture();

	bool LoadTGA(const char *filename, unsigned char mode); // mode: bitmask (0=normal)
	void CreateFromData(unsigned char *data, int width, int height, unsigned char mode);
	GLuint getHandle();
};

#endif
