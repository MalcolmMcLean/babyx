#include <stdlib.h>
#include "BabyX.h"


typedef struct
{
  BABYX *bbx;
  BBX_Panel *pan;
  BBX_Label **lab;
  BBX_Popup **sub;
  int N;
  int *xpos;
  int height;
  BBX_PopUp2 *submenu;
  int subindex;
  void (*fptr)(void *ptr, int id);
  void *ptr;
} MENUBAR;


static void layout(void *obj, int width, int height);
static void mousefunc(void *obj, int action, int x, int y, int buttons);
static void chosen(void *obj, int id);

BBX_Menubar *bbx_menubar(BABYX *bbx, BBX_Panel *parent, void (*fptr)(void *ptr, int d), void *ptr)
{
  return BBX_menubar(bbx, parent->win, fptr, ptr);
}

BBX_Menubar *BBX_menubar(BABYX *bbx, HWND parent, void (*fptr)(void *ptr, int d), void *ptr)
{
  MENUBAR *menu;

  menu = bbx_malloc(sizeof(MENUBAR));
  menu->bbx = bbx;
  menu->pan = BBX_panel(bbx, parent, "menubar", layout, menu);
  menu->N = 0;
  menu->lab = 0;
  menu->sub = 0;
  menu->xpos = bbx_malloc(1 * sizeof(int));
  menu->xpos[0] = 0;
  menu->height = 1;
  menu->submenu = 0;
  menu->subindex = -1;
  menu->fptr = fptr;
  menu->ptr = ptr;

  bbx_panel_setmousefunc(menu->pan, mousefunc, menu);
  return menu->pan;
}

void bbx_menubar_kill(BBX_Menubar *mb)
{
  MENUBAR *menu;
  int i;

  if(mb)
  {
    menu = bbx_panel_getptr(mb);
    if(menu)
    {
      if(menu->submenu)
        bbx_popup2_kill(menu->submenu);
      for(i=0;i<menu->N;i++)
        bbx_label_kill(menu->lab[i]);
      free(menu->lab);
      for(i=0;i<menu->N;i++)
          bbx_popup_kill(menu->sub[i]);
      free(menu->sub);
      free(menu->xpos);
      bbx_panel_kill(menu->pan);    
      free(menu);
    }
  } 
}

void bbx_menubar_addmenu(BBX_Menubar *mb, char *name, BBX_Popup *sub)
{
  MENUBAR *menu;
  int pwidth, pheight;

  menu = bbx_panel_getptr(mb);
  menu->lab = bbx_realloc(menu->lab, (menu->N+1) * sizeof(BBX_Label *));
  menu->xpos = bbx_realloc(menu->xpos, (menu->N+2) * sizeof(int));
  menu->sub = bbx_realloc(menu->sub, (menu->N+1) * sizeof(BBX_Popup *));
  menu->lab[menu->N] = bbx_label(menu->bbx, menu->pan, name);
  bbx_label_getpreferredsize(menu->lab[menu->N], &pwidth, &pheight);
  menu->xpos[menu->N+1] = menu->xpos[menu->N] + pwidth + 10;
  menu->sub[menu->N] = sub;
  menu->N++;
}

void bbx_menubar_disable(BBX_Menubar *mb, int id)
{
  int i;
  MENUBAR *menu = bbx_panel_getptr(mb);
  for(i=0;i<menu->N;i++)
    bbx_popup_disable(menu->sub[i], id);
}

void bbx_menubar_enable(BBX_Menubar *mb, int id)
{
  int i;
  MENUBAR *menu = bbx_panel_getptr(mb);
  for(i=0;i<menu->N;i++)
    bbx_popup_enable(menu->sub[i], id);
}

void bbx_menubar_handlepointer(BBX_Menubar *mb)
{
  MENUBAR *menu = bbx_panel_getptr(mb);
  int x, y;
  int i;

  if(bbx_panel_gotmouse(mb, &x, &y))
  {
   for(i=0;i<menu->N;i++)
      if(x >= menu->xpos[i] && x < menu->xpos[i+1])
	break;
   if(i == menu->N)
     i = menu->N -1;
    
    if(menu->submenu && menu->subindex != i)
    {
      bbx_popup2_dropmodal(menu->submenu);
      bbx_popup2_kill(menu->submenu);
      menu->submenu = 0;
    }
    if(menu->submenu == 0 && i >= 0 && i < menu->N)
    {
        menu->submenu = bbx_popup2(menu->bbx, menu->pan, menu->xpos[i], menu->height, menu->sub[i], chosen, menu);
        menu->subindex = i;
        bbx_popup2_makemodal(menu->submenu);
    }
  }
  else if(menu->submenu)
  {
    bbx_popup2_doptr(menu->submenu);
  }
   
}

static void layout(void *obj, int width, int height)
{
  MENUBAR *menu = obj;
  int i;

  for(i=0;i<menu->N;i++)
    bbx_setpos(menu->bbx, menu->lab[i], menu->xpos[i], 0, menu->xpos[i+1] - menu->xpos[i], height);

  menu->height = height;
}

static void mousefunc(void *obj, int action, int x, int y, int buttons)
{
  MENUBAR *menu = obj;
  int i;

  if(action == BBX_MOUSE_CLICK && (buttons & BBX_MOUSE_BUTTON1))
  {
   for(i=0;i<menu->N;i++)
      if(x >= menu->xpos[i] && x < menu->xpos[i+1])
      {
        menu->submenu = bbx_popup2(menu->bbx, menu->pan, menu->xpos[i], menu->height, menu->sub[i], chosen, menu);
        menu->subindex = i;
        bbx_popup2_makemodal(menu->submenu);
        break;
      }
  }
}

static void chosen(void *obj, int id)
{
  MENUBAR *menu = obj;

  if(menu->submenu)
  {
    bbx_popup2_dropmodal(menu->submenu);
    bbx_popup2_kill(menu->submenu);
    menu->submenu = 0;
  }
  if(id >= 0 && menu->fptr)
    (*menu->fptr)(menu->ptr, id);
}
