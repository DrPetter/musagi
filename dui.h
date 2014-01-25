#ifndef dui_h
#define dui_h

#include <stdio.h>
#include "Log.h"
#include "Texture.h"
#include "DPInput.h"

#include "glkit_global.h"

#include "fileselector.h"

struct dui_State
{
	bool active;
	bool clicked; // LMB pressed then released over area
	bool contextmenu; // RMB pressed then released over area
	// relative mouse coords
	int rmx;
	int rmy;
	// mouse delta
	int mdx;
	int mdy;
};

struct dui_MouseState
{
	bool lmbdown, lmbclick, lmbrelease, dlmbclick;
	bool rmbdown, rmbclick, rmbrelease;
	bool mmbdown, mmbclick, mmbrelease;
};

class DUI
{
public:
	char active_element[64]; // preserved between frames
	char active_window[64]; // preserved between frames
	char current_window[64]; // preserved between frames
	char namestring[16]; // for namespaces (to avoid conflict between instances of the same class)
	char curname[64];
	
	int mouseregion;
	bool lmbdown, lmbclick, lmbrelease, dlmbclick;
	bool rmbdown, rmbclick, rmbrelease;
	bool mmbdown, mmbclick, mmbrelease;
	int mousex, mousey;
	int mousepx, mousepy;
	int mousedx, mousedy;
	int mousewheel;
	int dlmb_counter;
	int cmx, cmy;
	int anchorx, anchory;
	float anchorfx, anchorfy;
	int anchorix, anchoriy;
	
	int tmousex, tmousey;
	bool tlmbclick, trmbclick, tmmbclick;
	
	bool key_a, key_c, key_v;
	bool shift_down;
	bool ctrl_down;
	
	int window_level;
	int window_x[32];
	int window_y[32];
	int window_sx[32];
	int window_sy[32];
	int window_w[32];
	int window_h[32];
	char winname[32][64];

	char mousewindow[64];
	int mf_settled;
	dui_MouseState realmouse[2];
	int mouseprivilege;
	
	char *estring;
	char eoldstring[128];
	bool editstring;

	int gui_winx;
	int gui_winy;
	
	dui_State state; // state for current component
	dui_State mstate; // state for current menu

	float joyaxis[4];

	// for part icons
	int *part_ypos;
	int *part_start;
	float part_zoom;
	int part_moy;
	int part_mox;
	bool part_is;
//	int part_ystart;

	bool rcmenu;
	int rcmenu_type;
	int rcmenu_coffset;
	int rcmenu_x;
	int rcmenu_y;
	float *rcmenu_pval;

	int tooltips_on;
	int popup_counter;
	char popup_text[128];

	Color palette[32];
	
//	Texture plutt;
	Texture font;
	Texture knobe[4];
	Texture knobtop;
	Texture knobdot;
	Texture knobshadow;
	Texture buttonup;
	Texture buttondown;
//	Texture mouse;
	Texture scrollbar_h1;
	Texture scrollbar_h2;
	Texture scrollbar_hs;
	Texture scrollbar_v1;
	Texture scrollbar_v2;
	Texture scrollbar_vs;
	Texture brushed_metal;
	Texture wood_walnut;
	Texture vglare;
	Texture led[4];
	Texture logo;
	Texture vgradient;
	Texture icon_resize;
	
	int font_width[110];
	int font_offset[110];
	
	glkit_mouse *glkmouse;

