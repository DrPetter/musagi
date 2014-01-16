#ifndef gear_effect_h
#define gear_effect_h

#include "musagi.h"

class gear_effect
{
protected:
	char name[64];
	int inst_id;

public:
	gear_effect()
	{
		for(int i=0;i<64;i++)
			name[i]=0;
		name[0]='\0';
		inst_id=0;
	};

	virtual ~gear_effect()
	{
	};

	void PrepGUI()
	{
		inst_id=0;
	};
	
	char* Name()
	{
		return name;
	};

	virtual int ProcessBuffer(StereoBufferP *buffer)=0;

	virtual bool Load(FILE *file, int fversion)=0;
};

#endif

