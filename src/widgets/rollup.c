#include "BabyX.h"

typedef struct
{
	struct rollup *rup;
	int index;
} RPTAG;

typedef struct
{
	BBX_Button *but;
	BBX_Panel *pan;
	RPTAG *tag;
	int height;
	char *text;
	int enabled;
	int state;
	void(*layout)(void *ptr, int width, int height);
	void *ptr;

} ROLLUPPANEL;

typedef struct rollup
{
	BABYX *bbx;
	BBX_Panel *pan;
	BBX_Panel *main_pan;
	BBX_Scrollbar *sb;
	ROLLUPPANEL *rollups;
	int Nrollups;
	int ypos;
	BBX_Canvas *left_can;
	BBX_Canvas *right_can;
	BBX_Canvas *bottom_can;
	BBX_Canvas *top_can;
	int range;
	int width;
	int height;
} ROLLUP;

static void layout(void *ptr, int width, int height);
static void layoutmain(void *ptr, int width, int height);
static void pressbutton(void *ptr);
static void scrollme(void *ptr, int pos);
static int rollup_gettotheight(ROLLUP *rup);

BBX_Panel *bbx_rollup(BABYX *bbx, BBX_Panel *parent, int scrollbar)
{
	ROLLUP *rup;

	rup = bbx_malloc(sizeof(ROLLUP));
	rup->bbx = bbx;
	rup->pan = bbx_panel(bbx, parent, "rollup", layout, rup);
	if (scrollbar)
	{
		rup->sb = bbx_scrollbar(bbx, rup->pan, BBX_SCROLLBAR_VERTICAL, scrollme, rup);
	}
	else
	{
		rup->sb = 0;
	}
	rup->main_pan = bbx_panel(bbx, rup->pan, "rollupmainpanel", layoutmain, rup);

	rup->ypos = 0;
	rup->rollups = 0;
	rup->Nrollups = 0;

	rup->width = 0;
	rup->height = 0;
	rup->left_can = 0;
	rup->right_can = 0;
	rup->top_can = 0;
	rup->bottom_can = 0;

	return rup->pan;
}

void bbx_rollup_kill(BBX_Panel *obj)
{
	ROLLUP *rup;
	int i;

	if (obj)
	{
		rup = bbx_panel_getptr(obj);
		bbx_canvas_kill(rup->top_can);
		bbx_canvas_kill(rup->bottom_can);
		bbx_canvas_kill(rup->left_can);
		bbx_canvas_kill(rup->right_can);
		bbx_scrollbar_kill(rup->sb);
		bbx_panel_kill(rup->main_pan);
		bbx_panel_kill(rup->pan);
		for (i = 0; i < rup->Nrollups; i++)
			free(rup->rollups[i].tag);
		free(rup->rollups);
		free(rup);
	}
}


BBX_Panel *bbx_rollup_addrollup(BBX_Panel *obj, char *text, int height, void (*layout)(void *ptr, int width, int height), void *ptr)
{
	ROLLUP *rup;
	ROLLUPPANEL *rp;
	rup = bbx_panel_getptr(obj);

	rup->rollups = bbx_realloc(rup->rollups, (rup->Nrollups + 1)*sizeof(ROLLUPPANEL));
	rp = &rup->rollups[rup->Nrollups];

	rp->tag = bbx_malloc(sizeof(RPTAG));
	rp->tag->rup = rup;
	rp->tag->index = rup->Nrollups;
	rp->text = bbx_strdup(text);
	rp->but = bbx_button(rup->bbx, rup->main_pan, text, pressbutton, rp->tag);
	rp->state = 0;
	rp->height = height;
	rp->pan = bbx_panel(rup->bbx, rup->main_pan, "rolluppanel", layout, ptr);
	rp->ptr = ptr;
	rp->enabled = 0;

	rup->Nrollups++;
	rup->range = rollup_gettotheight(rup);

	if (rup->height != 0)
		layout(rup, rup->width, rup->height);

	return rp->pan;

}

BBX_Panel *bbx_rollup_getpanel(BBX_Panel *obj, char *name)
{
	ROLLUP *rup;
	int i;

	rup = bbx_panel_getptr(obj);
	for (i = 0; i < rup->Nrollups; i++)
		if (!strcmp(name, rup->rollups[i].text))
			return rup->rollups[i].pan;
	return 0;
}

void bbx_rollup_getstate(BBX_Panel *obj, char *name)
{
	ROLLUP *rup;
	int i;

	rup = bbx_panel_getptr(obj);
	for (i = 0; i < rup->Nrollups; i++)
		if (!strcmp(name, rup->rollups[i].text))
			return rup->rollups[i].state;

	return -1;
}

void bbx_rollup_closepanel(BBX_Panel *obj, char *name)
{
	ROLLUP *rup;
	int i;

	rup = bbx_panel_getptr(obj);
	for (i = 0; i < rup->Nrollups; i++)
		if (!strcmp(name, rup->rollups[i].text))
		{
			rup->rollups[i].state = 1;
			pressbutton(rup->rollups[i].tag);
		}
}