	DUI()
	{
		active_element[0]='\0';
		active_window[0]='\0';
		
		mousewindow[0]='\0';
		mf_settled=2;
		ClearMouse();
		StoreMouse(0);
		StoreMouse(1);

		part_is=false;
		editstring=false;
		dlmb_counter=0;
		
		rcmenu=false;
		tlmbclick=false;
		trmbclick=false;
		tmmbclick=false;
		tmousex=-1;
		tmousey=-1;
//		calibration=false;

		tooltips_on=1;
		popup_counter=0;
		popup_text[0]='\0';

		FileSelectorInit();

		LogPrint("dui: loading textures");
		font.LoadTGA("skin/font0.tga", TEXTURE_NOFILTER|TEXTURE_NOMIPMAPS);
		DetectFontWidths("skin/font0.tga");

//		plutt.LoadTGA("skin/plutt.tga", TEXTURE_NOFILTER|TEXTURE_NOMIPMAPS);
		knobe[0].LoadTGA("skin/knob1_edge0.tga", TEXTURE_NOFILTER|TEXTURE_NOMIPMAPS);
		knobe[1].LoadTGA("skin/knob1_edge1.tga", TEXTURE_NOFILTER|TEXTURE_NOMIPMAPS);
		knobe[2].LoadTGA("skin/knob1_edge2.tga", TEXTURE_NOFILTER|TEXTURE_NOMIPMAPS);
		knobe[3].LoadTGA("skin/knob1_edge3.tga", TEXTURE_NOFILTER|TEXTURE_NOMIPMAPS);
		knobtop.LoadTGA("skin/knob1_top.tga", TEXTURE_NOFILTER|TEXTURE_NOMIPMAPS);
		knobshadow.LoadTGA("skin/knob1_shadow.tga", TEXTURE_NOFILTER|TEXTURE_NOMIPMAPS);
		knobdot.LoadTGA("skin/knob1_dot.tga", TEXTURE_NOMIPMAPS);
		buttonup.LoadTGA("skin/button1_up.tga", TEXTURE_NOFILTER|TEXTURE_NOMIPMAPS);
		buttondown.LoadTGA("skin/button1_down.tga", TEXTURE_NOFILTER|TEXTURE_NOMIPMAPS);
//		mouse.LoadTGA("skin/mouse0.tga", TEXTURE_NOMIPMAPS);
		scrollbar_h1.LoadTGA("skin/slidebar_h1.tga", TEXTURE_NOFILTER|TEXTURE_NOMIPMAPS);
		scrollbar_h2.LoadTGA("skin/slidebar_h2.tga", TEXTURE_NOFILTER|TEXTURE_NOMIPMAPS);
		scrollbar_hs.LoadTGA("skin/slider1_h.tga", TEXTURE_NOFILTER|TEXTURE_NOMIPMAPS);
		scrollbar_v1.LoadTGA("skin/slidebar_v1.tga", TEXTURE_NOFILTER|TEXTURE_NOMIPMAPS);
		scrollbar_v2.LoadTGA("skin/slidebar_v2.tga", TEXTURE_NOFILTER|TEXTURE_NOMIPMAPS);
		scrollbar_vs.LoadTGA("skin/slider1_v.tga", TEXTURE_NOFILTER|TEXTURE_NOMIPMAPS);
		brushed_metal.LoadTGA("skin/mainbg.tga", TEXTURE_NOFILTER|TEXTURE_NOMIPMAPS);
//		wood_walnut.LoadTGA("skin/walnut2.tga", TEXTURE_NOFILTER|TEXTURE_NOMIPMAPS);
		wood_walnut.LoadTGA("skin/titlebar.tga", TEXTURE_NOFILTER|TEXTURE_NOMIPMAPS|TEXTURE_NOREPEAT);
		vglare.LoadTGA("skin/vglare.tga", TEXTURE_NOFILTER|TEXTURE_NOMIPMAPS|TEXTURE_NOREPEAT);
		led[0].LoadTGA("skin/led1.tga", TEXTURE_NOFILTER|TEXTURE_NOMIPMAPS);
		led[1].LoadTGA("skin/led2.tga", TEXTURE_NOFILTER|TEXTURE_NOMIPMAPS);
		led[2].LoadTGA("skin/led3.tga", TEXTURE_NOFILTER|TEXTURE_NOMIPMAPS);
		led[3].LoadTGA("skin/led4.tga", TEXTURE_NOFILTER|TEXTURE_NOMIPMAPS);
//		logo.LoadTGA("skin/logo3.tga", TEXTURE_NOFILTER|TEXTURE_NOMIPMAPS);
		logo.LoadTGA("skin/logomask.tga", TEXTURE_NOFILTER|TEXTURE_NOMIPMAPS);
		vgradient.LoadTGA("skin/vgradient.tga", TEXTURE_NOFILTER|TEXTURE_NOMIPMAPS);
		icon_resize.LoadTGA("skin/icon_resize.tga", TEXTURE_NOFILTER|TEXTURE_NOMIPMAPS);

		palette[0].r=0.5f;
		palette[0].g=0.0f;
		palette[0].b=0.0f;
	
		palette[1].r=0.0f;
		palette[1].g=0.0f;
		palette[1].b=0.5f;
	
		palette[2].r=1.0f;
		palette[2].g=1.0f;
		palette[2].b=1.0f;
	
		palette[3].r=1.0f;
		palette[3].g=1.0f;
		palette[3].b=0.0f;
	
		palette[4].r=0.4f;
		palette[4].g=0.4f;
		palette[4].b=0.4f;
	
		palette[5].r=0.45f;
		palette[5].g=0.45f;
		palette[5].b=0.45f;
	
		palette[6].r=0.0f;
		palette[6].g=0.0f;
		palette[6].b=0.0f;
	
		palette[7].r=0.5f;
		palette[7].g=0.5f;
		palette[7].b=0.5f;
	
		palette[8].r=160.0f/255;
		palette[8].g=160.0f/255;
		palette[8].b=160.0f/255;
	
		palette[9].r=1.0f;
		palette[9].g=1.0f;
		palette[9].b=1.0f;
	
		palette[10].r=0.75f;
		palette[10].g=0.75f;
		palette[10].b=0.75f;
	
		palette[11].r=1.0f;
		palette[11].g=0.0f;
		palette[11].b=0.0f;
	
		palette[12].r=0.0f;
		palette[12].g=1.0f;
		palette[12].b=0.0f;
	
		palette[13].r=0.0f;
		palette[13].g=0.0f;
		palette[13].b=1.0f;
		
		palette[14].r=0.3f;
		palette[14].g=0.3f;
		palette[14].b=0.3f;

		palette[15].r=0.6f;
		palette[15].g=0.4f;
		palette[15].b=0.4f;

		palette[16].r=0.7f;
		palette[16].g=0.7f;
		palette[16].b=0.7f;

		palette[17].r=0.55f;
		palette[17].g=0.55f;
		palette[17].b=0.55f;
	};
	
	~DUI()
	{
	};

	// append namespace name to a string	
	void SpaceName()
	{
		int last=strlen(curname);
		if(last>63-9)
			last=63-9;
		for(int i=0;i<9;i++)
			curname[last++]=namestring[i];
	};

	void StoreMouse(int i)
	{
		realmouse[i].lmbdown=lmbdown;
		realmouse[i].rmbdown=rmbdown;
		realmouse[i].mmbdown=mmbdown;
		realmouse[i].lmbrelease=lmbrelease;
		realmouse[i].rmbrelease=rmbrelease;
		realmouse[i].mmbrelease=mmbrelease;
		realmouse[i].lmbclick=lmbclick;
		realmouse[i].rmbclick=rmbclick;
		realmouse[i].mmbclick=mmbclick;
		realmouse[i].dlmbclick=dlmbclick;
//		LogPrint("dui: StoreMouse(%i) - lmbclick=%i", i, lmbclick);
	};
	
	void LoadMouse(int i)
	{
		lmbdown=realmouse[i].lmbdown;
		rmbdown=realmouse[i].rmbdown;
		mmbdown=realmouse[i].mmbdown;
		lmbrelease=realmouse[i].lmbrelease;
		rmbrelease=realmouse[i].rmbrelease;
		mmbrelease=realmouse[i].mmbrelease;
		lmbclick=realmouse[i].lmbclick;
		rmbclick=realmouse[i].rmbclick;
		mmbclick=realmouse[i].mmbclick;
		dlmbclick=realmouse[i].dlmbclick;
//		LogPrint("dui: LoadMouse(%i) - lmbclick=%i", i, lmbclick);
	};

	void ClearMouse()
	{
		lmbdown=false;
		rmbdown=false;
		mmbdown=false;
		lmbrelease=false;
		rmbrelease=false;
		mmbrelease=false;
		lmbclick=false;
		rmbclick=false;
		mmbclick=false;
		dlmbclick=false;
//		LogPrint("dui: ClearMouse()");
	};

	bool MouseInZone(int x, int y, int w, int h)
	{
		if(mousex>=x && mousey>=y && mousex<x+w && mousey<y+h)
			return true;
		return false;
	};
	
