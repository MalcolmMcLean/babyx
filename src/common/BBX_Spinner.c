#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <ctype.h>
#include <math.h>

#include "BabyX.h"

#define BBX_SPINNER_REAL 1
#define BBX_SPINNER_LOGARITHMIC 2
#define BBX_SPINNER_INTERACTIVE 4

typedef struct
{
  BABYX *bbx;
  BBX_Panel *pan;
  BBX_LineEdit *edt;
  BBX_Canvas *can;
  double val;
  double minval;
  double maxval;
  double delta;
  void (*change)(void *ptr, double val);
  void *ptr;
  int direction;
  void *ticker;
  char *fmt;
  double spunval;
  int counter;
  int mode;
  int disabled;
} BBX_SPIN;

static void layout(void *obj, int width, int height);
static void doedit(void *obj, char *text);
static void domousepress(void *obj, int action, int x, int y, int buttons);
static void ticktick(void *obj);
static void setvalue(BBX_SPIN *spin, double val);
static void drawupdown(BBX_SPIN *spin);
static char *trim(char *str);

BBX_Spinner *bbx_spinner(BABYX *bbx, BBX_Panel *pan, double val, double minval, double maxval, double delta, void (*change)(void *ptr, double val), void *ptr)
{
  BBX_SPIN *spin;
  char buff[256];

  sprintf(buff, "%g", val);
  spin = bbx_malloc(sizeof(BBX_SPIN));
  spin->bbx = bbx;
  spin->pan = bbx_panel(bbx, pan, "spinner", layout, spin);
  spin->edt = bbx_lineedit(bbx, spin->pan, buff, doedit, spin);
  spin->can = bbx_canvas(bbx, spin->pan, 10, 20, BBX_Color("white"));

  spin->val = val;
  spin->minval = minval;
  spin->maxval = maxval;
  spin->delta = delta;
  spin->change = change;
  spin->ptr = ptr;

  spin->fmt = bbx_strdup("%g");
  spin->mode = 0;
  spin->direction = 0;
  spin->ticker = 0;
  spin->disabled = 0;
   
  bbx_canvas_setmousefunc(spin->can, domousepress, spin);

  return spin->pan;
  
}

void bbx_spinner_kill(BBX_Spinner *sp)
{  
  BBX_SPIN *spin;

  if(sp)
  {
    spin = bbx_panel_getptr(sp);
    if(spin)
    {
      bbx_lineedit_kill(spin->edt);
      bbx_canvas_kill(spin->can);
      if(spin->ticker)
        bbx_removeticker(spin->bbx, spin->ticker);
      free(spin->fmt);
      free(spin);
    }
    bbx_panel_kill(sp);
  }  
}

double bbx_spinner_getvalue(BBX_Spinner *sp)
{
  BBX_SPIN *spin = bbx_panel_getptr(sp);
  return spin->val;
}

void bbx_spinner_setvalue(BBX_Spinner *sp, double val)
{
  BBX_SPIN *spin = bbx_panel_getptr(sp);

  setvalue(spin, val);
}

void bbx_spinner_setparams(BBX_Spinner *sp, double val, double minval, double maxval, double delta)
{
   BBX_SPIN *spin = bbx_panel_getptr(sp);
   spin->minval = minval;
   spin->maxval = maxval;
   spin->delta = delta;
   setvalue(spin, val);
}

int bbx_spinner_setmode(BBX_Spinner *sp, int mode)
{
  BBX_SPIN *spin = bbx_panel_getptr(sp);
  int answer;

  answer = spin->mode;
  spin->mode = mode;

  return answer;
}

void bbx_spinner_setformat(BBX_Spinner *sp, char *fmt)
{
   BBX_SPIN *spin = bbx_panel_getptr(sp);
   free(spin->fmt);
   spin->fmt = bbx_strdup(fmt);
   setvalue(spin, spin->val);
}

int bbx_spinner_spinning(BBX_Spinner *sp)
{
  BBX_SPIN *spin = bbx_panel_getptr(sp);

  return spin->ticker ? 1 : 0;
}

void bbx_spinner_enable(BBX_Spinner *sp)
{
   BBX_SPIN *spin = bbx_panel_getptr(sp);
   
   if(spin->disabled == 1)
   {
     bbx_lineedit_enable(spin->edt);
     spin->disabled = 0;
     drawupdown(spin);
   }

}

void bbx_spinner_disable(BBX_Spinner *sp)
{
   BBX_SPIN *spin = bbx_panel_getptr(sp);

   if(spin->disabled == 0)
   {
     if(spin->ticker)
     {
       bbx_removeticker(spin->bbx, spin->ticker);
       spin->ticker = 0;
     }
     bbx_lineedit_disable(spin->edt);
     spin->disabled = 1;
     drawupdown(spin);
   }
}

static void layout(void *obj, int width, int height)
{
  BBX_SPIN *spin = obj;
  int cwidth, cheight;
  int updownwidth;

  updownwidth = height/2;
  if(updownwidth > width)
    updownwidth = width-1;
  bbx_setpos(spin->bbx, spin->edt, 0, 0, width-updownwidth, height);
  bbx_getsize(spin->bbx, spin->can, &cwidth, &cheight);
  if(cwidth != updownwidth || cheight != height)
  {
    bbx_canvas_kill(spin->can);
    spin->can = bbx_canvas(spin->bbx, spin->pan, updownwidth, height, BBX_Color("white")); 
    bbx_canvas_setmousefunc(spin->can, domousepress, spin);
  }
  bbx_setpos(spin->bbx, spin->can, width-updownwidth, 0, updownwidth, height);
  drawupdown(spin);
}

