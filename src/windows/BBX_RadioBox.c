#include <stdlib.h>

#include "BabyX.h"

typedef struct
{
  void *ptr;
  int index;
} RADIOELEMENT;

typedef struct
{
  BABYX *bbx;
  BBX_Panel *pan;
  BBX_Label **lab;
  BBX_Canvas **can;
  RADIOELEMENT *el;
  int *disabled;
  int N;
  int checked;
  void (*fptr)(void *ptr, int index);
  void *ptr;

} RADIOPANEL;


static void layout(void *obj, int width, int height);
static void mousefunc(void *obj, int action, int x, int y, int buttons);
static void drawelement(RADIOPANEL *pan, int index);


BBX_RadioBox *bbx_radiobox(BABYX *bbx, BBX_Panel *parent, char **text, int N, void (*fptr)(void *ptr, int index), void *ptr )
{
  return BBX_radiobox(bbx, parent->win, text, N, fptr, ptr);
}

BBX_RadioBox *BBX_radiobox(BABYX *bbx, HWND parent, char **text, int N, void (*fptr)(void *ptr, int index), void *ptr )
{
  RADIOPANEL *rad;
  int i;

  rad = bbx_malloc(sizeof(RADIOPANEL));
  rad->bbx = bbx;
  rad->lab = bbx_malloc(N * sizeof(BBX_Label *));
  rad->can = bbx_malloc(N * sizeof(BBX_Canvas *));
  rad->el = bbx_malloc(N * sizeof(RADIOELEMENT));
  rad->disabled = bbx_malloc(N * sizeof(int));

  rad->pan = BBX_panel(bbx, parent, "radiobox", layout, rad); 
  for(i=0;i<N;i++)
  {
    rad->lab[i] = bbx_label(bbx, rad->pan, text[i]);
    rad->can[i] = bbx_canvas(bbx, rad->pan, 10, 10, 0);
    bbx_canvas_setmousefunc(rad->can[i], mousefunc, &rad->el[i]);
    rad->el[i].index = i;
    rad->el[i].ptr = rad;
    rad->disabled[i] = 0;
  }
  
  rad->N = N;
  rad->fptr = fptr;
  rad->ptr = ptr;
  rad->checked = -1;
  
  return rad->pan;
}

void bbx_radiobox_kill(BBX_RadioBox *rp)
{
  RADIOPANEL *rad;
  int i;

  if(rp)
  {
    rad = bbx_panel_getptr(rp);
    if(rad)
    {
      for(i=0;i<rad->N;i++)
      {
        if(rad->lab)
          bbx_label_kill(rad->lab[i]);
        if(rad->can)
          bbx_canvas_kill(rad->can[i]);
       }
       bbx_panel_kill(rad->pan);
       free(rad->lab); 
       free(rad->can);
       free(rad->el);
       free(rad->disabled);
       free(rad);
    }
  }
}


int bbx_radiobox_getselected(BBX_RadioBox *obj)
{
  RADIOPANEL *rad = bbx_panel_getptr(obj);

  return rad->checked;
}

void bbx_radiobox_setselected(BBX_RadioBox *obj, int index)
{
  RADIOPANEL *rad = bbx_panel_getptr(obj);
  int old;
  
  old = rad->checked;
  rad->checked = index;
  if(old >= 0 && old < rad->N)
    drawelement(rad, old);
 
  if(rad->checked >= 0 && rad->checked < rad->N)
    drawelement(rad, rad->checked);    
}

int bbx_radiobox_disable(BBX_RadioBox *obj, int index)
{
    RADIOPANEL *rad = bbx_panel_getptr(obj);
    if(index >= 0 && index < rad->N)
    {
      bbx_label_setforeground(rad->lab[index], bbx_color("DimGray"));
      rad->disabled[index] = 1;
      drawelement(rad, index);
      return 0;
    }
    return -1;
}

int bbx_radiobox_enable(BBX_RadioBox *obj, int index)
{
    RADIOPANEL *rad = bbx_panel_getptr(obj);

    if(index >= 0 && index < rad->N)
    {
      bbx_label_setforeground(rad->lab[index], bbx_color("black"));
      rad->disabled[index] = 0;
      drawelement(rad, index);
      return 0;
    }

    return -1;
}

static void layout(void *obj, int width, int height)
{
  RADIOPANEL *rad = obj;
  int i;

  for(i=0;i<rad->N;i++)
  {
    bbx_setpos(rad->bbx, rad->lab[i], 0, i * height/rad->N, width-20, height/rad->N);
    bbx_setpos(rad->bbx, rad->can[i], width-20, i *height/rad->N + (height/rad->N-10)/2, 10, 10);
    drawelement(rad, i);
  }
}

static void mousefunc(void *obj, int action, int x, int y, int buttons)
{
  RADIOELEMENT *el;
  RADIOPANEL *rad;
  int old;

  el = obj;
  rad = el->ptr;


  if(action == BBX_MOUSE_CLICK && (buttons & BBX_MOUSE_BUTTON1) )
  {
    if(!rad->disabled[el->index])
    {
      old = rad->checked;
      rad->checked = el->index;
      if(old >= 0 && old < rad->N)
        drawelement(rad, old);
      drawelement(rad, rad->checked); 
      if(rad->fptr)
        (*rad->fptr)(rad->ptr, rad->checked);    
    }
  }
}

static void drawelement(RADIOPANEL *rad, int index)
{
  unsigned char *rgba;
  int width, height;
  int x, y;
  BBX_RGBA col;
  BBX_RGBA grey;

  rgba = bbx_canvas_rgba(rad->can[index], &width, &height);

  grey = bbx_color("gray");
  col = bbx_color("white");
  for(y=0;y<height;y++)
    for(x=0;x<width;x++)
      if( (x-5)*(x-5)+(y-5)*(y-5) < 25)
      {
        rgba[(y*width+x)*4] = bbx_red(col);
        rgba[(y*width+x)*4+1] = bbx_green(col);
        rgba[(y*width+x)*4+2] = bbx_blue(col);     
      }
      else
      {
        rgba[(y*width+x)*4] = bbx_red(grey);
        rgba[(y*width+x)*4+1] = bbx_green(grey);
        rgba[(y*width+x)*4+2] = bbx_blue(col);     
      }

 
  col = bbx_color("black");
  if(rad->checked == index)
  {
    for(y=0;y<width;y++)
      for(x=0;x<height;x++)
        if( (x-5)*(x-5)+(y-5)*(y-5) < 16)
	{
          rgba[(y*width+x)*4] = bbx_red(col);
          rgba[(y*width+x)*4+1] = bbx_green(col);
          rgba[(y*width+x)*4+2] = bbx_blue(col);     
	}
  }

  bbx_canvas_flush(rad->can[index]);
}