	void Update(DPInput *input)
	{
//		LogPrint("\ndui: Update() *****");
		
		LoadMouse(0);
		StoreMouse(1);
/*		if(lmbclick || rmbclick || mmbclick)
		{
			mousex=cmx;
			mousey=cmy;
		}
*/		
		mousex=glkmouse->glk_mousex;
		mousey=glkmouse->glk_mousey;
		mousedx=mousex-mousepx;
		mousedy=mousey-mousepy;
		mousepx=mousex;
		mousepy=mousey;
		lmbclick=false;
		rmbclick=false;
		mmbclick=false;
		dlmbclick=false;
		lmbrelease=false;
		rmbrelease=false;
		mmbrelease=false;
		if(input->KeyPressed(SDLK_LALT))
		{
			glkmouse->glk_mousemiddle=glkmouse->glk_mouseleft;
			glkmouse->glk_mouseleft=false;
		}
		if(!lmbdown && glkmouse->glk_mouseleft) lmbclick=true;
		if(lmbdown && !glkmouse->glk_mouseleft) lmbrelease=true;
		lmbdown=glkmouse->glk_mouseleft;
		if(!rmbdown && glkmouse->glk_mouseright) rmbclick=true;
		if(rmbdown && !glkmouse->glk_mouseright) rmbrelease=true;
		rmbdown=glkmouse->glk_mouseright;
		if(!mmbdown && glkmouse->glk_mousemiddle) mmbclick=true;
		if(mmbdown && !glkmouse->glk_mousemiddle) mmbrelease=true;
		mmbdown=glkmouse->glk_mousemiddle;
		mousewheel=glkmouse->glk_mousewheel;

		if(mousedx!=0 || mousedy!=0 || lmbclick || rmbclick || mmbclick)
		{
			popup_counter=0;
			popup_text[0]='\0';
		}

//		if(abs(mousedx)>1 || abs(mousedy)>1)
		if(mousedx!=0 || mousedy!=0)
			dlmb_counter=0;
		else
			popup_counter++;
		if(lmbclick)
		{
			if(dlmb_counter>0)
			{
				dlmbclick=true;
				dlmb_counter=0;
			}
			else
				dlmb_counter=20;
		}
		if(dlmb_counter>0)
			dlmb_counter--;

//		if(!lmbdown && !rmbdown && !mmbdown)
		if(lmbclick || rmbclick || mmbclick)
		{
			mouseregion=0;
			mousex=glkmouse->glk_cmx;
			mousey=glkmouse->glk_cmy;
//			cmx=mousex;
//			cmy=mousey;
		}

		if(rcmenu) // repress mouse events during right-click menu... somewhat ugly
		{
			tmousex=mousex;
			tmousey=mousey;
			tlmbclick=lmbclick;
			trmbclick=rmbclick;
			tmmbclick=mmbclick;
			mousex=-1;
			mousey=-1;
			lmbclick=false;
			rmbclick=false;
			mmbclick=false;
//			lmbdown=false;
//			rmbdown=false;
//			mmbdown=false;
		}

		shift_down=input->KeyPressed(SDLK_LSHIFT)||input->KeyPressed(SDLK_RSHIFT);
		ctrl_down=input->KeyPressed(SDLK_LCTRL)||input->KeyPressed(SDLK_RCTRL);
		if(editstring)
		{
			key_a=false;
			key_c=false;
			key_v=false;
		}
		else
		{
			key_a=input->KeyPressed(SDLK_a);
			key_c=input->KeyPressed(SDLK_c);
			key_v=input->KeyPressed(SDLK_v);
		}
		
		joyaxis[0]+=((-input->JoyAxis(0)*0.5f+0.5f)-joyaxis[0])*0.2f;
		joyaxis[1]+=((-input->JoyAxis(1)*0.5f+0.5f)-joyaxis[1])*0.2f;
//		joyaxis[1]=-input->JoyAxis(1)*0.5f+0.5f;

		window_level=0;
		window_x[0]=0;
		window_y[0]=0;
		gui_winx=0;
		gui_winy=0;

		if(editstring)
		{
			char glkkey=glkitGetKey();
			if(glkkey!=0)
			{
				int slen=(int)strlen(estring);
//				if(glkkey>='a' && glkkey<='z') // forbid lower case ascii
//					glkkey-='a'-'A';
				if(slen<48 && ((glkkey>=' ' && glkkey<='~'+1) || glkkey=='Å' || glkkey=='Ä' || glkkey=='Ö' || glkkey=='å' || glkkey=='ä' || glkkey=='ö'))
				{
//					if(glkkey>='A' && glkkey<='Z') // upper case -> lower case
//						glkkey+='a'-'A';
					estring[slen-1]=glkkey;
					estring[slen+0]='_';
					estring[slen+1]='\0';
				}
				if(slen>1 && glkkey==8) // backspace
				{
					estring[slen-2]='_';
					estring[slen-1]='\0';
				}
				if(glkkey==13) // enter
					FinishStringEdit();
				if(input->KeyPressed(SDLK_ESCAPE))
					CancelStringEdit();
			}
			if(lmbclick || rmbclick || mmbclick)
				FinishStringEdit();
			glkitResetKey();
		}

		if(mf_settled==0)
		{
//			LogPrint("dui: mf_settled, mousewindow==\"%s\" ------", mousewindow);
			StoreMouse(0);
			mousex=glkmouse->glk_cmx;
			mousey=glkmouse->glk_cmy;
//			mousex=mousepx;
//			mousey=mousepy;
//			mousepx-=mousedx;
//			mousepy-=mousedy;
		}
		else
		{
			StoreMouse(0);
			StoreMouse(1);
		}
		mf_settled++;

/*		if(!realmouse[1].lmbclick && !realmouse[1].rmbclick && !realmouse[1].mmbclick)
		{
		}
		else
*/
		mouseprivilege=0;
		ClearMouse();

		namestring[0]='\0';
	};
	
	float* DoOverlay(int& choice) // returns right-click menu choice, if any.... uh, that's ugly :P
	{
		if(rcmenu)
		{
			float *value=DoRCMenu(choice);
			if(lmbclick || rmbclick || mmbclick)
			{
				rcmenu=false;
				tlmbclick=false;
				trmbclick=false;
				tmmbclick=false;
				tmousex=-1;
				tmousey=-1;
			}
			return value;
		}
		else if(tooltips_on)
		{
			if(popup_counter>40 && popup_text[0]!='\0')
			{
				int width=VStringLength(popup_text);
				int x=mousex;
				if(x+width+6>=glkitGetWidth())
					x=glkitGetWidth()-width-6;
				DrawBar(x, mousey-12, width+6, 12, palette[9]);
				DrawBox(x, mousey-12, width+6, 12, palette[6]);
				DrawBarAlpha(x, mousey, width+6, 2, palette[6], 0.3f);
				DrawText(x+3, mousey-10, palette[6], popup_text);
			}
		}
		return NULL;
	};

	void EditString(char *string)
	{
		glkitResetKey();
		if(editstring)
			FinishStringEdit();
		strcpy(eoldstring, string);
		estring=string;
		editstring=true;
		strcat(estring, "_");
	};

	void FinishStringEdit()
	{
		int slen=(int)strlen(estring);
		estring[slen-1]='\0';
		editstring=false;
	};

	void CancelStringEdit()
	{
		strcpy(estring, eoldstring);
//		int slen=(int)strlen(estring);
//		estring[slen-1]='\0';
		editstring=false;
	};

	void PopupInstrumentSelect()
	{
		rcmenu=true;
		rcmenu_type=1;
		rcmenu_coffset=10;
		rcmenu_x=mousex-10;
		rcmenu_y=mousey-4;
		rcmenu_pval=(float*)333; // bogus, not used
	};

