#ifndef gear_effect_h
#define gear_effect_h

#include "musagi.h"
#include "dui.h"

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

	~gear_effect()
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

	virtual void DoInterface(DUI *dui)=0;
	
	void DefaultWindow(DUI *dui, int x, int y)
	{
		dui->NameSpace(this+inst_id);
//		dui->StartFlatWindow(x, y, z, 400, 30, dui->palette[4]);
		dui->StartFlatWindow(x, y, 400, 30, "gefwin");
		DoInterface(dui);
		dui->EndWindow();
		dui->NameSpace(NULL);
		inst_id++;
	};
	
	virtual bool Save(FILE *file)=0;

	virtual bool Load(FILE *file, int fversion)=0;
};

#endif

