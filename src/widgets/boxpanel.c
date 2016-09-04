#include <stdlib.h>
#include <string.h>
#include "BabyX.h"

typedef struct
{
	BABYX *bbx;
	BBX_Panel *pan;
	int alignment;
	char *text;
	int style;
	BBX_Label *lab;
	BBX_Canvas *topleft_can;
	BBX_Canvas *topright_can;
	BBX_Canvas *left_can;
	BBX_Canvas *right_can;
	BBX_Canvas *bottom_can;
	struct bitmap_font *font;
	void(*layout)(void *ptr, int width, int height);
	void *ptr;
} BOXPANEL;

static void boxpanel_layout(void *ptr, int width, int height);

BBX_Panel *bbx_boxpanel(BABYX *bbx, BBX_Panel *parent, char *text, void (layout)(void *ptr, int width, int height), void *ptr)
{
	BOXPANEL *box;
	
	box = bbx_malloc(sizeof(BOXPANEL));
	box->bbx = bbx;
	box->pan = bbx_panel(bbx, parent, "boxpanel", boxpanel_layout, box);
	box->layout = layout;
	box->ptr = ptr;
	if (text && strlen(text))
	{
		box->text = bbx_strdup(text);
		box->lab = bbx_label(bbx, box->pan, text);
	}
	else
	{
		box->text = 0;
		box->lab = 0;
	}
	box->topleft_can = 0;
	box->topright_can = 0;
	box->bottom_can = 0;
	box->left_can = 0;
	box->right_can = 0;

	box->font = 0;

	box->alignment = BBX_ALIGN_LEFT;

	return box->pan;
}

void bbx_boxpanel_kill(BBX_Panel *obj)
{
	BOXPANEL *box;

	if (obj)
	{
		box = bbx_panel_getptr(obj);
		bbx_label_kill(box->lab);
		bbx_canvas_kill(box->topleft_can);
		bbx_canvas_kill(box->topright_can);
		bbx_canvas_kill(box->bottom_can);
		bbx_canvas_kill(box->left_can);
		bbx_canvas_kill(box->right_can);
		free(box->text);
		bbx_panel_kill(box->pan);
		free(box);
	}
}

void bbx_boxpanel_setalignment(BBX_Panel *obj, int alignment)
{
	BOXPANEL *box;
	int width, height;

	box = bbx_panel_getptr(obj);
	box->alignment = alignment;
	bbx_getsize(box->bbx, obj, &width, &height);
	boxpanel_layout(box, width, height);
}

void bbx_boxpanel_setfont(BBX_Panel *obj, struct bitmap_font *font)
{
	BOXPANEL *box;
	int width, height;

	box = bbx_panel_getptr(obj);
	if (box->lab)
		bbx_label_setfont(box->lab, font);
	box->font = font;
	bbx_getsize(box->bbx, obj, &width, &height);
	boxpanel_layout(box, width, height);

}

void bbx_boxpanel_settext(BBX_Panel *obj, char *text)
{
	BOXPANEL *box;
	int width, height;

	box = bbx_panel_getptr(obj);
	free(box->text);
	if (text == 0 || strlen(text) == 0)
	{
		box->text = 0;
		bbx_label_kill(box->lab);
		box->lab = 0;
	}
	else
	{
		box->text = bbx_strdup(text);
		if (box->lab)
			bbx_label_settext(box->lab, text);
		else
		{
			box->lab = bbx_label(box->bbx, box->pan, text);
			if (box->font)
				bbx_label_setfont(box->lab, box->font);
		}
	}
	bbx_getsize(box->bbx, obj, &width, &height);
	boxpanel_layout(box, width, height);
}