	float* DoRCMenu(int& choice)
	{
		mousex=tmousex;
		mousey=tmousey;
		lmbclick=tlmbclick;
		rmbclick=trmbclick;
		mmbclick=tmmbclick;
		int width=0;
		int numitems=0;
		char sitem[10][32];
		if(rcmenu_type==0)
		{
			width=50;
			numitems=3;
			strcpy(sitem[0], "record");
			strcpy(sitem[1], "clear");
			strcpy(sitem[2], "default");
		}
		else if(rcmenu_type==1)
		{
			width=70;
			numitems=5;
			strcpy(sitem[0], "xnes");
			strcpy(sitem[1], "chip");
			strcpy(sitem[2], "vsampler");
			strcpy(sitem[3], "swave");
			strcpy(sitem[4], "protobass");
		}
		else
			return NULL;
		for(int i=0;i<numitems;i++)
		{
			bool cur=MouseInZone(rcmenu_x, rcmenu_y+i*10, width, 10);
			if(lmbclick)
			{
				if(cur)
				{
//					LogPrint("dui: rcmenu selected %i (%.8X)", i, rcmenu_pval);
					choice=i+rcmenu_coffset;
					return rcmenu_pval;
				}
			}
			else
			{
				if(cur)
				{
					DrawBar(rcmenu_x, rcmenu_y+i*10, width, 10, palette[8]);
					DrawText(rcmenu_x+3, rcmenu_y+i*10+1, palette[9], sitem[i]);
				}
				else
				{
					DrawBar(rcmenu_x, rcmenu_y+i*10, width, 10, palette[10]);
					DrawText(rcmenu_x+3, rcmenu_y+i*10+1, palette[6], sitem[i]);
				}
			}
		}
		DrawBox(rcmenu_x, rcmenu_y, width, numitems*10+1, palette[6]);
		DrawBarAlpha(rcmenu_x, rcmenu_y+numitems*10+1, width, 2, palette[6], 0.3f);
		return NULL;
	};

	void EnterWindow(char *winid, int x, int y, int sx, int sy, int w, int h)
	{
		window_level++;
		window_x[window_level]=x;
		window_y[window_level]=y;
		window_sx[window_level]=sx;
		window_sy[window_level]=sy;
		window_w[window_level]=w;
		window_h[window_level]=h;
		strcpy(winname[window_level], winid);
		strcpy(current_window, winid);
		gui_winx=x-sx;
		gui_winy=y-sy;
		int clipx=x;
		int clipy=y;
		int clipw=w;
		int cliph=h;
		int pw=window_level-1;
		if(pw>0)
		{
			if(clipx+clipw>window_x[pw]+window_w[pw]) clipw=window_x[pw]+window_w[pw]-clipx;
			if(clipy+cliph>window_y[pw]+window_h[pw]) cliph=window_y[pw]+window_h[pw]-clipy;
			if(clipx<window_x[pw])
			{
				clipw-=window_x[pw]-clipx;
				clipx=window_x[pw];
			}
			if(clipy<window_y[pw])
			{
				cliph-=window_y[pw]-clipy;
				clipy=window_y[pw];
			}
			if(clipw<0) clipw=0;
			if(cliph<0) cliph=0;
		}
//		glScissor(x, glkitGetHeight()-(y)-h, w, h);
		glScissor(clipx, glkitGetHeight()-(clipy)-cliph, clipw, cliph);
		glEnable(GL_SCISSOR_TEST);

		LoadMouse(0);
		bool mactivate=(!dlmbclick && (lmbclick||rmbclick||mmbclick) && MouseInZone(x, y, w, h));
		if(mactivate)
		{
//			LogPrint("dui: mactivate for window \"%s\" ------ %i%i%i", winid, lmbclick, mmbclick, rmbclick);
			mf_settled=0;
			strcpy(mousewindow, winid);
		}
		if(mf_settled>=1 && strcmp(mousewindow, winid)==0) // this window has mouse focus, load last frame's mouse data
		{
			LoadMouse(1);
			mouseprivilege++;
//			LogPrint("dui: window \"%s\" has mouse focus", winid);
		}
		else
			ClearMouse();
	};
	void LeaveWindow()
	{
		window_level--;
		if(window_level<0)
			window_level=0;
		strcpy(current_window, winname[window_level]);
		gui_winx=window_x[window_level]-window_sx[window_level];
		gui_winy=window_y[window_level]-window_sy[window_level];
		if(window_level==0)
			glDisable(GL_SCISSOR_TEST);
		else
//			glScissor(window_x[window_level], glkitGetHeight()-(window_x[window_level])-window_h[window_level], window_w[window_level], window_h[window_level]);
			glScissor(window_x[window_level], glkitGetHeight()-(window_y[window_level])-window_h[window_level], window_w[window_level], window_h[window_level]);
		mouseprivilege--;
		if(mouseprivilege<=0)
		{
			mouseprivilege=0;
			ClearMouse();
		}
	};
	
	void DeselectWindow()
	{
		active_window[0]='\0';
	};
	
	void NameSpace(void *identifier)
	{
		if(identifier==NULL)
			namestring[0]='\0';
		else
			sprintf(namestring, "%.8X", identifier);
//		LogPrint("Namespace is now: \"%s\"", namestring);
	};

	bool DoButtonLogic(int x, int y, int w, int h, float& value, char *idstr) // Use config file thing instead of manual coordinates/sizes?
	{
		// translate to global coordinates
		x+=gui_winx;
		y+=gui_winy;

		state.rmx=mousex-x;
		state.rmy=mousey-y;
		state.mdx=mousedx;
		state.mdy=mousedy;
		state.clicked=false;
		
//		LogPrint("dui: button, lmbclick=%i", lmbclick);

		bool activated=false; // To save a string comparison
		if(strcmp(idstr, active_element)!=0) // Inactive
		{
			if((lmbclick || rmbclick) && MouseInZone(x, y, w, h)) // Activated
			{
				activated=true;
				state.active=true;
				strcpy(active_element, idstr);
			}
		}
		else
			activated=true;

		if(activated) // Active
		{
			if((lmbclick || rmbclick) && !MouseInZone(x, y, w, h))
			{
				state.active=false;
				active_element[0]='\0';
				part_is=false;
				return false; // Deactivated
			}
			if(lmbclick && MouseInZone(x, y, w, h)) // LMB down
			{
//				part_ystart=(*part_ypos);
				part_mox=0;
				part_moy=0;
			}
			if(lmbrelease)
			{
				if(MouseInZone(x, y, w, h)) // Clicked
				{
					state.clicked=true;
					active_element[0]='\0';
				}
				else // Released outside
				{
					state.active=false;
					active_element[0]='\0';
				}
			}
			if(lmbdown) // Dragging knob (where applicable)
			{
				if(shift_down)
					value-=mousedy*0.001f;
				else
					value-=mousedy*0.01f;
				if(value<0.0f) value=0.0f;
				if(value>1.0f) value=1.0f;
				
				// dragging part icon (where applicable)
				if(part_is)
				{
					part_moy+=mousedy;
					(*part_ypos)+=part_moy/10;
					if(part_moy>=10)
						part_moy=part_moy%10;
					if(part_moy<=-10)
						part_moy=-((-part_moy)%10);

					part_mox+=mousedx;
					(*part_start)+=(part_mox/4)*160*16;
					if(part_mox>=4)
						part_mox=part_mox%4;
					if(part_mox<=-4)
						part_mox=-((-part_mox)%4);
				}
			}
			part_is=false;
			return true; // Still active
		}

		state.active=false;
		part_is=false;
		return false; // Not active
	};

