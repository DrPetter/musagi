#ifndef gear_instrument_h
#define gear_instrument_h

#include "musagi.h"
#include "dui.h"

class gear_ichannel
{
public:
	bool queued_release;
	int release_delay;
	bool queued_slide;
	int slide_delay;
	int slide_target;
	int slide_length;
	int age;

	bool active;
	int note;

	gear_ichannel()
	{
		active=false;
		note=-1;
		queued_release=false;
		queued_slide=false;
		age=0;
	};

	~gear_ichannel()
	{
	};

	int UpdateBuffer(StereoBufferP *buffer, int size)
	{
		age++;
		int channels=0;
//		if(active)
//		{
			if(buffer->undefined)
			{
				ZeroBuffer(buffer->left, size);
				ZeroBuffer(buffer->right, size);
				buffer->undefined=false;
			}
			channels=RenderBuffer(buffer, size);
//		}
//		else
//		if(!active)
//			return 0; // no output
			
		if(queued_release)
		{
			release_delay-=size*16/GetTempo();
			if(release_delay<=0)
			{
				CRelease();
				queued_release=false;
			}
		}

		if(queued_slide)
		{
			slide_delay-=size*16/GetTempo();
			if(slide_delay<=0)
			{
				Slide(slide_target, slide_length*GetTempo()/16);
				queued_slide=false;
			}
		}

		return channels;
	};

	void CTrigger(int tnote, float tvolume, int tduration, int tslidenote, int tslidelength)
	{
//		LogPrint("channel [%.8X]: trigger", this);
//		LogPrint("channel: triggered");

		age=0;
		note=tnote;
		if(tduration<0)
			queued_release=false;
		else
		{
			release_delay=tduration;
			queued_release=true;
		}
		if(tslidelength<0)
			queued_slide=false;
		else
		{
			slide_delay=tduration;
			release_delay+=tslidelength;
			slide_length=tslidelength;
			slide_target=tslidenote;
			queued_slide=true;
		}
//		LogPrint("channel: calling child Trigger()");
		Trigger(tnote, tvolume);
	};
	
	void CRelease()
	{
		note=-1;
		Release();
	};

	virtual int RenderBuffer(StereoBufferP *buffer, int size)=0; // add audio samples to specified buffer
	
	virtual void Trigger(int tnote, float tvolume)=0;

	virtual void Release()=0;

	virtual void Slide(int tnote, int length)=0;
};

class gear_instrument
{
protected:
	StereoBufferP buffer;
	
	gear_ichannel *channels[64];
	int numchannels;
	
	char name[64];
	char visname[64];
	bool atomic;

	int inst_id;
	int gid;

public:
	gear_instrument()
	{
//		LogPrint("gear_instrument: init");

		buffer.left=NULL;
		buffer.right=NULL;
		buffer.size=0;
		
		numchannels=0;

		for(int i=0;i<64;i++)
		{
			name[i]=0;
			visname[i]=0;
		}
		name[0]='\0';
		visname[0]='\0';
		atomic=false;
		inst_id=0;
		gid=0;
	};

	~gear_instrument()
	{
		if(buffer.left)
			free(buffer.left);
		if(buffer.right)
			free(buffer.right);

		for(int i=0;i<numchannels;i++)
			delete channels[i];
	};

	int GetGid()
	{
		return gid;
	};

	void SetGid(int id)
	{
		gid=id;
	};

	void PrepGUI()
	{
		inst_id=0;
	};
	
	char* Name()
	{
		return visname;
	};
	
	bool Atomic()
	{
		return atomic;
	};

	void ManageBuffer(int size) // allocate new buffer if size doesn't match
	{
		if(buffer.size!=size)
		{
			if(buffer.left)
				free(buffer.left);
			if(buffer.right)
				free(buffer.right);
			buffer.size=size;
			buffer.left=(float*)malloc(buffer.size*sizeof(float));
			buffer.right=(float*)malloc(buffer.size*sizeof(float));
		}
	};

