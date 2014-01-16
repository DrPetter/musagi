DWORD hsv_to_rgb(int h, int s, int v)
{
	float fc[3];
	fc[0]=0.0f;
	fc[1]=0.0f;
	fc[2]=0.0f;
	if(h>251) h=0;
	int wh=h%42;
	float fh=(float)wh/42;
	float ifh=1.0f-fh;
	switch(h/42)
	{
	case 0: // red->yellow
		fc[0]=1.0f;
		fc[1]=fh;
		break;
	case 1: // green->yellow
		fc[1]=1.0f;
		fc[0]=ifh;
		break;
	case 2: // green->cyan
		fc[1]=1.0f;
		fc[2]=fh;
		break;
	case 3: // blue->cyan
		fc[2]=1.0f;
		fc[1]=ifh;
		break;
	case 4: // blue->purple
		fc[2]=1.0f;
		fc[0]=fh;
		break;
	case 5: // red->purple
		fc[0]=1.0f;
		fc[2]=ifh;
		break;
	}
	float fs=(float)s/255;
	for(int i=0;i<3;i++)
		fc[i]=1.0f-(1.0f-fc[i])*fs;
	float fv=(float)v/255;
	for(int i=0;i<3;i++)
		fc[i]*=fv;
	DWORD color=0xFF000000;
	color|=(int)(fc[0]*255)<<16;
	color|=(int)(fc[1]*255)<<8;
	color|=(int)(fc[2]*255);
	return color;
}

int color_val(DWORD color)
{
	float fc[3];
	fc[0]=(float)((color>>16)&0xFF)/255;
	fc[1]=(float)((color>>8)&0xFF)/255;
	fc[2]=(float)(color&0xFF)/255;
	// find minimum channel
	int imax=-1;
	float fmax=-1.0f;
	for(int i=0;i<3;i++)
		if(fc[i]>fmax) { imax=i; fmax=fc[i]; }
	return (int)(fmax*255);
}

int color_sat(DWORD color)
{
	float fc[3];
	fc[0]=(float)((color>>16)&0xFF)/255;
	fc[1]=(float)((color>>8)&0xFF)/255;
	fc[2]=(float)(color&0xFF)/255;
	// find maximum channel
	int imax=-1;
	float fmax=-1.0f;
	for(int i=0;i<3;i++)
		if(fc[i]>fmax) { imax=i; fmax=fc[i]; }
	if(fmax<0.001f) // black has zero saturation
		return 0;
	// normalize, max channel is 1.0
	for(int i=0;i<3;i++)
		fc[i]/=fmax;
	// find minimum channel
	int imin=-1;
	float fmin=2.0f;
	for(int i=0;i<3;i++)
		if(fc[i]<fmin) { imin=i; fmin=fc[i]; }
	// span between 1.0 and minimum channel is saturation
	return (int)(255-fmin*255);
}

int color_hue(DWORD color)
{
	float fc[3];
	fc[0]=(float)((color>>16)&0xFF)/255;
	fc[1]=(float)((color>>8)&0xFF)/255;
	fc[2]=(float)(color&0xFF)/255;
	// find minimum channel
	int imin=-1;
	float fmin=2.0f;
	for(int i=0;i<3;i++)
		if(fc[i]<fmin) { imin=i; fmin=fc[i]; }
	// subtract minimum channel, leaving two non-zero channels
	for(int i=0;i<3;i++)
		fc[i]-=fmin;
	// find maximum channel
	int imax=-1;
	float fmax=0.0f;
	for(int i=0;i<3;i++)
		if(fc[i]>fmax) { imax=i; fmax=fc[i]; }
	if(fmax<0.001f) // no saturation -> undefined hue
		return 0;
	// normalize, giving one channel==1, one==0 and one in 0...1
	for(int i=0;i<3;i++)
		fc[i]/=fmax;
	int imid=imin+1;
	if(imid>2) imid=0;
	if(imid==imax) imid++;
	if(imid>2) imid=0;
	int fraction=(int)(fc[imid]*41); // fraction of midchannel/maxchannel
	int ifraction=41-fraction;
	int interval=42;
	switch(imax*10+imid) // determine hue interval
	{
	case 01: // red->yellow
		return fraction;
		break;
	case 10: // green->yellow
		return 1*interval+ifraction;
		break;
	case 12: // green->cyan
		return 2*interval+fraction;
		break;
	case 21: // blue->cyan
		return 3*interval+ifraction;
		break;
	case 20: // blue->purple
		return 4*interval+fraction;
		break;
	case 02: // red->purple
		return 5*interval+ifraction;
		break;
	}
	return 0; // undefined (shouldn't happen)
}

