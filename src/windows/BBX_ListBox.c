#include <stdlib.h>

#include "BabyX.h"

typedef struct
{
  BABYX *bbx;
  int N;
  char **str;
  int sel;
  BBX_Label **lab;
  BBX_Scrollbar *sbv;
  BBX_ListBox *pan;
  int pwidth;
  int pheight;
  int top;
  int shown;
  int labheight;
  void (*fptr)(void *ptr, int selected);
  void *ptr;
} BBX_LB;

static void layout(void *obj, int width, int height);
static void mousefunc(void *obj, int action, int x, int y, int buttons);
static void scrollvertical(void *obj, int pos);
static void unhighlightselection(BBX_LB *lb);
static void highlightselection(BBX_LB *lb);
static void resetstrings(BBX_LB *lb);

BBX_ListBox *bbx_listbox(BABYX *bbx, BBX_Panel *parent, void (*fptr)(void *ptr, int selected), void *ptr)
{
  BBX_LB *lb;
  BBX_Panel *answer;

  lb = bbx_malloc(sizeof(BBX_LB));
  lb->bbx = bbx;
  answer = bbx_panel(bbx, parent, "listbox", layout, lb);
  lb->sbv = bbx_scrollbar(bbx, answer, BBX_SCROLLBAR_VERTICAL, scrollvertical, answer);
  lb->pwidth = -1;
  lb->pheight = -1;
  lb->str = 0;
  lb->N = 0;
  lb->lab = 0;
  lb->pan = answer;
  lb->shown = 0;
  lb->top = 0;
  lb->labheight = 20;
  lb->sel = -1;
  lb->fptr = fptr;
  lb->ptr = ptr;
  bbx_panel_setmousefunc(answer, mousefunc, lb);
  
  return answer;
}

void bbx_listbox_kill(BBX_ListBox *obj)
{
  BBX_LB *lb;
  int i;

  if(obj)
  {
    lb = bbx_panel_getptr(obj);
    if(lb->sbv)
      bbx_scrollbar_kill(lb->sbv);
    if(lb->lab)
      for(i=0;i<lb->shown;i++)
        bbx_label_kill(lb->lab[i]);
    if(lb->str)
      for(i=0;i<lb->N;i++)
        free(lb->str[i]);
    free(lb->str);
    free(lb->lab);
    free(lb);
    bbx_panel_kill(obj);
  }
}

int bbx_listbox_addstring(BBX_ListBox *box, char *str)
{
  BBX_LB *lb;

  lb = bbx_panel_getptr(box);
  lb->str = bbx_realloc(lb->str, (lb->N + 1) * sizeof(char *)); 
  lb->str[lb->N] = bbx_strdup(str);
  lb->N++;
  if(lb->N < lb->top + lb->shown)
    bbx_label_settext(lb->lab[lb->N-lb->top-1], str);
  bbx_scrollbar_set(lb->sbv, lb->N, lb->shown, lb->top);
  
  return 0;
}

void bbx_listbox_clear(BBX_ListBox *obj)
{
  BBX_LB *lb;
  int i;

  lb = bbx_panel_getptr(obj);
  unhighlightselection(lb);
  lb->sel = -1;
  for(i=0;i<lb->N;i++)
    free(lb->str[i]);
  lb->N = 0;
  lb->top = 0;
  bbx_scrollbar_set(lb->sbv, lb->N, lb->shown, lb->top);
  resetstrings(lb);
}

int bbx_listbox_getselected(BBX_ListBox *box)
{
  BBX_LB *lb;

  lb = bbx_panel_getptr(box);
  return lb->sel;
}

int bbx_listbox_setselected(BBX_ListBox *box, int idx)
{
  BBX_LB *lb;

  lb = bbx_panel_getptr(box);
  if(lb->sel != idx)
  {
    unhighlightselection(lb);
    lb->sel = (idx >= 0 && idx < lb->N) ? idx : -1;
    highlightselection(lb);
  }

  return 0;
}

static void layout(void *obj, int width, int height)
{
  BBX_LB *lb = obj;
  BABYX *bbx = lb->bbx;
  int i;

  lb->top = 0;
  lb->pwidth = width;
  lb->pheight = height;
  if(lb->shown != height/lb->labheight)
  {
     
    for(i=0;i<lb->shown;i++)
      bbx_label_kill(lb->lab[i]);
    lb->shown = height/lb->labheight;
    lb->lab = bbx_realloc(lb->lab, lb->shown * sizeof(BBX_Label *));
    for(i=0;i<lb->shown;i++)
    {
      lb->lab[i] = bbx_label(bbx, lb->pan, "");
      bbx_label_setbackground(lb->lab[i], bbx_color("white"));
      bbx_label_setalignment(lb->lab[i], BBX_ALIGN_LEFT);
    }
  }
  resetstrings(lb);
  bbx_setpos(bbx, lb->sbv, width-20, 0, 20, height);
  for(i=0;i<lb->shown;i++)
    bbx_setpos(bbx, lb->lab[i], 0, i * lb->labheight, width-20, lb->labheight);
  bbx_scrollbar_set(lb->sbv, lb->N, lb->shown, lb->top);
}

static void mousefunc(void *obj, int action, int x, int y, int buttons)
{
  BBX_LB *lb = obj;
  int idx;

  if(action == BBX_MOUSE_CLICK && (buttons & BBX_MOUSE_BUTTON1))
  {
    idx = y / lb->labheight + lb->top;
    if(idx != lb->sel)
    {
      unhighlightselection(lb);
      if(idx >= 0 && idx < lb->N)
        lb->sel = idx;
      else
        lb->sel = -1;
      highlightselection(lb);
      if(lb->fptr)
        (*lb->fptr)(lb->ptr, lb->sel);
    }
  }
  
}
 
static void scrollvertical(void *obj, int pos)
{
  BBX_LB *lb = bbx_panel_getptr(obj);
  if( lb->top != pos)
  {
    unhighlightselection(lb);
    lb->top = pos;
    resetstrings(lb);
    highlightselection(lb);
  }
}

static void unhighlightselection(BBX_LB *lb)
{
  if(lb->sel >= 0 && lb->sel - lb->top < lb->shown && lb->sel - lb->top >= 0)
  {
    bbx_label_setbackground(lb->lab[lb->sel-lb->top], bbx_color("white"));
    bbx_label_setforeground(lb->lab[lb->sel-lb->top], bbx_color("black"));
  }
}


static void highlightselection(BBX_LB *lb)
{
  if(lb->sel >= 0 && lb->sel - lb->top < lb->shown && lb->sel - lb->top >= 0)
  {
    bbx_label_setbackground(lb->lab[lb->sel-lb->top], bbx_color("blue"));
    bbx_label_setforeground(lb->lab[lb->sel-lb->top], bbx_color("white"));
  }
}

static void resetstrings(BBX_LB *lb)
{
  int i;

  for(i=0;i<lb->shown;i++)
  {
    if(i + lb->top < lb->N)
      bbx_label_settext(lb->lab[i], lb->str[i+lb->top]);
    else
      bbx_label_settext(lb->lab[i], "");
  }

}