	StereoBufferP RenderBuffer(int size)
	{
//		LogPrint("gear_instrument: RenderBuffer()");
		ManageBuffer(size);

		buffer.undefined=true;
		
		int bchannels=0;
		for(int i=0;i<numchannels;i++)
		{
			int bch=channels[i]->UpdateBuffer(&buffer, size);
//			if(bch>0)
//				LogPrint("instrument: channel %i's buffer generated data", i);
			if(bch>bchannels)
				bchannels=bch;
		}

		StereoBufferP out;
		switch(bchannels)
		{
		case 0: // silent
			out.left=NULL;
			out.right=NULL;
			out.mono=true;
			out.size=0;
			break;
		case 1: // mono, duplicate channel to get stereo output
		{
			out.left=buffer.left;
			float *bp=buffer.right;
			float *sp=buffer.left;
			for(int i=0;i<size;i+=8)
			{
				(*bp++)=*sp++;
				(*bp++)=*sp++;
				(*bp++)=*sp++;
				(*bp++)=*sp++;
				(*bp++)=*sp++;
				(*bp++)=*sp++;
				(*bp++)=*sp++;
				(*bp++)=*sp++;
			}
			out.right=buffer.right;
			out.mono=true;
			out.size=size;
			break;
		}
		case 2: // stereo
			out.left=buffer.left;
			out.right=buffer.right;
			out.mono=false;
			out.size=size;
			break;
		}
		
		return out;
	};
	
//	virtual int UpdateBuffer(int size)=0; // fill buffer with data, return number of channels filled (0=none, 1=mono (left), 2=stereo)

	void Trigger(int tnote, float tvolume) // start playing given note
	{
//		LogPrint("instrument [%.8X]: trigger", this);
		Trigger(tnote, tvolume, -1, -1, -1);
	};

	void Trigger(int tnote, float tvolume, int tduration)
	{
		Trigger(tnote, tvolume, tduration, -1, -1);
	};
	
	void Trigger(int tnote, float tvolume, int tduration, int tslidenote, int tslidelength) // start playing given note (negative duration is undetermined/infinite)
	{
		int maxage=0;
		int oldest=0;
		for(int i=0;i<numchannels;i++)
		{
			if(!channels[i]->active || numchannels==1)
			{
//				LogPrint("instrument: trigger channel %i", i);
//				LogPrint("instrument [%.8X]: trigger channel %i", this, i);
				channels[i]->CTrigger(tnote, tvolume, tduration, tslidenote, tslidelength);
				return;
			}
			else if(channels[i]->age>maxage)
			{
				maxage=channels[i]->age;
				oldest=i;
			}
		}
		// unable to locate an available channel, reuse oldest
//		LogPrint("instrument [%.8X]: trigger channel %i (recycled, all channels active)", this, oldest);
		channels[oldest]->CTrigger(tnote, tvolume, tduration, tslidenote, tslidelength);
	};
	
	void Release(int rnote) // release given note
	{
		for(int i=0;i<numchannels;i++)
			if(channels[i]->note==rnote)
				channels[i]->CRelease();
	};
	
	void ReleaseAll() // release any playing notes
	{
		for(int i=0;i<numchannels;i++)
			if(channels[i]->note!=-1)
				channels[i]->CRelease();
	};

	void StopChannels() // abruptly stop all channels from playing (as long as they are culling on !active... but they should be ;))
	{
		for(int i=0;i<numchannels;i++)
			channels[i]->active=false;
	};
	
	bool IsTriggered()
	{
		for(int i=0;i<numchannels;i++)
			if(channels[i]->active)
//			if(channels[i]->note!=-1)
				return true;
		return false;
	};
	
	virtual void DoInterface(DUI *dui)=0;
	
	void DefaultWindow(DUI *dui, int x, int y)
	{
//		LogPrint("gear_instrument: DefaultWindow()");
		dui->NameSpace(this+inst_id);
//		dui->StartFlatWindow(x, y, z, 400, 50, dui->palette[4]);
		dui->StartFlatWindow(x, y, 400, 50, "ginwin");
		DoInterface(dui);
		dui->EndWindow();
		dui->NameSpace(NULL);
		inst_id++;
//		LogPrint("gear_instrument: DefaultWindow() done");
	};
	
	virtual bool Save(FILE *file)=0;

	virtual bool Load(FILE *file)=0;
};

#endif

