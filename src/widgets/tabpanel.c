#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "BabyX.h"

#include "tabpanel.h"

#define PI 3.1415926535897932384626433832795

typedef struct
{
	BABYX *bbx;
	BBX_Panel *pan;
	BBX_Canvas *tabpane_can;
	BBX_Canvas *left_can;
	BBX_Canvas *bottom_can;
	BBX_Canvas *right_can;
	int Ntabs;
	int current;
	char **tabnames;
	void(**layout)(void *ptr, int width, int height, int miny);
	void **ptr;
	void **objects;
	int Nchildren;
	int width;
	int height;
} BBX_TAB;

static void bbx_tab_layout(void *ptr, int width, int height);
static void bbx_tab_drawtabpane();
static void bbx_tab_mouse(void *ptr, int action, int x, int y, int buttons);
static void drawarc(unsigned char *rgba, int width, int height, int x0, int y0, int radius, double from, double to, BBX_RGBA col);

BBX_Panel *bbx_tab(BABYX *bbx, BBX_Panel *parent)
{
	BBX_Panel *pan;
	BBX_TAB *tab;


	tab = bbx_malloc(sizeof(BBX_TAB));
	tab->bbx = bbx;
	pan = bbx_panel(bbx, parent, "tab", bbx_tab_layout, tab);
	tab->pan = pan;
	tab->tabpane_can = 0;
	tab->left_can = 0;
	tab->right_can = 0;
	tab->bottom_can = 0;
	tab->current = -1;
	tab->Ntabs = 0;
	tab->tabnames = 0;
	tab->layout = 0;
	tab->ptr = 0;
	tab->objects = 0;
	tab->Nchildren = 0;
	tab->width = -1;
	tab->height = -1;
	return pan;
}

void bbx_tab_kill(BBX_Panel *obj)
{
	BBX_TAB *tab;
	int i;

	if (obj)
	{
		tab = bbx_panel_getptr(obj);
		bbx_canvas_kill(tab->tabpane_can);
		bbx_canvas_kill(tab->left_can);
		bbx_canvas_kill(tab->right_can);
		bbx_canvas_kill(tab->bottom_can);
		bbx_panel_kill(tab->pan);
		for (i = 0; i < tab->Ntabs; i++)
			free(tab->tabnames[i]);
		free(tab->tabnames);
		free(tab->ptr);
		free(tab->layout);
		free(tab);
	}
}

int bbx_tab_addtab(BBX_Panel *pan, char *name, void(*layout)(void *ptr, int width, int height, int miny), void *ptr)
{
	BBX_TAB *tab = bbx_panel_getptr(pan);

	tab->tabnames = bbx_realloc(tab->tabnames, (tab->Ntabs + 1) * sizeof(char *));
	tab->tabnames[tab->Ntabs] = bbx_strdup(name);

	tab->layout = bbx_realloc(tab->layout, (tab->Ntabs + 1) * sizeof(void(*)(void *, int, int, int)));
	tab->layout[tab->Ntabs] = layout;

	tab->ptr = bbx_realloc(tab->ptr, (tab->Ntabs + 1) * sizeof(void *));
	tab->ptr[tab->Ntabs] = ptr;
	tab->Ntabs++;
	tab->current = 0;

}

int bbx_tab_showtab(BBX_Panel *pan, char *name)
{
	BBX_TAB *tab = bbx_panel_getptr(pan);
	int i;
	int cwidth, cheight;

	for (i = 0; i < tab->Ntabs; i++)
		if (!strcmp(tab->tabnames[i], name))
			tab->current = i;
	if (!tab->tabpane_can)
		return;

	bbx_tab_drawtabpane(tab);
	for (i = 0; i < tab->Nchildren; i++)
	{
		bbx_getsize(tab->bbx, tab->objects[i], &cwidth, &cheight);
		bbx_setpos(tab->bbx, tab->objects[i], tab->width + 1, 0, cwidth, cheight);
	}
	if (tab->layout[tab->current])
		(*tab->layout[tab->current])(tab->ptr[tab->current], tab->width, tab->height, 25);
}

void bbx_tab_register(BBX_Panel *pan, void *obj)
{
	BBX_TAB *tab = bbx_panel_getptr(pan);

	tab->objects = bbx_realloc(tab->objects, (tab->Nchildren + 1) * sizeof(void *));
	tab->objects[tab->Nchildren] = obj;
	tab->Nchildren++;
}

void bbx_tab_deregister(BBX_Panel *pan, void *obj)
{
	BBX_TAB *tab = bbx_panel_getptr(pan);
	int i;

	for (i = 0; i < tab->Nchildren; i++)
		if (tab->objects[i] == obj)
			break;
	if (i < tab->Nchildren)
	{
		memmove(&tab->objects[i], &tab->objects[i + 1], (tab->Nchildren - i - 1) * sizeof(void *));
		tab->Nchildren--;
	}

}