	// public methods

	bool DoButton(int x, int y, char *idstr, char *helptext)
	{
		if(MouseInZone(x+gui_winx, y+gui_winy, 20, 10))
			strcpy(popup_text, helptext);
		return DoButton(x, y, false, idstr);
	};
	bool DoButtonRed(int x, int y, char *idstr, char *helptext)
	{
		if(MouseInZone(x+gui_winx, y+gui_winy, 20, 10))
			strcpy(popup_text, helptext);
		return DoButton(x, y, true, idstr);
	};
	bool DoButton(int x, int y, bool red, char *idstr)
	{
		strcpy(curname, idstr);
		SpaceName();
		idstr=curname;

		float dummy=0.0f;
		bool active=DoButtonLogic(x, y, 20, 10, dummy, idstr);
//		DrawButton(0, x, y, w, h, active, active&&lmbdown&&MouseInZone(x+gui_winx, y+gui_winy, w, h), 0.0f);
		if(red)
			glColor4f(1,0.7f,0.7f,1);
		else
			glColor4f(1,1,1,1);
		if(active && lmbdown && MouseInZone(x+gui_winx, y+gui_winy, 20, 10))
//			DrawBar(x, y, w, h, palette[0]);
			DrawSprite(buttondown, x, y);
		else
//			DrawBar(x, y, w, h, palette[1]);
			DrawSprite(buttonup, x, y);
		glColor4f(1,1,1,1);
		if(active)
			return state.clicked;
		return false;
	};
	bool DoKnob(int x, int y, float& value, int jax, char *idstr, char *helptext)
	{
		if(MouseInZone(x+gui_winx, y+gui_winy, 16, 16))
			strcpy(popup_text, helptext);
		return DoKnob(x, y, value, jax, idstr);
	};
	bool DoKnob(int x, int y, float& value, int jax, char *idstr)
	{
		strcpy(curname, idstr);
		SpaceName();
		idstr=curname;

		bool active=DoButtonLogic(x, y, 16, 16, value, idstr);
		if(active && rmbclick)
		{
			rcmenu=true;
			rcmenu_type=0;
			rcmenu_coffset=0;
			rcmenu_x=mousex-10;
			rcmenu_y=mousey-26;
			rmbclick=false; // to keep the rcmenu from going away immediately
			rcmenu_pval=&value;
		}

		if(jax>-1)
			value=joyaxis[jax];

//		if(calibration) // "fake" ui call to set up knobrec pointers
//			KnobRecCalibrate(idstr, &value);

		int rec=KnobRecState(&value);
		DrawKnob(x, y, value, rec);
		return active;
	};
	void DoPartIcon(int x, int y, int w, int& start, int& ypos, float zoom, Color& color, char *name, int mode, bool clone, char *idstr)
	{
		if(mode==0)
		{
			part_start=&start;
			part_ypos=&ypos;
			part_zoom=zoom;
	
			part_is=true;
			float dummy;
			bool active=DoButtonLogic(x, y, w, 10, dummy, idstr);
			DrawTexturedBar(x, y, w, 10, color, vgradient, &vglare); // title bar
			if(clone)
				DrawBar(x, y+5, w, 5, palette[14]); // clone body
			DrawBox(x, y, w, 10, palette[6]); // frame
		}
		else
			DrawBox(x, y, w, 10, palette[9]); // frame
		EnterWindow(current_window, x+1+gui_winx, y+gui_winy, 0, 0, w-2, 9);
		char pyttestr[3];
		pyttestr[0]=name[0];
		pyttestr[1]=name[1];
		pyttestr[2]='\0';
		DrawText(2, 0, palette[9], pyttestr); // instrument number
		DrawText(17, 0, palette[8], &name[2]); // name
		LeaveWindow();
	};
	void RequestWindowFocus(char *idstr)
	{
		strcpy(curname, idstr);
		SpaceName();
		idstr=curname;
		strcpy(active_window, idstr);
	};
	bool StartWindowSnapX(int& x, int& y, int& snapx, int w, int h, char *title, char *idstr)
	{
		int px=x;
		snapx=x;
		bool snapped=false;
		if(abs(snapx-(glkitGetWidth()-w))<16) // snap to right window edge
		{
			snapx=glkitGetWidth()-w;
			snapped=true;
		}
		if(abs(snapx)<16) // snap to left window edge
		{
			snapx=0;
			snapped=true;
		}
		x=snapx;
		bool active=StartWindow(x, y, snapped, w, h, false, w, h, h, title, idstr);
		x=px+(x-snapx);
		if(!snapped)
			snapx=x;
		if(snapped && lmbrelease)
			x=snapx;
		return active;
	};
	bool StartWindow(int& x, int& y, int w, int h, char *title, char *idstr)
	{
		return StartWindow(x, y, false, w, h, false, w, h, h, title, idstr);
	};
	bool StartWindowResize(int& x, int& y, int& w, int& h, int minw, int minh, int maxh, char *title, char *idstr)
	{
		return StartWindow(x, y, false, w, h, true, minw, minh, maxh, title, idstr);
	};
//	bool StartWindow(int& x, int& y, bool xsnap, int w, int h, char *title, char *idstr)
	bool StartWindow(int& x, int& y, bool xsnap, int& w, int& h, bool resize, int minw, int minh, int maxh, char *title, char *idstr)
	{
		strcpy(curname, idstr);
		SpaceName();
		idstr=curname;
		
		glLoadIdentity();
		glTranslatef(0.0f, 0.0f, 0.0f);
		
//		LogPrint("dui: starting window \"%s\"", idstr);

//		LoadMouse(1);

		LoadMouse(0);
		bool mactivate=(!dlmbclick && (lmbclick||rmbclick||mmbclick) && MouseInZone(x, y, w, 12));
		if(mactivate)
		{
//			LogPrint("dui: mactivate (titlebar) for window \"%s\" ------ %i%i%i", idstr, lmbclick, mmbclick, rmbclick);
			mf_settled=0;
			strcpy(mousewindow, idstr);
		}
		if(mf_settled>=1 && strcmp(mousewindow, idstr)==0) // this window has mouse focus
			LoadMouse(1);
		else
			ClearMouse();

		float dummy;
		bool activate=((lmbclick||rmbclick||mmbclick) && MouseInZone(x, y, w, h+12));
		bool moved=false;
//		if(DoButtonLogic(x-12, y, w, 12, dummy, idstr) && lmbdown) // dragging window
		if(DoButtonLogic(x, y, w, 12, dummy, idstr) && lmbdown) // dragging window
		{
			if(!xsnap)
				x+=mousedx;
			else
				moved=true;
			y+=mousedy;
			// window should already be active here, but... not always the case apparently, so fix it ;)
			strcpy(mousewindow, idstr);
			activate=true;
		}
		char id2[64];
		strcpy(id2, idstr);
		strcat(id2, "rsz");
		if(resize && DoButtonLogic(x+w-8, y+h+12-8, 8, 8, dummy, id2) && lmbdown) // resizing window
		{
			if(lmbclick) // clicked - anchor
			{
				anchorx=mousex;
				anchory=mousey;
				anchorix=w;
				anchoriy=h;
			}
//			w+=mousedx;
//			h+=mousedy;
			w=anchorix+(mousex-anchorx);
			h=anchoriy+(mousey-anchory);
			if(w<minw) w=minw;
			if(h<minh) h=minh;
			if(h>maxh) h=maxh;
			// window should already be active here, but... not always the case apparently, so fix it ;)
			strcpy(mousewindow, idstr);
			activate=true;
		}
		if(activate) // window was activated (by clicking inside it)
			strcpy(active_window, idstr);
		bool active=(strcmp(active_window, idstr)==0);
		DrawTexturedBar(x, y, w, 12, palette[9], wood_walnut, &vglare); // title bar
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
//		DrawSprite(iconify, x+w-13, y+2);
		DrawBox(x, y, w, 12, palette[6]); // title bar frame
		// title
		int colorout=8;
		int colorin=6;
		if(active)
		{
			colorout=7;
			colorin=3;
		}
		DrawText(x+2, y+1, palette[colorout], title);
		DrawText(x+4, y+1, palette[colorout], title);
		DrawText(x+3, y+0, palette[colorout], title);
		DrawText(x+3, y+2, palette[colorout], title);
		DrawText(x+3, y+1, palette[colorin], title);

//		EnterWindow(idstr, x, y+12, 0, 0, w, h);
		EnterWindow(idstr, x, y, 0, 0, w, h+12);

//		DrawBar(0, 0, w, h, color); // window body
		DrawBox(0, 12, w, h, palette[6]); // window frame

		if(moved)
			x+=mousedx;
		
		return active;
	};
//	void StartFlatWindow(int x, int y, int z, int w, int h, Color& color)
	void StartFlatWindow(int x, int y, int w, int h, char *idstr)
	{
		strcpy(curname, idstr);
		SpaceName();
		idstr=curname;

		glLoadIdentity();
		glTranslatef(0.0f, 0.0f, 0.0f);
		
		EnterWindow(current_window, x, y, 0, 0, w, h);
//		EnterWindow(idstr, x, y, 0, 0, w, h);

//		DrawBar(0, 0, w, h, color); // window body
//		DrawBox(0, -1, w, h+1, palette[6]); // window frame
	};
	void EndWindow()
	{
		LeaveWindow();
	};
	void StartScrollArea(int x, int y, int w, int h, float& scrollx, float& scrolly, int fw, int fh, Color& color, char *idstr)
	{
		strcpy(curname, idstr);
		SpaceName();
		idstr=curname;

		float dummy;
		char sid[32];
//		DrawBox(x, y, w, h, palette[6]); // area border
		bool hscroll=fw>w;
		bool vscroll=fh>h;
		if(hscroll && vscroll)
			DrawBar(x+w-8, y+h-8, 8, 8, palette[6]);
		if(hscroll)
		{
			int width=w;
			if(vscroll)
				width-=8;
			strcpy(sid, idstr);
			strcat(sid, "H");
			bool ascroll=false;
			ascroll=DoButtonLogic(x+(int)(scrollx*(width-16)), y+h-8, 16, 8, dummy, sid);
			if(ascroll && lmbclick) // clicked slider - anchor
			{
				anchorx=mousex;
				anchorfx=scrollx;
			}
			if(ascroll && lmbdown) // dragging slider
			{
				mouseregion=1;
//				scrollx+=(float)mousedx/(width-16);
				scrollx=anchorfx+(float)(mousex-anchorx)/(width-16);
			}
			else
			{
				ascroll=DoButtonLogic(x, y+h-8, width, 8, dummy, sid);
				if(ascroll && lmbdown) // setting slider
				{
					mouseregion=1;
					scrollx=(float)(mousex-(gui_winx+x+8))/(width-16);
				}
				if(ascroll && lmbclick) // clicked slider - anchor
				{
					anchorx=mousex;
					anchorfx=scrollx;
				}
			}
			if(scrollx<0.0f)
				scrollx=0.0f;
			if(scrollx>1.0f)
				scrollx=1.0f;
			glColor4f(1,1,1,1);
			DrawSprite(scrollbar_h1, x, y+h-8);
			DrawSprite(scrollbar_h2, x+width-8, y+h-8);
			DrawBar(x+8, y+h-8, width-16, 8, palette[6]);
			DrawBar(x+8, y+h-8+1, width-16, 6, palette[8]);
			DrawBar(x+8, y+h-8+3, width-16, 2, palette[7]);
			glColor4f(1,1,1,1);
			DrawSprite(scrollbar_hs, x+(int)((width-16)*scrollx), y+h-8);
			h-=8;
		}
		else
			scrollx=0.0f; // reset scroll to get predictable behavior for large areas
		if(vscroll)
		{
			int height=h;
//			if(hscroll)		// why is this commented out?
//				height-=8;
			strcpy(sid, idstr);
			strcat(sid, "V");
			bool ascroll=false;
			ascroll=DoButtonLogic(x+w-8, y+(int)(scrolly*(height-16)), 8, 16, dummy, sid);
			if(ascroll && lmbclick) // clicked slider - anchor
			{
				anchory=mousey;
				anchorfy=scrolly;
			}
			if(ascroll && lmbdown) // dragging slider
			{
				mouseregion=1;
//				scrolly+=(float)mousedy/(height-16);
				scrolly=anchorfy+(float)(mousey-anchory)/(height-16);
			}
			else
			{
				ascroll=DoButtonLogic(x+w-8, y, 8, height, dummy, sid);
				if(ascroll && lmbdown) // setting slider
				{
					mouseregion=1;
					scrolly=(float)(mousey-(gui_winy+y+8))/(height-16);
				}
				if(ascroll && lmbclick) // clicked slider - anchor
				{
					anchory=mousey;
					anchorfy=scrolly;
				}
			}
			if(scrolly<0.0f)
				scrolly=0.0f;
			if(scrolly>1.0f)
				scrolly=1.0f;
			glColor4f(1,1,1,1);
			DrawSprite(scrollbar_v1, x+w-8, y);
			DrawSprite(scrollbar_v2, x+w-8, y+height-8);
			DrawBar(x+w-8, y+8, 8, height-16, palette[6]);
			DrawBar(x+w-8+1, y+8, 6, height-16, palette[8]);
			DrawBar(x+w-8+3, y+8, 2, height-16, palette[7]);
			glColor4f(1,1,1,1);
			DrawSprite(scrollbar_vs, x+w-8, y+(int)((height-16)*scrolly));
			w-=8;
		}
		else
			scrolly=0.0f; // reset scroll to get predictable behavior for large areas
		if(hscroll && vscroll) // UGLYYY, this doesn't imply resize
			DrawSprite(icon_resize, x+w, y+h);
		DrawBar(x, y, w, h, color); // area body
		EnterWindow(current_window, gui_winx+x, gui_winy+y, (int)(scrollx*(fw-w)), (int)(scrolly*(fh-h)), w, h);
	};
	void EndScrollArea()
	{
		LeaveWindow();
	};

