#include <stdlib.h>
#include <string.h>

#include "BabyX.h"



typedef struct
{
  BABYX *bbx;
  BBX_Panel *pan;
  BBX_Canvas *can;
  char *str;
  void (*fptr)(void *ptr);
  void *ptr;
  int disabled;
  struct bitmap_font *font;
} BUTTON;

static void layout(void *obj, int width, int height);
static void mousefunc(void *obj, int action, int x, int y, int buttons);
static void redraw(BUTTON *but);

BBX_Button *bbx_button(BABYX *bbx, BBX_Panel *parent, char *str, void (*fptr)(void *ptr), void *ptr)
{
  BUTTON *but;

  but = bbx_malloc(sizeof(BUTTON));
  but->bbx = bbx;
  but->str = bbx_strdup(str);
  but->can = 0;
  but->fptr = fptr;
  but->ptr = ptr;
  but->font = bbx->gui_font;
  but->disabled = 0;
  but->pan = bbx_panel(bbx, parent, "button", layout, but);

  return but->pan;
}

void bbx_button_kill(BBX_Button *obj)
{
  BUTTON *but;

  if(obj)
  {
    but = bbx_panel_getptr(obj);
    if(but)
    {
      free(but->str);
      bbx_canvas_kill(but->can);
      bbx_panel_kill(but->pan);
    }
    free(but);
  }
}

void bbx_button_settext(BBX_Button *obj, char *str)
{
  BUTTON *but;

  but = bbx_panel_getptr(obj);
  free(but->str);
  but->str = bbx_strdup(str);
  redraw(but);
}

void bbx_button_enable(BBX_Button *obj)
{
  BUTTON *but;

  but = bbx_panel_getptr(obj);
  but->disabled = 0;
  redraw(but);
}

void bbx_button_disable(BBX_Button *obj)
{
  BUTTON *but;

  but = bbx_panel_getptr(obj);
  but->disabled = 1;
  redraw(but);
}


static void layout(void *obj, int width, int height)
{
  BUTTON *but = obj;

  if(but->can)
    bbx_canvas_kill(but->can);
  but->can = bbx_canvas(but->bbx, but->pan, width, height, 0);
  bbx_setpos(but->bbx, but->can, 0, 0, width, height);
  bbx_canvas_setmousefunc(but->can, mousefunc, but);
  redraw(but);
}

static void mousefunc(void *obj, int action, int x, int y, int buttons)
{
  BUTTON *but = obj;
  if( action == BBX_MOUSE_CLICK && (buttons & BBX_MOUSE_BUTTON1) )
  {
    if(but->fptr && but->disabled == 0)
      (*but->fptr)(but->ptr);
  }
}

static void redraw(BUTTON *but)
{
  unsigned char *rgba;
  int width, height;
  int font_height;
  BBX_RGBA col;
  int x, y;
  int i;
 
  if(!but->can)
    return;

  font_height = but->font->ascent + but->font->descent;
  rgba = bbx_canvas_rgba(but->can, &width, &height);
  col = bbx_color("gray");

  for(i=0;i<width*height;i++)
  {
    rgba[i*4] = bbx_red(col);
    rgba[i*4+1] = bbx_green(col);
    rgba[i*4+2] = bbx_blue(col);
    rgba[i*4+3] = 0xFF;
  }
  col = bbx_color("LightGray");
  for(x=0;x<width;x++)
  {
    rgba[(0*width+x)*4] = bbx_red(col);
    rgba[(0*width+x)*4+1] = bbx_green(col);
    rgba[(0*width+x)*4+2] = bbx_blue(col);
  }
  for(y=0;y<height;y++)
  {
    rgba[(y*width+0)*4] = bbx_red(col);
    rgba[(y*width+0)*4+1] = bbx_green(col);
    rgba[(y*width+0)*4+2] = bbx_blue(col);
  }

  col = bbx_color("DimGray");

  for(x=0;x<width;x++)
  {
    rgba[((height-1)*width+x)*4] = bbx_red(col);
    rgba[((height-1)*width+x)*4+1] = bbx_green(col);
    rgba[((height-1)*width+x)*4+2] = bbx_blue(col);
  }
  for(y=0;y<height;y++)
  {
    rgba[(y*width+width-1)*4] = bbx_red(col);
    rgba[(y*width+width-1)*4+1] = bbx_green(col);
    rgba[(y*width+width-1)*4+2] = bbx_blue(col);
  }


  if(but->disabled)
    col = bbx_color("DimGray");
  else  
    col = bbx_color("black");
  y =  but->font->ascent + (height - font_height)/2;
  x = (width - bbx_textwidth(but->font, but->str, strlen(but->str)))/2;
  bbx_drawutf8(rgba, width, height, x, y, but->str, strlen(but->str), but->font, col);
   
  bbx_canvas_flush(but->can);
}
