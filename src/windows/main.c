#include "BabyX.h"

void create(void *obj, BABYX *bbx, BBX_Panel *root);
void layout(void *obj, int width, int height);
void domenu(void *ptr, int d);

typedef struct
{
	BABYX *bbx;
	BBX_Panel *root;
	BBX_Menubar *menu;
	BBX_Label *lab;
	BBX_Canvas *can;
	BBX_Button *but;
	BBX_Scrollbar *sb;
	BBX_EditBox *box;
} APP;

int APIENTRY WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPTSTR    lpCmdLine,
	int       nCmdShow)
{
	APP app;

	startbabyx(hInstance, "Fred", 500, 200, create, layout, &app);
}

void create(void *obj, BABYX *bbx, BBX_Panel *root)
{
	APP *app = (APP *)obj;
	BBX_Popup *hello_mnu = bbx_popup(bbx);
	BBX_Popup *goodbye_mnu = bbx_popup(bbx);
	BBX_Popup *help_mnu = bbx_popup(bbx);
	BBX_Popup *sub_mnu = bbx_popup(bbx);

	bbx_popup_append(sub_mnu, 300, "sub1", "", 0);
	bbx_popup_append(sub_mnu, 301, "sub2", "", 0);

	bbx_popup_append(hello_mnu, 1, "Hello", "", 0);
	bbx_popup_append(hello_mnu, 2, "Goodbye", "", 0);
	bbx_popup_append(hello_mnu, 3, "one", "", 0);
	bbx_popup_append(hello_mnu, 4, "two", "", sub_mnu);
	bbx_popup_append(hello_mnu, 5, "three", "", 0);
	bbx_popup_append(hello_mnu, 6, "four", "", 0);
	bbx_popup_append(hello_mnu, 7, "five", "", 0);
	bbx_popup_append(hello_mnu, 8, "six", "", 0);
	bbx_popup_append(hello_mnu, 9, "seven", "", 0);

	bbx_popup_append(goodbye_mnu, 100, "Bye bye", "b", 0);
	bbx_popup_append(goodbye_mnu, 101, "Farewell", "", 0);
	
	bbx_popup_append(help_mnu, 200, "Help", "", 0);
	bbx_popup_append(help_mnu, 200, "About", "", 0);

	app->bbx = bbx;
	app->root = root;
	app->menu = bbx_menubar(bbx, root, domenu, app);
	bbx_menubar_addmenu(app->menu, "Hello", hello_mnu);
	bbx_menubar_addmenu(app->menu, "Goodbye", goodbye_mnu);
	bbx_menubar_addmenu(app->menu, "Help", help_mnu);

	app->lab = bbx_label(bbx, root, "Fred");
	app->can = bbx_canvas(bbx, root, 100, 100, bbx_rgba(255, 0, 255, 255));
    app->but = bbx_button(bbx, root, "Press me", 0, 0);
	app->sb = bbx_scrollbar(bbx, root, BBX_SCROLLBAR_VERTICAL, 0, 0);
	app->box = bbx_editbox(bbx, root, 0, 0);
	bbx_editbox_settext(app->box, "Hello Fred");
}

void domenu(void *ptr, int d)
{
	APP *app = ptr;

	switch (d)
	{
	case 1:
		bbx_getopenfile(app->bbx, "*");
		break;
	}
}

void layout(void *obj, int width, int height)
{
	APP *app = obj;

	bbx_setpos(app->bbx, app->menu, 0, 0, width, 20);
	bbx_setpos(app->bbx, app->lab, 10, 20, 100, 30);
	bbx_setpos(app->bbx, app->can, 10, 50, 100, 100);
	bbx_setpos(app->bbx, app->but, 120, 20, 100, 25);
	bbx_setpos(app->bbx, app->sb, 250, 20, 20, 150);
	bbx_setpos(app->bbx, app->box, 280, 25, 200, 150);
}