	// ---------------------- GFX -------------------------

	void DetectFontWidths(const char *filename)
	{
		unsigned char * data;
		FILE *file;
		unsigned char byte, id_length/*, chan[4]*/;
		int n, width, height, channels, x, y;
		file=fopen(filename, "rb");
		fread( &id_length, 1, 1, file );	// length of ID field
		fread( &byte, 1, 1, file );			// color map (1 or 0)
		fread( &byte, 1, 1, file );			// image type (should be 2(unmapped RGB))
		for(n=0;n<5;n++) fread( &byte, 1, 1, file );		// color map info
		fread( &n, 1, 2, file );		// image origin X
		fread( &n, 1, 2, file );		// image origin Y
		width=0;
		height=0;
		fread( &width, 1, 2, file );	// width
		fread( &height, 1, 2, file );	// height
		fread( &byte, 1, 1, file );		// bits
		channels=byte/8;
		fread( &byte, 1, 1, file );		// image descriptor byte (per-bit info)
		for(n=0;n<id_length;n++) fread( &byte, 1, 1, file );	// image description
		// color map data could be here
		data = (unsigned char*) malloc( width * height * channels );
		for(y=0;y<height;y++)
			for(x=0;x<width;x++)
			{
				n=(y*width+x)*channels;
				fread(&byte, 1, 1, file ); data[n+2]=byte;
				fread(&byte, 1, 1, file ); data[n+1]=byte;
				fread(&byte, 1, 1, file ); data[n+0]=byte;
			}
		// Detect character widths
		for(int i=0;i<110;i++)
		{
			font_offset[i]=-1;
			font_width[i]=-1;
			for(int x=0;x<8;x++)
			{
				int y;
				for(y=0;y<8;y++)
					if(data[(y*width+(i*8+x))*3+1]!=0)
						break;
				if(y!=8) // Non-transparent pixel found
				{
					if(font_offset[i]==-1)
						font_offset[i]=x;
					font_width[i]=x-font_offset[i]+1;
				}
			}
			if(font_offset[i]==-1)
				font_offset[i]=0;
			if(font_width[i]==-1)
				font_width[i]=2; // Space
		}
		// special case: '1'
		font_width[17]=font_width[16];
		font_offset[17]=font_offset[16];
		free(data);
	};
	