static void doedit(void *obj, char *text)
{
  BBX_SPIN *spin = obj;
  double value;
  char *end;
  char *trimmed;

  trimmed = trim(text);

  value = strtod(trimmed, &end);

  setvalue(spin, value);
  if(!spin->disabled && spin->change)
    (*spin->change)(spin->ptr, spin->val);

  free(trimmed);
}

static void domousepress(void *obj, int action, int x, int y, int buttons)
{
  BBX_SPIN *spin = obj;
  int width, height;

  if(spin->disabled)
    return;

  if(action == BBX_MOUSE_CLICK && (buttons & BBX_MOUSE_BUTTON1) )
  {
    spin->spunval = spin->val;
    bbx_canvas_rgba(spin->can, &width, &height);
    if ( y < height/2)
      spin->direction = 1;
    else
      spin->direction = -1;
    spin->ticker =  bbx_addticker(spin->bbx, 100, ticktick, spin);
    spin->counter = 0;

  }
  else if(action == BBX_MOUSE_RELEASE && (buttons & BBX_MOUSE_BUTTON1) )
  {
    spin->counter = 3;
    ticktick(spin);
    if(spin->ticker)
       bbx_removeticker(spin->bbx, spin->ticker); 
    spin->ticker = 0;
    spin->direction = 0;
    if(!spin->disabled && spin->change)
      (*spin->change)(spin->ptr, spin->val);
  }
}

static void ticktick(void *obj)
{
  BBX_SPIN *spin = obj;
  double delta;

  if(spin->counter++ < 2)
    return;
 
  if(spin->mode & BBX_SPINNER_LOGARITHMIC)
  {
    delta = spin->direction * spin->delta * spin->spunval;
    if(delta == 0)
      delta = DBL_MIN;
    if(delta >= DBL_MAX/10 || delta <= -DBL_MAX/10)
      delta = spin->direction * DBL_MAX / 10;
    spin->spunval += delta;
  }
  else
    spin->spunval += spin->direction * spin->delta;
  if(spin->spunval < spin->minval)
    spin->spunval = spin->minval;
  if(spin->spunval > spin->maxval)
    spin->spunval = spin->maxval; 
  
  setvalue(spin, spin->spunval);
  if( (spin->mode & BBX_SPINNER_INTERACTIVE) && spin->change)
    (*spin->change)(spin->ptr, spin->val);
}

static void setvalue(BBX_SPIN *spin, double val)
{
  char buff[256];
  double steps;

  if( (spin->mode & BBX_SPINNER_REAL) == 0 )
  {
    steps = floor( (val - spin->minval)/spin->delta + 0.5);
    val = spin->minval + steps * spin->delta;
  }
  if(val < spin->minval)
    val = spin->minval;
  if(val > spin->maxval)
    val = spin->maxval;
  spin->val = val;
  sprintf(buff, spin->fmt, val);
  bbx_lineedit_settext(spin->edt, buff);
}

static void drawupdown(BBX_SPIN *spin)
{
  unsigned char *rgba;
  int width, height;
  BBX_RGBA higrey;
  BBX_RGBA dimgrey;
  BBX_RGBA grey;
  BBX_RGBA black;
  double upx[3];
  double upy[3];
  double downx[3];
  double downy[3];

  rgba = bbx_canvas_rgba(spin->can, &width, &height);
  grey = bbx_color("gray");
  dimgrey = bbx_color("DimGray");
  higrey = bbx_color("LightGray");
  black = bbx_color("black");

 
  bbx_rectangle(rgba, width, height, 0, 0, width-1, height-1, grey);
  bbx_line(rgba, width, height, 0, 0, 0, height-1, higrey);
  bbx_line(rgba, width, height, 0, 0, width-1, 0, higrey);
  bbx_line(rgba, width, height, 0, height-1, width-1, height-1, dimgrey);
  bbx_line(rgba, width, height, width-1, 0, width-1, height-1, dimgrey); 
  bbx_line(rgba, width, height, 0, height/2, width-1, height/2, dimgrey);
  bbx_line(rgba, width, height, 0, height/2+1, width-1, height/2+1, higrey);  
 
  upx[0] = width/2.0;
  upx[1] = 1.5;
  upx[2] = width-1;
  upy[0] = 1.0;
  upy[1] = height/2.0 - 1.5;
  upy[2] = height/2.0 - 1.5;

  downx[0] = width/2.0;
  downx[1] = 1;
  downx[2] = width-1;
  downy[0] = height-1.5;
  downy[1] = height/2.0 + 1.5;
  downy[2] = height/2.0 + 1;

  if(spin->disabled)
  {
    bbx_polygonaa(rgba, width, height, upx, upy, 3, higrey);
    bbx_polygonaa(rgba, width, height, downx, downy, 3, higrey); 
  }
  else
  {
   bbx_polygonaa(rgba, width, height, upx, upy, 3, black);
    bbx_polygonaa(rgba, width, height, downx, downy, 3, black);
 
  } 

  bbx_canvas_flush(spin->can);

}


static char *trim(char *str)
{
  char *answer = bbx_malloc(strlen(str) +1);
  int i = 0;
  if(!answer)
    return 0;
  while(isspace( (unsigned char) *str))
    str++;
  while(*str)
    answer[i++] = *str++;
  answer[i] = 0;
  i--;
  while(i >= 0 && isspace( (unsigned char) answer[i]))
    answer[i--] = 0;
  return answer;
}