static void boxpanel_layout(void *ptr, int width, int height)
{
	BOXPANEL *box = ptr;
	int labwidth, labheight;

	bbx_canvas_kill(box->topleft_can);
	bbx_canvas_kill(box->topright_can);
	bbx_canvas_kill(box->bottom_can);
	bbx_canvas_kill(box->left_can);
	bbx_canvas_kill(box->right_can);

	
	if (box->lab)
	{
		bbx_label_getpreferredsize(box->lab, &labwidth, &labheight);
		if (box->alignment == BBX_ALIGN_LEFT)
			bbx_setpos(box->bbx, box->lab, 5, 0, labwidth, labheight);
		else if (box->alignment == BBX_ALIGN_RIGHT)
			bbx_setpos(box->bbx, box->lab, width - labwidth - 5, 0, labwidth, labheight);
		else if (box->alignment == BBX_ALIGN_CENTER)
			bbx_setpos(box->bbx, box->lab, (width - labwidth)/2, 0, labwidth, labheight);
		if (width - labwidth - 5 > 2)
		{
			if (box->alignment == BBX_ALIGN_LEFT)
			{
				box->topright_can = bbx_canvas(box->bbx, box->pan, width - labwidth - 5, 2, bbx_color("light grey"));
				bbx_setpos(box->bbx, box->topright_can, 5 + labwidth, labheight / 2, width - labwidth - 5, 2);
				box->topleft_can = bbx_canvas(box->bbx, box->pan, 5, 2, bbx_color("light grey"));
				bbx_setpos(box->bbx, box->topleft_can, 0, labheight / 2, 5, 2);
			}
			else if (box->alignment == BBX_ALIGN_RIGHT)
			{
				box->topleft_can = bbx_canvas(box->bbx, box->pan, width - labwidth - 5, 2, bbx_color("light grey"));
				bbx_setpos(box->bbx, box->topleft_can, 0, labheight / 2, width - labwidth - 5, 2);
				box->topright_can = bbx_canvas(box->bbx, box->pan, 5, 2, bbx_color("light grey"));
				bbx_setpos(box->bbx, box->topright_can, width-5, labheight / 2, 5, 2);
			}
			else if (box->alignment == BBX_ALIGN_CENTER)
			{
				box->topleft_can = bbx_canvas(box->bbx, box->pan, (width - labwidth)/2, 2, bbx_color("light grey"));
				bbx_setpos(box->bbx, box->topleft_can, 0, labheight / 2, (width - labwidth)/2, 2);
				box->topright_can = bbx_canvas(box->bbx, box->pan, (width-labwidth)/2, 2, bbx_color("light grey"));
				bbx_setpos(box->bbx, box->topright_can, (width-labwidth)/2 + labwidth, labheight / 2, (width-labwidth)/2, 2);
			}
		}
		else
		{
			box->topleft_can = 0;
			box->topright_can = 0;
		}
		if (height - labheight/2 > 4)
		{
			box->left_can = bbx_canvas(box->bbx, box->pan, 2, height - labheight/2 - 4, bbx_color("light grey"));
			bbx_setpos(box->bbx, box->left_can, 0, labheight / 2 + 2, 2, height - labheight / 2 - 4);
			box->right_can = bbx_canvas(box->bbx, box->pan, 2, height - labheight / 2 - 4, bbx_color("light grey"));
			bbx_setpos(box->bbx, box->right_can, width-2, labheight / 2 + 2, 2, height - labheight / 2 - 4);
			box->bottom_can = bbx_canvas(box->bbx, box->pan, width, 2, bbx_color("light grey"));
			bbx_setpos(box->bbx, box->bottom_can, 0, height - 2, width, 2);
		}
		else
		{
			box->left_can = 0;
			box->right_can = 0;
			box->bottom_can = 0;
		}
	}
	else
	{
		box->left_can = bbx_canvas(box->bbx, box->pan, 2, height - 4, bbx_color("light grey"));
		box->right_can = bbx_canvas(box->bbx, box->pan, 2, height - 4, bbx_color("light grey"));
		box->bottom_can = bbx_canvas(box->bbx, box->pan, width, 2, bbx_color("light grey"));

		bbx_setpos(box->bbx, box->left_can, 0, 2, 2, height - 4);
		bbx_setpos(box->bbx, box->right_can, width - 2, 2, 2, height - 4);
		bbx_setpos(box->bbx, box->bottom_can, 0, height - 2, width, 2);
		box->topleft_can = bbx_canvas(box->bbx, box->pan, width, 2, bbx_color("light grey"));
		bbx_setpos(box->bbx, box->topleft_can, 0, 0, width, 2);
		box->topright_can = 0;
	}
	if (box->layout)
		(*box->layout)(box->ptr, width, height);
}
