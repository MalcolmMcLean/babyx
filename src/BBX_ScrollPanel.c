#include <stdlib.h>
#include "BabyX.h"

typedef struct
{
  int x;
  int y;
  int width;
  int height;
} RECT;

typedef struct
{
  BABYX *bbx;
  int width;
  int height;
  int pwidth;
  int pheight;
  int top;
  int left;
  void **bbx_children;
  int Nchildren;
  RECT *bbx_pos;
  BBX_Scrollbar *sbv;
  BBX_Scrollbar *sbh;
} BBX_SP;

static void layout(void *obj, int width, int height);
static void scrollvertical(void *obj, int pos);
static void scrollhorizontal(void *obj, int pos);

BBX_ScrollPanel *bbx_scrollpanel(BABYX *bbx, BBX_Panel *parent, int width, int height, int bars)
{
  BBX_SP *sp;
  BBX_Panel *answer;

  sp = bbx_malloc(sizeof(BBX_SP));
  sp->bbx = bbx;
  answer = bbx_panel(bbx, parent, "scrollpanel", layout, sp);
  sp->width = width;
  sp->height = height;
  sp->pwidth = -1;
  sp->pheight = -1; 
  sp->top = 0;
  sp->left = 0;
  sp->bbx_children  = 0;
  sp->bbx_pos = 0;
  sp->Nchildren = 0;
  sp->sbv = 0;
  sp->sbh = 0;

  if(bars & BBX_SCROLLBAR_VERTICAL)
    sp->sbv = bbx_scrollbar(bbx, answer, BBX_SCROLLBAR_VERTICAL, scrollvertical, answer);
  if(bars & BBX_SCROLLBAR_HORIZONTAL)
    sp->sbh = bbx_scrollbar(bbx, answer, BBX_SCROLLBAR_HORIZONTAL, scrollhorizontal, answer);

  return answer;
}

void bbx_scrollpanel_kill(BBX_ScrollPanel *obj)
{
  BBX_SP *sp;

  if(obj)
  {
    sp = bbx_panel_getptr(obj);
    free(sp->bbx_children);
    free(sp->bbx_pos);
    if(sp->sbv)
      bbx_scrollbar_kill(sp->sbv);
    if(sp->sbh)
      bbx_scrollbar_kill(sp->sbh);
    free(sp);
    bbx_panel_kill(obj);
  }
}

int bbx_scrollpanel_add(BBX_ScrollPanel *pan, void *bbxobj, int x, int y, int width, int height)
{
  BBX_SP *sp;

  sp = bbx_panel_getptr(pan);
  sp->bbx_pos = bbx_realloc(sp->bbx_pos, (sp->Nchildren +1) * sizeof(RECT));
  sp->bbx_children = bbx_realloc(sp->bbx_children, (sp->Nchildren +1) * sizeof(void *));
  sp->bbx_pos[sp->Nchildren].x = x;
  sp->bbx_pos[sp->Nchildren].y = y;
  sp->bbx_pos[sp->Nchildren].width = width;
  sp->bbx_pos[sp->Nchildren].height = height;
  sp->bbx_children[sp->Nchildren] = bbxobj;
  sp->Nchildren++;  

  return 0;
}

static void layout(void *obj, int width, int height)
{
  int i;
  int adj = 0;
  BBX_SP *sp = obj;

  if(sp->top > sp->height - height)
    sp->top = sp->height - height; 
  if(sp->top < 0)
    sp->top = 0;
    
  if(sp->left > sp->width - width)
    sp->left = sp->width - width; 
  if(sp->left < 0)
    sp->left = 0;
 
  if(sp->pwidth != width || sp->pheight != height)
  {  
    if(sp->sbv)
    {
      adj = sp->sbh ? 20 : 0;
      bbx_setpos(sp->bbx, sp->sbv, width-20, 0, 20, height -adj);
      bbx_scrollbar_set(sp->sbv,sp->height, height, sp->top); 
    }
    if(sp->sbh)
    {
       adj = sp->sbv ? 20 : 0;
       bbx_setpos(sp->bbx, sp->sbh, 0, height-20, width - adj, 20);
       bbx_scrollbar_set(sp->sbh, sp->width, width, sp->left); 
    }
    sp->pwidth = width;
    sp->pheight = height;
  }

  for(i=0;i<sp->Nchildren;i++)
    bbx_setpos(sp->bbx, sp->bbx_children[i], sp->bbx_pos[i].x -sp->left, sp->bbx_pos[i].y - sp->top, sp->bbx_pos[i].width, sp->bbx_pos[i].height);
}

static void scrollvertical(void *obj, int pos)
{
  BBX_SP *sp = bbx_panel_getptr(obj);
  sp->top = pos;
  layout(sp, sp->pwidth, sp->pheight);
}

static void scrollhorizontal(void *obj, int pos)
{
  BBX_SP *sp = bbx_panel_getptr(obj);
  sp->left = pos;
  layout(sp, sp->pwidth, sp->pheight);
}
