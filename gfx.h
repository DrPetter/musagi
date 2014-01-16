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
}

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
}

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
	glTexCoord2f(0.0f, 1.0f-(float)h/512);
	glVertex3i(x, y+h, 0);
	glTexCoord2f((float)w/512, 1.0f-(float)h/512);
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
		glTexCoord2f(0.0f, 1.0f-(float)h/128);
		glVertex3i(x, y+h, 0);
		glTexCoord2f((float)w/128, 1.0f-(float)h/12);
		glVertex3i(x+w, y+h, 0);
		glTexCoord2f((float)w/128, 1.0f);
		glVertex3i(x+w, y, 0);
		glEnd();
	/*	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 1.0f);
		glVertex3i(x, y, 0);
		glTexCoord2f(0.0f, 0.0f);
		glVertex3i(x, y+h, 0);
		glTexCoord2f(1.0f, 0.0f);
		glVertex3i(x+w, y+h, 0);
		glTexCoord2f(1.0f, 1.0f);
		glVertex3i(x+w, y, 0);
		glEnd();*/
	}
	glDisable(GL_TEXTURE_2D);
}

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
}

void DrawKnob(int x, int y, float value)
{
	glColor4f(1.0f, 1.0f, 1.0f, 0.4f);
	DrawSprite(knobshadow, x, y+11);
	float angle=PI*1.25f-value*PI*1.5f;
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	int knobindex=(int)((angle/(2*PI)*16*2)+32)%4;
	DrawSprite(knobe[knobindex], x, y);
	glColor4f(0.8f, 0.9f, 1.0f, 1.0f);
	DrawSprite(knobtop, x, y);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glTranslatef((float)(x+gui_winx)+8.0f, (float)(y+gui_winy)+8.0f, 0.0f);
	glRotatef(-angle/(2*PI)*360, 0,0,1);
	glTranslatef(-(float)(x+gui_winx)-8.0f, -(float)(y+gui_winy)-8.0f, 0.0f);
	DrawSprite(knobdot, x, y);
	glLoadIdentity();
}

void DrawLED(int x, int y, int colid, bool lit)
{
	Color color;
	color=palette[colid+11];
	if(!lit)
	{
		if(colid==2)
			glColor4f(color.r*1.0f, color.g*1.0f, color.b*1.0f, 1.0f);
		else
			glColor4f(color.r*0.8f, color.g*0.8f, color.b*0.8f, 1.0f);
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
}

