#include "BabyX.h"
#include <stdio.h>
#include <stdlib.h>
#include "animatedgif.h"
#include "bbx_GIFanimationwidget.h"

typedef struct
{
	BABYX *bbx;
	BBX_Panel *pan;
	BBX_Canvas *can;
	GIF *gif;
	void * ticker;
	long tick;
	long tock;
	unsigned char *buff;
	unsigned char pal[256 * 3];
	int transparent;
	BBX_RGBA background;
} GIFANIMATION;

static void gifanimation_layout(void *ptr, int width, int height);
static void gifanimation_redraw(GIFANIMATION *ga);
static void gifanimation_tick(void *ptr);

/*
   Create the animation widget
*/
BBX_Panel *gifanimationwidget(BABYX *bbx, BBX_Panel *parent)
{
	BBX_Panel *pan;
	GIFANIMATION *ga;


	ga = bbx_malloc(sizeof(GIFANIMATION));
	pan = bbx_panel(bbx, parent, "gifanimation", gifanimation_layout, ga);
	ga->bbx = bbx;
	ga->pan = pan;
	ga->can = 0;

	ga->gif = 0;
	ga->buff = 0;
	ga->tick = 0;
	ga->tock = 0;
	ga->ticker = bbx_addticker(bbx, 10, gifanimation_tick, ga);

	ga->background = bbx_rgba(255, 255, 255, 255);
	return pan;

}

/*
  Destructor
*/
void gifanimationwidget_kill(BBX_Panel *wgt)
{
	GIFANIMATION *ga;

	if (wgt)
	{
		ga = bbx_panel_getptr(wgt);
		bbx_removeticker(ga->bbx, ga->ticker);
		free(ga->buff);
		bbx_canvas_kill(ga->can);
		free(ga);
		bbx_panel_kill(wgt);
	}
}

/*
  set it to shaow a GIF. Note we don't manage the GIF, its' caller's
  responsibility to free it up.
*/
void gifanimation_setGIF(BBX_Panel *wgt, GIF *gif)
{
	GIFANIMATION *ga;
	int width, height;

	ga = bbx_panel_getptr(wgt);
	ga->gif = gif;
	free(ga->buff);
	ga->buff = gif_getfirstframe(gif, &width, &height, ga->pal, &ga->transparent);

	gifanimation_redraw(ga);

}

/*
  set background colour for animation
*/
void gifanimation_setbackground(BBX_Panel *wgt, BBX_RGBA col)
{
	GIFANIMATION *ga;

	ga = bbx_panel_getptr(wgt);
	ga->background = col;
	gifanimation_redraw(ga);
}

/*
  Our layout fucntion. Note we have to re-create the canvas
*/
static void gifanimation_layout(void *ptr, int width, int height)
{
	GIFANIMATION *ga = ptr;

	if (ga->can)
		bbx_canvas_kill(ga->can);
	ga->can = bbx_canvas(ga->bbx, ga->pan, width, height, bbx_rgba(255, 255, 255, 255));
	bbx_setpos(ga->bbx, ga->can, 0, 0, width, height);
	gifanimation_redraw(ga);
}


/*
  Drawing function
*/
static void gifanimation_redraw(GIFANIMATION *ga)
{
	int width, height;
	unsigned char *rgba;
	int x, y;
	int dx, dy;
	int ix, iy;
	int red, green, blue;
	int ci;

	if (!ga->can)
		return;

	rgba = bbx_canvas_rgba(ga->can, &width, &height);
	bbx_rectangle(rgba, width, height, 0, 0, width, height, ga->background);

	if (ga->gif)
	{
		dx = (width - ga->gif->width) / 2;
		dy = (height - ga->gif->height) / 2;
		for (y = 0; y < ga->gif->height; y++)
			for (x = 0; x < ga->gif->width; x++)
			{
			ci = ga->buff[y*ga->gif->width + x];
			if (ci == ga->gif->frames[ga->gif->current].transparent)
				continue;
			red = ga->pal[ci * 3];
			green = ga->pal[ci * 3 + 1];
			blue = ga->pal[ci * 3 + 2];
			iy = y + dy;
			ix = x + dx;

			if (ix >= 0 && ix < width && iy >= 0 && iy < height)
			{
				rgba[(iy*width + ix) * 4] = red;
				rgba[(iy*width + ix) * 4 + 1] = green;
				rgba[(iy*width + ix) * 4 + 2] = blue;
			}
			}
	}
	bbx_canvas_flush(ga->can);
}

/*
  Ticker function. We get one tick every 0.01 second. 
  That's the resolution of a gif time delay.
*/
static void gifanimation_tick(void *ptr)
{
	GIFANIMATION *ga = ptr;
	int width, height;
	int end;

	if (!ga->gif)
		return;
	if (ga->gif->Nframes == 1)
		return;
	ga->tock++;
	if (ga->tock - ga->tick > ga->gif->frames[ga->gif->current].delay)
	{
		ga->tock = ga->tick;
		end = gif_getnextframe(ga->gif, ga->buff);
		if (end == 0)
		{
			free(ga->buff);
			ga->buff = gif_getfirstframe(ga->gif, &width, &height, ga->pal, &ga->transparent);
		}
		gifanimation_redraw(ga);
	}
}
