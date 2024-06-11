#include <stdlib.h>
#include "BabyX.h"
#include "loadimage.h"

typedef struct
{
	BABYX *bbx;
	BBX_Panel *root;
    BBX_Canvas *can;
	BBX_Button *ok_but;
    unsigned char *image;
    int width;
    int height;
} APP;

void createapp(void *obj, BABYX *bbx, BBX_Panel *root);
void killapp(void *obj);
void layoutapp(void *obj, int width, int height);
void ok_pressed(void *obj);

int main(int argc, char**argv)
{
	APP app;
    
    if (argc == 2)
    {
        unsigned char *rgba;
        int err;
        
        rgba = loadrgba(argv[1], &app.width, &app.height, &err);
        if (!rgba)
            exit(EXIT_FAILURE);
        app.image = rgba;
    }
    else
        exit(EXIT_FAILURE);
    
	startbabyx("Baby X Image Viewer", 20 + app.width, 80 + app.height, createapp, layoutapp, &app);
    free(app.image);

	return 0;
}


void createapp(void *obj, BABYX *bbx, BBX_Panel *root)
{
	APP *app = obj;
	app->bbx = bbx;
	app->root = root;
    app->can = bbx_canvas(bbx, root, app->width, app->height, bbx_color("white"));
	app->ok_but = bbx_button(bbx, root, "OK", ok_pressed, app);
}

void killapp(void *obj)
{
	APP *app = obj;

    bbx_canvas_kill(app->can);
	bbx_button_kill(app->ok_but);
}


void layoutapp(void *obj, int width, int height)
{
	APP *app = obj;

    bbx_setpos(app->bbx, app->can, 10, 10, app->width, app->height);
	bbx_setpos(app->bbx, app->ok_but, width / 2 - 25, height - 50, 50, 25);
    bbx_canvas_setimage(app->can, app->image, app->width, app->height);
    bbx_canvas_flush(app->can);
}

void ok_pressed(void *obj)
{
	APP *app = obj;

	killapp(app);
	stopbabyx(app->bbx);
}
