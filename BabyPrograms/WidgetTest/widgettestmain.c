#include <stdio.h>
#include "animatedgif.h"
#include "BabyX.h"
#include "colorpicker.h"
#include "datepicker.h"
#include "bbx_console.h"
#include "bbx_GIFanimationwidget.h"



typedef struct
{
	BABYX *bbx;
	BBX_Panel *root;
	BBX_Menubar *menu;
	BBX_Label *message_lab;
	BBX_Panel *console_pan;
	BBX_Panel *animation_pan;
	BBX_Button *ok_but;
} APP;

void createapp(void *obj, BABYX *bbx, BBX_Panel *root);
void killapp(void *obj);
void layoutapp(void *obj, int width, int height);
void ok_pressed(void *obj);

static void menuchosen(void *obj, int id);

int main(void)
{
	APP app;
	startbabyx("Widget Test", 512, 512, createapp, layoutapp, &app);

	return 0;
}

void createapp(void *obj, BABYX *bbx, BBX_Panel *root)
{
	APP *app = obj;
	app->bbx = bbx;
	app->root = root;

	app->message_lab = bbx_label(bbx, root, "Hello Widgets");
	app->ok_but = bbx_button(bbx, root, "OK", ok_pressed, app);

	BBX_Popup *pop = bbx_popup(bbx);
	bbx_popup_append(pop, 1, "Open ...", "", 0);
	bbx_popup_append(pop, 2, "Animation", "", 0);
	bbx_popup_append(pop, 3, "Save ...", "", 0);
	bbx_popup_append(pop, 10, "Exit", "", 0);


	BBX_Popup *sub = bbx_popup(bbx);
	bbx_popup_append(sub, 4, "Red", "#FF0000", 0);
	bbx_popup_append(sub, 5, "Alice Blue", "#FFF8FF", 0);
	bbx_popup_append(sub, 6, "Green", "#00FF00", 0);

	BBX_Popup *pop2 = bbx_popup(bbx);
	bbx_popup_append(pop2, 7, "Choose", "me", 0);
	bbx_popup_append(pop2, 8, "Colours", "", sub);
	bbx_popup_append(pop2, 9, "Another Menu Item", "", 0);
	bbx_popup_append(pop2, 10, "Pick date ...", "", 0);

	app->menu = bbx_menubar(bbx, root, menuchosen, app);
	bbx_menubar_addmenu(app->menu, "File", pop);
	bbx_menubar_addmenu(app->menu, "Help", pop2);

	app->console_pan = bbx_console(bbx, app->root);
	app->animation_pan = gifanimationwidget(bbx, app->root);
}

void killapp(void *obj)
{
	APP *app = obj;

	bbx_label_kill(app->message_lab);
	bbx_button_kill(app->ok_but);
}


void layoutapp(void *obj, int width, int height)
{
	APP *app = obj;
	int labwidth, labheight;

	bbx_setpos(app->bbx, app->menu, 0, 0, width, 20);
	bbx_label_getpreferredsize(app->message_lab, &labwidth, &labheight);
	bbx_setpos(app->bbx, app->message_lab, width / 2 - labwidth / 2, 50, labwidth, labheight);
	bbx_setpos(app->bbx, app->ok_but, width / 2 - 25, height - 50, 50, 25);

	bbx_setpos(app->bbx, app->console_pan, 100, 100, 300, 200);
	bbx_setpos(app->bbx, app->animation_pan, 10, 20, 64, 64);

	bbx_console_printf(app->console_pan, "BBX console says hello\n");
}

void ok_pressed(void *obj)
{
	APP *app = obj;

	killapp(app);
	stopbabyx(app->bbx);
}

static void menuchosen(void *obj, int id)
{
	APP *app = obj;
	BABYX *bbx = app->bbx;
	char *fname;
	char *text;

	switch (id)
	{
	case 2:
		fname = bbx_getopenfile(app-> bbx, "*.gif");
		if (fname)
		{
			GIF *gif = loadanimatedgif(fname);
			if (gif)
			{
				gifanimation_setGIF(app->animation_pan, gif);
			}
		}
		break;
	case 10:
		pickcolor(app->bbx, bbx_color("green"));
		//fname = bbx_getopenfile(bbx, "*");
		break;
	case 1:
		pickdate(app->bbx, 2000, 1, 1, 0, 0, 0);
		break;
	}
}
