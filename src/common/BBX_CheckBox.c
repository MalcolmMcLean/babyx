#include <stdlib.h>
#include "BabyX.h"


typedef struct
{
  BABYX *bbx;
  BBX_Panel *pan;
  BBX_Label *lab;
  BBX_Canvas *can;
  int checked;
  int disabled;
  void (*fptr)(void *ptr, int state);
  void *ptr;
} CHECKBOX;

static void layout(void *obj, int width, int height);
static void mousefunc(void *obj, int action, int x, int y, int buttons);
static void redraw(CHECKBOX *chk);

BBX_CheckBox *bbx_checkbox(BABYX *bbx, BBX_Panel *parent, char *text, void (*fptr)(void *ptr, int state), void *ptr)
{
  CHECKBOX *chk;

  chk = bbx_malloc(sizeof(CHECKBOX));
  chk->bbx = bbx;
  chk->pan = bbx_panel(bbx, parent, "checkbox", layout, chk);
  chk->lab = bbx_label(bbx, chk->pan, text);
  chk->can = bbx_canvas(bbx, chk->pan, 10, 10, 0);
  chk->checked = 0;
  chk->disabled = 0;
  chk->fptr = fptr;
  chk->ptr = ptr;
  bbx_canvas_setmousefunc(chk->can, mousefunc, chk);
  return chk->pan; 
}

void bbx_checkbox_kill(BBX_CheckBox *box)
{
  CHECKBOX *chk;

  if(box)
  {
    chk = bbx_panel_getptr(box);
    if(chk)
    {
      bbx_label_kill(chk->lab);
      bbx_canvas_kill(chk->can);
      bbx_panel_kill(chk->pan);
    }
    free(chk);
  }
}

int bbx_checkbox_getstate(BBX_CheckBox *obj)
{
   CHECKBOX *chk = bbx_panel_getptr(obj);
   return chk->checked;
}


int bbx_checkbox_setstate(BBX_CheckBox *obj, int checked)
{
   CHECKBOX *chk = bbx_panel_getptr(obj);
   
   if(chk->checked != checked)
   {
     chk->checked = checked;
     redraw(chk);
   } 
   return 0;
}

void bbx_checkbox_enable(BBX_CheckBox *obj)
{
  CHECKBOX *chk = bbx_panel_getptr(obj);
  if(chk->disabled)
  {
    bbx_label_setforeground(chk->lab, bbx_color("black"));
    chk->disabled = 0;
    redraw(chk);
  }
}

void bbx_checkbox_disable(BBX_CheckBox *obj)
{
   CHECKBOX *chk = bbx_panel_getptr(obj);
   if(chk->disabled == 0)
   {
     bbx_label_setforeground(chk->lab, bbx_color("light gray"));
     chk->disabled = 1;
     redraw(chk);
   }
}


static void layout(void *obj, int width, int height)
{
  CHECKBOX *chk = obj;
  BABYX *bbx = chk->bbx;

  bbx_setpos(bbx, chk->lab, 0, 0, width-20, height);
  bbx_setpos(bbx, chk->can, width-20, height/2-5, 10, 10);
  redraw(chk);
}

static void mousefunc(void *obj, int action, int x, int y, int buttons)
{
  CHECKBOX *chk = obj;

  if(action == BBX_MOUSE_CLICK && (buttons & BBX_MOUSE_BUTTON1))
  {
    if(!chk->disabled)
    {
      chk->checked ^= 1;
      redraw(chk);
      if(chk->fptr)
        (*chk->fptr)(chk->ptr, chk->checked);
    }   
  }
}

static void redraw(CHECKBOX *chk)
{
  unsigned char *rgba;
  int width, height;
  BBX_RGBA cola;

  rgba = bbx_canvas_rgba(chk->can, &width, &height);
  if(chk->disabled)
    cola = bbx_color("light gray");
  else
    cola = bbx_color("white");
  bbx_rectangle(rgba, width, height, 0, 0, width-1, height-1, cola);
  cola = bbx_color("dim gray");
  bbx_line(rgba, width, height, 0, 0, 0, height-1, cola);
  bbx_line(rgba, width, height, 0, 0, width-1, 0, cola);
  cola = bbx_color("light gray");
  bbx_line(rgba, width, height, width-1, 0, width-1, height-1, cola);
  bbx_line(rgba, width, height, 0, height-1, width-1, height-1, cola);

  if(chk->checked)
  {
    cola = bbx_color("red");
    bbx_lineaa(rgba, width, height, 0, 0, width, height, 1.5, cola);  
    bbx_lineaa(rgba, width, height, 0, width, height, 0, 1.5, cola);   
  }

  bbx_canvas_flush(chk->can);
}