void bbx_rollup_openpanel(BBX_Panel *obj, char *name)
{
	ROLLUP *rup;
	int i;

	rup = bbx_panel_getptr(obj);
	for (i = 0; i < rup->Nrollups; i++)
		if (!strcmp(name, rup->rollups[i].text))
		{
			rup->rollups[i].state = 0;
			pressbutton(rup->rollups[i].tag);
		}
}

static void layout(void *ptr, int width, int height)
{
	ROLLUP *rup = ptr;
	int panwidth;

	panwidth = rup->sb ? width - 15 -2 : width -4;
	bbx_canvas_kill(rup->left_can);
	bbx_canvas_kill(rup->right_can);
	bbx_canvas_kill(rup->top_can);
	bbx_canvas_kill(rup->bottom_can);

	rup->top_can = bbx_canvas(rup->bbx, rup->pan, panwidth, 2, bbx_color("dark grey"));
	rup->bottom_can = bbx_canvas(rup->bbx, rup->pan, panwidth, 2, bbx_color("light grey"));
	rup->left_can = bbx_canvas(rup->bbx, rup->pan, 2, height - 4, bbx_color("dark grey"));
	bbx_setpos(rup->bbx, rup->top_can, 0, 0, panwidth, 2);
	bbx_setpos(rup->bbx, rup->left_can, 0, 2, 2, height - 4);
	bbx_setpos(rup->bbx, rup->bottom_can, 0, height - 2, panwidth, 2);
	if (rup->sb)
	{
		rup->right_can = bbx_canvas(rup->bbx, rup->pan, 2, height - 4, bbx_color("light grey"));
		bbx_setpos(rup->bbx, rup->right_can, width - 2, 2, 2, height - 4);
	}
	else
		rup->right_can = 0;

	if (rup->sb && rup->height != height)
	{
		bbx_setpos(rup->bbx, rup->sb, width - 15, 0, 15, height);
	}
	bbx_setpos(rup->bbx, rup->main_pan, 2, 2, panwidth, height - 4);
	
	if (rup->range != rollup_gettotheight(rup) || height != rup->height)
	{
		rup->range = rollup_gettotheight(rup);
		if (rup->ypos > rup->range - (height - 4))
			rup->ypos = rup->range - (height - 4);
		if (rup->ypos < 0)
			rup->ypos = 0;
		bbx_scrollbar_set(rup->sb, rup->range, height - 4, 0);
	}

	rup->width = width;
	rup->height = height;
}

static void layoutmain(void *ptr, int width, int height)
{
	ROLLUP *rup = ptr;
	int i;
	int y = -rup->ypos;

	for (i = 0; i < rup->Nrollups; i++)
	{
		bbx_setpos(rup->bbx, rup->rollups[i].but, width, 0, width, 18);
		bbx_setpos(rup->bbx, rup->rollups[i].pan, width, 0, width, rup->rollups[i].height);
	}

	for (i = 0; i < rup->Nrollups; i++)
	{
		bbx_setpos(rup->bbx, rup->rollups[i].but, 0, y, width, 18);
		y += 20;
		if (rup->rollups[i].state == 1)
		{
			bbx_setpos(rup->bbx, rup->rollups[i].pan, 0, y, width, rup->rollups[i].height);
			y += rup->rollups[i].height;
		}
		else
		{
			bbx_setpos(rup->bbx, rup->rollups[i].pan, width, 0, width, rup->rollups[i].height);
		}
	}
}

static void pressbutton(void *ptr)
{
	RPTAG *tag = ptr;
	ROLLUP *rup = tag->rup;
	int i = tag->index;
	int width, height;

	if (rup->rollups[i].state == 1)
		rup->rollups[i].state = 0;
	else
		rup->rollups[i].state = 1;
	rup->range = rollup_gettotheight(rup);
	if (rup->ypos > rup->range - (rup->height - 4))
		rup->ypos = rup->range - (rup->height - 4);
	if (rup->ypos < 0)
		rup->ypos = 0;
	bbx_scrollbar_set(rup->sb, rup->range, rup->height -4, rup->ypos);
	bbx_getsize(rup->bbx, rup->main_pan, &width, &height);
	layoutmain(rup, width, height);
}

static void scrollme(void *ptr, int pos)
{
	ROLLUP *rup = ptr;
	int width, height;

	rup->ypos = pos;
	bbx_getsize(rup->bbx, rup->main_pan, &width, &height);
	layoutmain(rup, width, height);
}

static int rollup_gettotheight(ROLLUP *rup)
{
	int i;
	int answer = 0;

	for (i = 0; i < rup->Nrollups; i++)
	{
		answer += 20;
		if (rup->rollups[i].state == 1)
			answer += rup->rollups[i].height;
	}

	return answer;
}