	void DrawChar(unsigned char ch, int x, int y, Color &color)
	{
		x+=gui_winx;
		y+=gui_winy;
		glBindTexture(GL_TEXTURE_2D, font.getHandle());
		glColor4f(color.r, color.g, color.b, 1.0f);
		ch-=' ';
		float u=(float)(ch*8+font_offset[ch])/1024;
		glBegin(GL_QUADS);
		glTexCoord2f(u, 1);
		glVertex3i(x, y, 0);
		glTexCoord2f(u, 0);
		glVertex3i(x, y+8,0);
		glTexCoord2f(u+(float)font_width[ch]/1024, 0);
		glVertex3i(x+font_width[ch], y+8, 0);
		glTexCoord2f(u+(float)font_width[ch]/1024, 1);
		glVertex3i(x+font_width[ch], y, 0);
		glEnd();
//		glLoadIdentity();
	};

	void DrawText(int sx, int sy, Color& color, char *string, ...)
	{
		glEnable(GL_TEXTURE_2D);
		char string2[256];
		va_list args;
	
		va_start(args, string);
		vsprintf(string2, string, args);
		va_end(args);
	
		int len=strlen(string2);
		int offset=0;
		for(int i=0;i<len;i++)
		{
			unsigned char ch=string2[i];
			int yo=0;
			if((char)ch=='Å') { ch='~'+2; yo=-1; }
			if((char)ch=='Ä') { ch='~'+3; yo=-1; }
			if((char)ch=='Ö') { ch='~'+4; yo=-1; }
			if((char)ch=='å') { ch='~'+5; yo=-1; }
			if((char)ch=='ä') { ch='~'+6; yo=-1; }
			if((char)ch=='ö') { ch='~'+7; yo=-1; }
			DrawChar(ch, sx+offset, sy, color);
			offset+=font_width[ch-' ']+1;
		}
		glDisable(GL_TEXTURE_2D);
	};

	int VStringLength(char *string, ...)
	{
		char string2[256];
		va_list args;
	
		va_start(args, string);
		vsprintf(string2, string, args);
		va_end(args);
	
		int len=strlen(string2);
		int offset=0;
		for(int i=0;i<len;i++)
		{
			unsigned char ch=string2[i];
			if(ch=='Å') ch='~'+2;
			if(ch=='Ä') ch='~'+3;
			if(ch=='Ö') ch='~'+4;
			if(ch=='å') ch='~'+5;
			if(ch=='ä') ch='~'+6;
			if(ch=='ö') ch='~'+7;
			offset+=font_width[ch-' ']+1;
		}
		return offset;
	};