static void bbx_tab_layout(void *ptr, int width, int height)
{
	BBX_TAB *tab = ptr;
	int cwidth, cheight;
	int i;

	bbx_canvas_kill(tab->tabpane_can);
	bbx_canvas_kill(tab->left_can);
	bbx_canvas_kill(tab->right_can);
	bbx_canvas_kill(tab->bottom_can);

	tab->width = width;
	tab->height = height;
	tab->tabpane_can = bbx_canvas(tab->bbx, tab->pan, width, 25, bbx_color("grey"));
	bbx_canvas_setmousefunc(tab->tabpane_can, bbx_tab_mouse, tab);
	bbx_setpos(tab->bbx, tab->tabpane_can, 0, 0, width, 25);
	bbx_tab_drawtabpane(tab);

	tab->left_can = bbx_canvas(tab->bbx, tab->pan, 2, height - 25-2, bbx_color("light grey"));
	tab->right_can = bbx_canvas(tab->bbx, tab->pan, 2, height - 25-2, bbx_color("dark grey"));
	tab->bottom_can = bbx_canvas(tab->bbx, tab->pan, width, 2, bbx_color("dark grey"));

	bbx_setpos(tab->bbx, tab->left_can, 0, 25, 2, height - 25 - 2);
	bbx_setpos(tab->bbx, tab->right_can, width - 2, 25, 2, height - 25 - 2);
	bbx_setpos(tab->bbx, tab->bottom_can, 0, height - 2, width, 2);

	for (i = 0; i < tab->Nchildren; i++)
	{
		bbx_getsize(tab->bbx, tab->objects[i], &cwidth, &cheight);
		bbx_setpos(tab->bbx, tab->objects[i], width + 1, 0, cwidth, cheight);
	}

	if (tab->current >= 0 && tab->current < tab->Ntabs)
	{
		if (tab->layout[tab->current])
			(*tab->layout[tab->current])(tab->ptr[tab->current], width, height, 25);
	}
}

static void bbx_tab_drawtabpane(BBX_TAB *tab)
{
	unsigned char *rgba;
	int width, height;
	int i;
	int x;
	int lenx;

	if (!tab->tabpane_can)
		return;

	rgba = bbx_canvas_rgba(tab->tabpane_can, &width, &height);
	bbx_rectangle(rgba, width, height, 0, 0, width, height, bbx_color("grey"));
	if (tab->current == 0)
	{
		bbx_line(rgba, width, height, 0, 0, 0, height - 1, bbx_color("light grey"));
		bbx_line(rgba, width, height, 1, 0, 1, height - 1, bbx_color("light grey"));
	}
	else
	{
		bbx_line(rgba, width, height, 0, 2, 0, height - 1, bbx_color("light grey"));
		bbx_line(rgba, width, height, 1, 2, 1, height - 1, bbx_color("light grey"));
	}
	x = 5;
	for (i = 0; i < tab->Ntabs; i++)
	{
		bbx_drawutf8(rgba, width, height, x, 18, tab->tabnames[i], bbx_utf8_Nchars(tab->tabnames[i]), tab->bbx->gui_font,
			bbx_color("black"));
		lenx = bbx_utf8width(tab->bbx->gui_font, tab->tabnames[i], bbx_utf8_Nchars(tab->tabnames[i]));
		
		if (tab->current != i)
		{
			bbx_line(rgba, width, height, x + lenx + 5, 7, x + lenx + 5, height - 1, bbx_color("light grey"));
			bbx_line(rgba, width, height, x + lenx + 5 + 1, 7, x + lenx + 5 + 1, height - 1, bbx_color("dark grey"));
			bbx_line(rgba, width, height, x-5, 2, x + lenx, 2, bbx_color("light grey"));
			bbx_line(rgba, width, height, x-5, 3, x + lenx, 3, bbx_color("light grey"));
			drawarc(rgba, width, height, x + lenx, 7, 5, -PI / 2, 0, bbx_color("light grey"));
			bbx_line(rgba, width, height, x-5, height - 2, x + lenx, height - 2, bbx_color("light grey"));
			bbx_line(rgba, width, height, x-5, height - 1, x + lenx, height - 1, bbx_color("light grey"));
		}
		else
		{
			bbx_line(rgba, width, height, x + lenx + 5, 5, x + lenx +5, height - 1, bbx_color("dark grey"));
			bbx_line(rgba, width, height, x + lenx + 5 + 1, 5, x + lenx + 5 + 1, height - 1, bbx_color("dark grey"));
			drawarc(rgba, width, height, x + lenx, 5, 5, -PI / 2, 0, bbx_color("light grey"));
			bbx_line(rgba, width, height, x-5, 0, x + lenx, 0, bbx_color("light grey"));
			bbx_line(rgba, width, height, x-5, 1, x + lenx, 1, bbx_color("light grey"));
		}
		x += lenx;
		x += 10;


	}

	bbx_line(rgba, width, height, x-10, height - 2, width, height - 2, bbx_color("light grey"));
	bbx_line(rgba, width, height, x-10, height - 1, width, height - 1, bbx_color("light grey"));
	bbx_canvas_flush(tab->tabpane_can);
}

