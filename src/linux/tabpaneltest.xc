#include "BabyX.h"
#include "tabpanel.h"

typedef struct
{
	BABYX *bbx;
	BBX_Panel *root;
	BBX_Panel *tab_wgt;

	BBX_Label *hellofred_lab;
	BBX_Label *hellojim_lab;
	BBX_Label *hellobert_lab;

} APP;

static void create(void *obj, BABYX *bbx, BBX_Panel *root);
static void layout(void *obj, int width, int height);

static void layout_fred(void *ptr, int width, int height, int ymin);
static void layout_bert(void *ptr, int width, int height, int ymin);
static void layout_jim(void *ptr, int width, int height, int ymin);

/*
int APIENTRY WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPTSTR    lpCmdLine,
	int       nCmdShow)
{
	APP app;


	startbabyx(hInstance, "Tab Panel test", 300, 250, create, layout, &app);
}
*/

int tabpaneltestmain(void)
{
  APP app;
  startbabyx("Tab Panel test", 300, 250, create, layout, &app);

  return 0;
}

static void create(void *obj, BABYX *bbx, BBX_Panel *root)
{
	APP *app = (APP *)obj;
	
	app->bbx = bbx;
	app->root = root;
	
	app->tab_wgt = bbx_tab(bbx, root);

	bbx_tab_addtab(app->tab_wgt, "Fred", layout_fred, app);
	bbx_tab_addtab(app->tab_wgt, "Jim", layout_jim, app);
	bbx_tab_addtab(app->tab_wgt, "Bert", layout_bert, app);

	app->hellofred_lab = bbx_label(bbx, app->tab_wgt, "Hello Fred");
	app->hellojim_lab = bbx_label(bbx, app->tab_wgt, "Hello Jim");
	app->hellobert_lab = bbx_label(bbx, app->tab_wgt, "Hello Bert");

	bbx_tab_register(app->tab_wgt, app->hellofred_lab);
	bbx_tab_register(app->tab_wgt, app->hellojim_lab);
	bbx_tab_register(app->tab_wgt, app->hellobert_lab);
}

static void layout(void *ptr, int width, int height)
{
	APP *app = ptr;


	bbx_setpos(app->bbx, app->tab_wgt, 10, 10, 200, 120);
}

static void layout_fred(void *ptr, int width, int height, int ymin)
{
	APP *app = ptr;
	bbx_setpos(app->bbx, app->hellofred_lab, 0, ymin, 100, 20);
}

static void layout_jim(void *ptr, int width, int height, int ymin)
{
	APP *app = ptr;
	bbx_setpos(app->bbx, app->hellojim_lab, 0, ymin + 10, 100, 20);
}

static void layout_bert(void *ptr, int width, int height, int ymin)
{
	APP *app = ptr;
	bbx_setpos(app->bbx, app->hellobert_lab, 20, ymin, 100, 20);
}
