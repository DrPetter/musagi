#include <windows.h>
#include <gl\gl.h>
#include <gl\glu.h>
#include "Texture.h"

//#include "VectorMath.h"
#include <gl\glext.h>

Texture::Texture()
{
	handle=0;
}

Texture::~Texture()
{
	glDeleteTextures(1, &handle);
}

bool Texture::LoadTGA(const char *filename, unsigned char mode) // Load TGA texture
{
//	int oh=handle;

//	GLuint texture;
	BYTE * data;
	FILE *file;
	unsigned char byte, id_length/*, chan[4]*/;
//	int n, width, height, channels, x, y;
	int n, channels, x, y;

	file=fopen(filename, "rb");
	if(!file)
		return false;

	fread( &id_length, 1, 1, file );	// length of ID field
	fread( &byte, 1, 1, file );			// color map (1 or 0)
	fread( &byte, 1, 1, file );			// image type (should be 2(unmapped RGB))
	if(byte!=2) // Not the right type of TGA
		return false;

	for(n=0;n<5;n++)
		fread( &byte, 1, 1, file );		// color map info
	fread( &n, 1, 2, file );		// image origin X
	fread( &n, 1, 2, file );		// image origin Y
	width=0;
	height=0;
	fread( &width, 1, 2, file );	// width
	fread( &height, 1, 2, file );	// height

	fread( &byte, 1, 1, file );		// bits
	channels=byte/8;

	fread( &byte, 1, 1, file );		// image descriptor byte (per-bit info)
	
//	FILE *blaj=fopen("texture.txt", "w");	
//	fprintf(blaj, "id_length=%i", id_length);
//	fclose(blaj);
	
	for(n=0;n<id_length;n++)
		fread( &byte, 1, 1, file );	// image description
	// color map data could be here
//	data = (unsigned char*) malloc( width * height * channels );
	data = (unsigned char*) malloc( width * height * 4 );
	for(y=0;y<height;y++)
	{
		for(x=0;x<width;x++)
		{
			n=(y*width+x)*4;
			if(channels>1)
			{
				fread(&byte, 1, 1, file );
				data[n+2]=byte;
				fread(&byte, 1, 1, file );
				data[n+1]=byte;
			}
			fread(&byte, 1, 1, file );
			data[n+0]=byte;
			if(channels==4)
			{
				fread(&byte, 1, 1, file );
				data[n+3]=byte;
			}
			else
			{
				data[n+3]=255;
				if(data[n+0]==128 && data[n+1]==0 && data[n+2]==128)
					data[n+3]=0;
			}
		}
	}
	
	channels=4;
	
	// allocate buffer
	// read texture data
	fclose( file );
	glEnable(GL_TEXTURE_2D);
	// allocate a texture name
	if(handle==0)
		glGenTextures( 1, &handle );
	// select our current texture
	glBindTexture( GL_TEXTURE_2D, handle );
	// select modulate to mix texture with color for shading
	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

	// normal (trilinear, mipmapped filtering)
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
/*
	// Anisotropic filtering
	float anisotropy_level;
	float max_anisotropy_level;
	if(mode & TEXTURE_ANISOTROPIC) // anisotropic
	{
		anisotropy_level=4;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_anisotropy_level);
		if(anisotropy_level>max_anisotropy_level) // If 4x anisotropy is not supported, disable it completely, since 2x is crappy anyways
			anisotropy_level=0;
//			anisotropy_level=max_anisotropy_level;
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy_level);
	}
*/
	if(mode & TEXTURE_NOMIPMAPS) // no mipmaps
	{
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	}
	if(mode & TEXTURE_NOFILTER) // no filtering (excludes mipmaps too)
	{
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	}

	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );


	if(mode & TEXTURE_NOREPEAT) // repeat
	{
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE ); // should check for EXT_texture_edge_clamp support first
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	}

	if(channels==3)
		gluBuild2DMipmaps( GL_TEXTURE_2D, channels, width, height, GL_RGB, GL_UNSIGNED_BYTE, data );
	if(channels==4)
		gluBuild2DMipmaps( GL_TEXTURE_2D, channels, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data );

	free( data );

//	handle=texture;

	return true;
}

void Texture::CreateFromData(BYTE *data, int width, int height, unsigned char mode)
{
	// allocate a texture name
	if(handle==0)
		glGenTextures( 1, &handle );
	// select our current texture
	glBindTexture( GL_TEXTURE_2D, handle );
	// select modulate to mix texture with color for shading
	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

	// normal (trilinear, mipmapped filtering)
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	if(mode & TEXTURE_NOMIPMAPS) // no mipmaps
	{
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	}
	if(mode & TEXTURE_NOFILTER) // no filtering (excludes mipmaps too)
	{
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	}

	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

	if(mode & TEXTURE_NOREPEAT) // no repeat
	{
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
	}

//	if(channels==3)
//		gluBuild2DMipmaps( GL_TEXTURE_2D, 3, width, height, GL_RGB, GL_UNSIGNED_BYTE, data );
//	if(channels==4)
	gluBuild2DMipmaps( GL_TEXTURE_2D, 4, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data );
//	glTexImage2D(somethingsomething?);
}

GLuint Texture::getHandle() {return handle;}