	void DrawSprite(Texture& sprite, int x, int y)
	{
		x+=gui_winx;
		y+=gui_winy;
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, sprite.getHandle());
		glBegin(GL_QUADS);
		glTexCoord2f(0, 1);
		glVertex3i(x, y, 0);
		glTexCoord2f(1, 1);
		glVertex3i(x+sprite.width, y,0);
		glTexCoord2f(1, 0);
		glVertex3i(x+sprite.width, y+sprite.height, 0);
		glTexCoord2f(0, 0);
		glVertex3i(x, y+sprite.height, 0);
		glEnd();
		glDisable(GL_TEXTURE_2D);
	};
	
	void DrawBox(int x, int y, int w, int h, Color& color)
	{
		x+=gui_winx;
		y+=gui_winy;
		glColor4f(color.r, color.g, color.b, 1.0f);
		glBegin(GL_LINES);
		glVertex3i(x, y, 0);
		glVertex3i(x+w, y, 0);
		glVertex3i(x, y, 0);
		glVertex3i(x, y+h, 0);
		glVertex3i(x+w-1, y, 0);
		glVertex3i(x+w-1, y+h, 0);
		glVertex3i(x, y+h, 0);
		glVertex3i(x+w, y+h, 0);
		glEnd();
	};
	
	void DrawTexturedBar(int x, int y, int w, int h, Color& color, Texture& texture, Texture *effect)
	{
		x+=gui_winx;
		y+=gui_winy;
		glColor4f(color.r, color.g, color.b, 1.0f);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texture.getHandle());
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 1.0f);
		glVertex3i(x, y, 0);
		glTexCoord2f(0.0f, 1.0f-(float)h/texture.height);
		glVertex3i(x, y+h, 0);
		glTexCoord2f((float)w/512, 1.0f-(float)h/texture.height);
		glVertex3i(x+w, y+h, 0);
		glTexCoord2f((float)w/512, 1.0f);
		glVertex3i(x+w, y, 0);
		glEnd();
		glColor4f(1,1,1, 0.3f);
		if(effect!=NULL)
		{
			glBindTexture(GL_TEXTURE_2D, effect->getHandle());
			if(w>128) w=128;
			glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3i(x, y, 0);
			glTexCoord2f(0.0f, 1.0f-(float)h/effect->height);
			glVertex3i(x, y+h, 0);
			glTexCoord2f((float)w/128, 1.0f-(float)h/12);
			glVertex3i(x+w, y+h, 0);
			glTexCoord2f((float)w/128, 1.0f);
			glVertex3i(x+w, y, 0);
			glEnd();
		}
		glDisable(GL_TEXTURE_2D);
	};
	
	void DrawBar(int x, int y, int w, int h, Color& color)
	{
		x+=gui_winx;
		y+=gui_winy;
		glColor4f(color.r, color.g, color.b, 1.0f);
		glBegin(GL_QUADS);
		glVertex3i(x, y, 0);
		glVertex3i(x, y+h, 0);
		glVertex3i(x+w, y+h, 0);
		glVertex3i(x+w, y, 0);
		glEnd();
	};
	
	void DrawLine(int x1, int y1, int x2, int y2, int width, Color& color)
	{
		x1+=gui_winx;
		y1+=gui_winy;
		x2+=gui_winx;
		y2+=gui_winy;
		glColor4f(color.r, color.g, color.b, 1.0f);
		glLineWidth((float)width);
		glBegin(GL_LINES);
		glVertex3i(x1, y1, 0);
		glVertex3i(x2, y2, 0);
		glEnd();
		glLineWidth(1.0f);
	};
	
	void DrawBarAlpha(int x, int y, int w, int h, Color& color, float alpha)
	{
		x+=gui_winx;
		y+=gui_winy;
		glColor4f(color.r, color.g, color.b, alpha);
		glBegin(GL_QUADS);
		glVertex3i(x, y, 0);
		glVertex3i(x, y+h, 0);
		glVertex3i(x+w, y+h, 0);
		glVertex3i(x+w, y, 0);
		glEnd();
	};

	void DrawKnob(int x, int y, float value, int rec)
	{
		glColor4f(1.0f, 1.0f, 1.0f, 0.4f);
		DrawSprite(knobshadow, x, y+11);
		float angle=PI*1.25f-value*PI*1.5f;
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		int knobindex=(int)((angle/(2*PI)*16*2)+32)%4;
		DrawSprite(knobe[knobindex], x, y);
		switch(rec)
		{
		case 0:
			glColor4f(0.8f/1.6f, 0.9f/1.6f, 1.0f/1.6f, 1.0f);
			break;
		case 1:
			glColor4f(1.0f/1.6f, 0.7f/1.6f, 0.7f/1.6f, 1.0f);
			break;
		case 2:
			glColor4f(1.0f, 0.3f, 0.3f, 1.0f);
			break;
		}
		DrawSprite(knobtop, x, y);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glTranslatef((float)(x+gui_winx)+8.0f, (float)(y+gui_winy)+8.0f, 0.0f);
		glRotatef(-angle/(2*PI)*360, 0,0,1);
		glTranslatef(-(float)(x+gui_winx)-8.0f, -(float)(y+gui_winy)-8.0f, 0.0f);
		DrawSprite(knobdot, x, y);
		glLoadIdentity();
	};
	
	void DrawLED(int x, int y, int colid, bool lit)
	{
		Color color;
		color=palette[colid+11];
		if(!lit)
		{
			if(colid==2)
				glColor4f(color.r*1.0f, color.g*1.0f, color.b*1.0f, 1.0f);
			else if(colid==1)
				glColor4f(color.r*0.6f, color.g*0.6f, color.b*0.6f, 1.0f);
			else
				glColor4f(color.r*0.7f, color.g*0.7f, color.b*0.7f, 1.0f);
			DrawSprite(led[0], x-5, y-3);
		}
		else
		{
			glColor4f(color.r, color.g, color.b, 1.0f);
			DrawSprite(led[1], x-5, y-3);
			glBlendFunc(GL_ONE, GL_ONE);
			if(colid==2)
				glColor4f(color.r*1.0f, color.g*1.0f, color.b*1.0f, 1.0f);
			else
				glColor4f(color.r*0.5f, color.g*0.5f, color.b*0.5f, 1.0f);
			DrawSprite(led[2], x-5, y-3);
			if(colid==2)
				glColor4f(0.9f, 0.9f, 0.9f, 1.0f);
			else
				glColor4f(0.4f, 0.4f, 0.4f, 1.0f);
			DrawSprite(led[3], x-5, y-3);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
	};

	bool SelectFileSave(char *filename, int type)
	{
		glkitShowMouse(true);
		bool chosen=false;
		chosen=FileSelectorSave(glkitGetScreen(), filename, type);
		glkitShowMouse(false);
		return chosen;
	};

	bool SelectFileLoad(char *filename, int type)
	{
		glkitShowMouse(true);
		bool chosen=false;
		chosen=FileSelectorLoad(glkitGetScreen(), filename, type);
		glkitShowMouse(false);
		return chosen;
	};
};

#endif

