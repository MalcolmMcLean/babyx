#include "BabyX.h"

typedef struct
{
	BABYX *bbx;
	BBX_Panel *root;
	BBX_Label *message_lab;
	BBX_Button *ok_but;

} APP;

void createapp(void *obj, BABYX *bbx, BBX_Panel *root);
void killapp(void *obj);
void layoutapp(void *obj, int width, int height);
void ok_pressed(void *obj);

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstnace, LPSTR lpCommandLine, int nCmdShow)
{
	APP app;
	startbabyx(hInstance, "Hello Baby X", 320, 200, createapp, layoutapp, &app);

	return 0;
}


void createapp(void *obj, BABYX *bbx, BBX_Panel *root)
{
	APP *app = obj;
	app->bbx = bbx;
	app->root = root;
	app->message_lab = bbx_label(bbx, root, "Hello World");
	app->ok_but = bbx_button(bbx, root, "OK", ok_pressed, app);
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

	bbx_label_getpreferredsize(app->message_lab, &labwidth, &labheight);
	bbx_setpos(app->bbx, app->message_lab, width/2 - labwidth/2, 50, labwidth, labheight);
	bbx_setpos(app->bbx, app->ok_but, width / 2 - 25, height - 50, 50, 25);
}

void ok_pressed(void *obj)
{
	APP *app = obj;

	killapp(app);
	stopbabyx(app->bbx);
}