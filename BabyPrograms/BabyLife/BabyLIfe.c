#include "BabyX.h"
#include <stdlib.h>
#include <stdio.h>
#include "string.h"

typedef struct
{
    unsigned char *cell;
    int width;
    int height;
} LIFE;

typedef struct
{
    BABYX *bbx;
    BBX_Panel *root;
    BBX_Label *message_lab;
    BBX_Canvas *life_can;
    BBX_Button *ok_but;
    void *ticker;
    LIFE *life;
} APP;

void createapp(void *obj, BABYX *bbx, BBX_Panel *root);
void killapp(void *obj);
void layoutapp(void *obj, int width, int height);
void ok_pressed(void *obj);

LIFE *life(int width, int height)
{
    int i;
    LIFE *life = bbx_malloc(sizeof(LIFE));
    life->cell = bbx_malloc(width * height);
    life->width = width;
    life->height = height;
    for (i = 0; i < width * height; i++)
        life->cell[i] = (rand() % 3) ? 0 : 1;
    
    return life;
}

void kill_life(LIFE *life)
{
    if (life)
    {
        free(life->cell);
        free(life);
    }
}

int life_NNeighbours(LIFE *life, int x, int y)
{
    int ix, iy;
    int nx, ny;
    int answer = 0;
    
    if (x < 0 || x >= life->width)
        return 0;
    if (y < 0 || y >= life->height)
        return 0;
    for (iy = -1; iy < 2; iy++)
    {
        ny = y + iy;
        if (ny < 0)
            ny = life->height - 1;
        if (ny >= life->height)
            ny = 0;
        for (ix = -1; ix < 2; ix++)
        {
            nx = x + ix;
            if (nx < 0)
                nx = life->width - 1;
            if (nx >= life->width)
                nx = 0;
            if (nx == x && ny == y)
                ;
            else
                answer += life->cell[ny * life->width + nx];
        }
    }
      
    return  answer;
}

void life_step(LIFE *life)
{
    int x, y;
    int nn;
    unsigned char *buff;
    
    buff = bbx_malloc(life->width *life->height);
    for (y = 0; y < life->height; y++)
    {
        for (x = 0; x < life->width; x++)
        {
            nn = life_NNeighbours(life, x, y);
            if (life->cell[y * life->width + x])
            {
                if (nn == 2 || nn == 3)
                    buff[y*life->width+x] = 1;
                else
                    buff[y * life->width + x] = 0;
            }
            else
            {
                  if (nn == 3)
                     buff[y*life->width+x] = 1;
                  else
                     buff[y*life->width+x] = 0;
            }
        }
    }
    memcpy(life->cell, buff, life->width * life->height);
    free(buff);
}

void tick(void *obj);


int main(void)
{
	APP app;
    srand((unsigned)time(0));
	startbabyx("BabyLife", 320, 200, createapp, layoutapp, &app);

	return 0;
}


void createapp(void *obj, BABYX *bbx, BBX_Panel *root)
{
	APP *app = obj;
	app->bbx = bbx;
	app->root = root;
	app->message_lab = bbx_label(bbx, root, "Hello BabyLife");
    app->life_can = bbx_canvas(bbx, root, 300, 150, bbx_color("black"));
	app->ok_but = bbx_button(bbx, root, "OK", ok_pressed, app);
    app->life = life(100, 50);
    
    app->ticker = bbx_addticker(app->bbx, 200, tick, app);
    
    /*
    void *bbx_addticker(BABYX *bbx, int ms_interval, void (*fptr)(void *ptr), void *ptr);
    void bbx_removeticker(BABYX *bbx, void *ticker);
     */
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
    bbx_setpos(app->bbx, app->life_can, 10, 10, 300, 150);
	bbx_setpos(app->bbx, app->ok_but, width / 2 - 25, height - 50, 50, 25);
}

void tick(void *obj)
{
    APP *app = obj;
    int x, y;
    int width_can, height_can;
    unsigned char *rgba;
    
    rgba = bbx_canvas_rgba(app->life_can, &width_can, &height_can);
    for (y = 0; y < height_can; y+=3)
    {
        for (x =0; x < width_can; x+=3)
        {
            BBX_RGBA cell;
            cell = app->life->cell[(y/3)*100+(x/3)] ? bbx_rgba(255, 255, 255, 255) : bbx_rgba(0, 0, 0, 255);
            bbx_rectangle(rgba, width_can, height_can, x, y, x + 3, y + 3, cell);
        }
    }
    bbx_canvas_flush(app->life_can);
   
    life_step(app->life);
}

void ok_pressed(void *obj)
{
	APP *app = obj;

    bbx_removeticker(app->bbx, app->ticker);
    app->ticker = 0;
	killapp(app);
	stopbabyx(app->bbx);
}