static void bbx_tab_mouse(void *ptr, int action, int x, int y, int buttons)
{
	BBX_TAB *tab = ptr;
	int i;
	int sx, ex;

	if (action == BBX_MOUSE_CLICK && (buttons & BBX_MOUSE_BUTTON1))
	{
		sx = 5;
		for (i = 0; i < tab->Ntabs; i++)
		{
			ex = sx + bbx_utf8width(tab->bbx->gui_font, tab->tabnames[i], bbx_utf8_Nchars(tab->tabnames[i]));
			if (x >= sx && x < ex)
			{
				bbx_tab_showtab(tab->pan, tab->tabnames[i]);
				break;
			}
			sx = ex + 10;
		}
	}
}

static double clamprange(double theta);
static int between(double thetaa, double thetab, double theta);

static void drawarc(unsigned char *rgba, int width, int height,  int x0, int y0, int radius, double from, double to, BBX_RGBA col)
{
	int x = 0;
	int y = radius;
	int delta = 2 - 2 * radius;
	int error = 0;
	double thetaa, thetab;
	double theta;
	unsigned char red, green, blue;

	red = bbx_red(col);
	green = bbx_green(col);
	blue = bbx_blue(col);

	thetaa = clamprange(from);
	thetab = clamprange(to);
	while (y >= 0) {
		//SetPixel(hdc,x0 + x, y0 + y,pencol);

		theta = atan2(-y, x);
		if (between(thetaa, thetab, theta))
		{
			if (y0 - y >= 0 && y0 - y < height && x0 + x >= 0 && x0 + x < width)
			{
				rgba[((y0 - y)*width + x0 + x) * 4] = red;
				rgba[((y0 - y)*width + x0 + x) * 4 + 1] = green;
				rgba[((y0 - y)*width + x0 + x) * 4 + 2] = blue;
			}
		}
		theta = atan2(-y, -x);
		if (between(thetaa, thetab, theta))
		{
			if (y0 - y >= 0 && y0 - y < height && x0 - x >= 0 && x0 - x < width)
			{
				rgba[((y0 - y)*width + x0 - x) * 4] = red;
				rgba[((y0 - y)*width + x0 - x) * 4 + 1] = green;
				rgba[((y0 - y)*width + x0 - x) * 4 + 2] = blue;
			}
		}
		theta = atan2(y, x);
		if (between(thetaa, thetab, theta))
		{
			if (y0 + y >= 0 && y0 + y < height && x0 + x >= 0 && x0 + x < width)
			{
				rgba[((y0 + y)*width + x0 + x) * 4] = red;
				rgba[((y0 + y)*width + x0 + x) * 4 + 1] = green;
				rgba[((y0 + y)*width + x0 + x) * 4 + 2] = blue;
			}
		}
		theta = atan2(y, -x);
		if (between(thetaa, thetab, theta))
		{
			if (y0 + y >= 0 && y0 + y < height && x0 - x >= 0 && x0 - x < width)
			{
				rgba[((y0 + y)*width + x0 - x) * 4] = red;
				rgba[((y0 + y)*width + x0 - x) * 4 + 1] = green;
				rgba[((y0 + y)*width + x0 - x) * 4 + 2] = blue;
			}
		}
		error = 2 * (delta + y) - 1;
		if (delta < 0 && error <= 0) {
			++x;
			delta += 2 * x + 1;
			continue;
		}
		error = 2 * (delta - x) - 1;
		if (delta > 0 && error > 0) {
			--y;
			delta += 1 - 2 * y;
			continue;
		}
		++x;
		delta += 2 * (x - y);
		--y;
	}
}

static double clamprange(double theta)
{
	while (theta > PI)
		theta -= 2 * PI;
	while (theta < -PI)
		theta += 2 * PI;
	return theta;
}

static int between(double thetaa, double thetab, double theta)
{
	if (thetaa < thetab && theta > thetaa && theta < thetab)
		return 1;
	if (thetaa > thetab && (theta > thetaa || theta < thetab) )
		return 1;
	return 0;
}
