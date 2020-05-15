#include <stdlib.h>

#include "BabyX.h"

void bbx_menubar_handlepointer(BBX_Menubar *mb);


typedef struct
{
  BABYX *bbx;
  BBX_Panel *pan;
  BBX_Label **lab;
  int N;
  int labheight;
  int maxleft;
  int maxright;
  int sel;
  int answer;
  BBX_Popup *pop;
  BBX_PopUp2 *sub;
  BBX_Panel *menubar;
  int sweetx;
  void (*chosen)(void *ptr, int id);
  void *ptr;
} POPUP;

static POPUP *popup(BBX_Popup *pop, HWND parent, int x, int y);
static void popup_kill(POPUP *pp);
static void pplayout(void *obj, int width, int height);
static  void ppmousefunc(void *obj, int action, int x, int y, int buttons);

BBX_PopUp2 *bbx_popup2(BABYX *bbx, BBX_Panel *parent, int x, int y, BBX_Popup *pop, void (*chosen)(void *ptr, int id), void *ptr)
{
  POPUP *pp;

  pp = popup(pop, parent->win, x, y);
  pp->sub = 0;
  pp->menubar = parent;
  pp->chosen =  chosen;
  pp->ptr = ptr;
  pp->sweetx = 10000;

  return pp->pan; 
}

void bbx_popup2_kill(BBX_PopUp2 *pop)
{
  POPUP *pp = bbx_panel_getptr(pop);
  if(pp->sub)
    bbx_popup2_kill(pp->sub);
  popup_kill(pp);
  
}

void bbx_popup2_makemodal(BBX_PopUp2 *pop)
{
    POPUP *pp = bbx_panel_getptr(pop);
	/*
    XGrabPointer(pop->bbx->dpy, pp->pan->win, False,
		 ButtonPressMask |
                 ButtonReleaseMask |
                 PointerMotionMask 
		   // FocusChangeMask |
		   // EnterWindowMask |
		   //LeaveWindowMask,
		   ,
               GrabModeAsync,
               GrabModeAsync,
	       RootWindow(pop->bbx->dpy, DefaultScreen(pop->bbx->dpy)),
               None,
               CurrentTime);
			   */
	SetCapture(pp->pan->win);
 
   bbx_popuppanel_makemodal(pop);
}

void bbx_popup2_dropmodal(BBX_PopUp2 *pop)
{
  POPUP *pp = bbx_panel_getptr(pop);
  if (pp->sub)
	  bbx_popup2_dropmodal(pp->sub);
  else
	  ReleaseCapture();
    //XUngrabPointer(pp->bbx->dpy, CurrentTime); 
  bbx_popuppanel_dropmodal(pop);
}

void bbx_popup2_doptr(BBX_PopUp2 *pop)
{
   POPUP *pp = bbx_panel_getptr(pop);
   int x, y;
   BBX_RGBA fgcol;

   if(bbx_panel_gotmouse(pp->pan, &x, &y))
   {
     if(pp->sel !=  y / pp->labheight && pp->sub && 
        (x < pp->sweetx || y < pp->sel * pp->labheight))
     {
       bbx_popuppanel_dropmodal(pp->sub);
       bbx_popup2_kill(pp->sub);
       pp->sub = 0;
	   /*
       XGrabPointer(pop->bbx->dpy, pp->pan->win, False,
		 ButtonPressMask |
                 ButtonReleaseMask |
                 PointerMotionMask 
		   // FocusChangeMask |
		   // EnterWindowMask |
		   //LeaveWindowMask,
		   ,
               GrabModeAsync,
               GrabModeAsync,
	       RootWindow(pop->bbx->dpy, DefaultScreen(pop->bbx->dpy)),
               None,
               CurrentTime);
			   */
	   SetCapture(pp->pan->win);
 
     }
     if(x < pp->sweetx)
       pp->sweetx = 10000;
   }
   else
   {
     if(pp->sub)
       bbx_popup2_doptr(pp->sub);
     else
     {
       if(pp->sel >= 0)
       {
         if(pp->pop->menu[pp->sel].disabled == 0)
             fgcol = bbx_color("black");
         else
           fgcol = bbx_color("light gray");
         bbx_label_setbackground(pp->lab[pp->sel*2], bbx_color("gray"));
         bbx_label_setforeground(pp->lab[pp->sel*2], fgcol);
         bbx_label_setbackground(pp->lab[pp->sel*2+1], bbx_color("gray"));
         bbx_label_setforeground(pp->lab[pp->sel*2+1], fgcol);
         pp->sel = -1;
       }
     }
     pp->sweetx = 10000;
   }
}



int BBX_popuppopup(BBX_Popup *pop, HWND parent, int x, int y)
{
  POPUP *pp;
  int answer;

  pp = popup(pop, parent, x, y);
  /*
  XGrabPointer(pop->bbx->dpy, pp->pan->win, True,
		 ButtonPressMask |
                 ButtonReleaseMask |
                 PointerMotionMask 
		   // FocusChangeMask |
		   // EnterWindowMask |
		   //LeaveWindowMask,
		   ,
               GrabModeAsync,
               GrabModeAsync,
	       RootWindow(pop->bbx->dpy, DefaultScreen(pop->bbx->dpy)),
               None,
               CurrentTime);
			   */
  SetCapture(pp->pan->win);
 
  bbx_popuppanel_makemodal(pp->pan);
  answer = pp->answer;
  popup_kill(pp);
  return answer;
}

static POPUP *popup(BBX_Popup *pop, HWND parent, int x, int y)
{
  POPUP *pp;
  int maxleft = 10;
  int maxright = 10;
  int maxheight = 10;
  int pwidth, pheight;
  int i;

  pp = bbx_malloc(sizeof(POPUP));
  pp->bbx = pop->bbx;
  pp->pan = BBX_popuppanel(pop->bbx, parent, "popup", pplayout, pp, x, y);
  pp->lab = bbx_malloc(pop->N * 2 * sizeof(BBX_Label *));

  for(i=0;i<pop->N;i++)
  {
    pp->lab[i*2] = bbx_label(pop->bbx, pp->pan, pop->menu[i].left);
    pp->lab[i*2+1] = bbx_label(pop->bbx, pp->pan, pop->menu[i].right);
    if(pop->menu[i].disabled)
    {
      bbx_label_setforeground(pp->lab[i*2], bbx_color("light gray"));
      bbx_label_setforeground(pp->lab[i*2+1], bbx_color("light gray"));
    } 
  }
  pp->pop = pop;
  pp->N = pop->N;
  pp->sel = -1;
  for(i=0;i<pop->N;i++)
  {
    bbx_label_getpreferredsize(pp->lab[i*2], &pwidth, &pheight);
    if(pwidth > maxleft)
      maxleft = pwidth;
    if(pheight > maxheight)
      maxheight = pheight;
    bbx_label_getpreferredsize(pp->lab[i*2+1], &pwidth, &pheight);
    if(pwidth > maxright)
      maxright = pwidth;
    if(pheight > maxheight)
      maxheight = pheight;
  }
  pp->maxleft = maxleft;
  pp->maxright = maxright;
  pp->labheight = maxheight;
  pp->answer = -1;

  bbx_panel_setmousefunc(pp->pan, ppmousefunc, pp); 
  bbx_setsize(pp->bbx, pp->pan, maxleft + maxright + 10, maxheight * pop->N + 5);

  return pp;
} 

static void popup_kill(POPUP *pp)
{
  int i;

  if(pp)
  {
    if(pp->lab)
    {
      for(i=0;i<pp->N * 2;i++)
        bbx_label_kill(pp->lab[i]);
      free(pp->lab);
    }
    bbx_panel_kill(pp->pan);
    free(pp);
  }
}

static void pplayout(void *obj, int width, int height)
{
  POPUP *pp = obj;
  int i;

  for(i=0;i<pp->N;i++)
  {
    bbx_setpos(pp->bbx, pp->lab[i*2], 5, i * pp->labheight, pp->maxleft +5, pp->labheight);
    bbx_setpos(pp->bbx, pp->lab[i*2+1], pp->maxleft + 5, i * pp->labheight, pp->maxright, pp->labheight);  
  }

}

static void ppmousefunc(void *obj, int action, int x, int y, int buttons)
{
  POPUP *pp = obj;
  POPUP *psub;
  int sel;
  BBX_RGBA fgcol;

  /*
  if(action == BBX_MOUSE_CLICK)
  {

  if(pp->sel >= 0 && pp->pop->menu[pp->sel].sub != 0)
      pp->answer = BBX_popuppopup(pp->pop->menu[pp->sel].sub, pp->pan->win, pp->maxleft + pp->maxright, y); 
    
      XUngrabPointer(pp->bbx->dpy, CurrentTime);
      bbx_popuppanel_dropmodal(pp->pan);
    
  }
  */
  if(action == BBX_MOUSE_CLICK)
  {
    if(pp->sel >= 0 && pp->pop->menu[pp->sel].sub != 0)
    {
      pp->sub = bbx_popup2(pp->bbx, pp->pan, pp->maxleft + pp->maxright, y, pp->pop->menu[pp->sel].sub, pp->chosen, pp->ptr);
      psub = bbx_panel_getptr(pp->sub);
      psub->menubar = pp->menubar;
      bbx_popup2_makemodal(pp->sub);
    }
    else
    {
      if(bbx_panel_gotmouse(pp->pan, 0, 0))
      {
        if(pp->chosen)
          (*pp->chosen)(pp->ptr, pp->answer);
      }
      else
      {
        if(pp->chosen)
          (*pp->chosen)(pp->ptr, -1);
      }
    }

  }

  else if(action == BBX_MOUSE_MOVE)
  {
    if(!bbx_panel_gotmouse(pp->pan, 0, 0))
    {
      bbx_menubar_handlepointer(pp->menubar);
      return;
    }
    pp->sweetx = x;
    sel = y / pp->labheight;
    if(y <= 0 || sel < 0 || sel >= pp->N)
      sel = -1;
    if(sel == pp->sel)
      return;
    if(pp->sel != -1)
    {
      if(pp->pop->menu[pp->sel].disabled == 0)
        fgcol = bbx_color("black");
      else
        fgcol = bbx_color("light gray");
        bbx_label_setbackground(pp->lab[pp->sel*2], bbx_color("gray"));
        bbx_label_setforeground(pp->lab[pp->sel*2], fgcol);
        bbx_label_setbackground(pp->lab[pp->sel*2+1], bbx_color("gray"));
        bbx_label_setforeground(pp->lab[pp->sel*2+1], fgcol);
    }
    pp->sel = sel;
    if(pp->sel != -1 && pp->pop->menu[pp->sel].disabled == 0)
    {
      bbx_label_setbackground(pp->lab[pp->sel*2], bbx_color("blue"));
      bbx_label_setforeground(pp->lab[pp->sel*2], bbx_color("white"));
      bbx_label_setbackground(pp->lab[pp->sel*2+1], bbx_color("blue"));
      bbx_label_setforeground(pp->lab[pp->sel*2+1], bbx_color("white"));
    }
    if(sel == -1 || pp->pop->menu[sel].disabled)
      pp->answer = -1;
    else
      pp->answer = pp->pop->menu[sel].id;

    if(pp->sel >= 0 && pp->pop->menu[pp->sel].sub != 0)
    {
      pp->sub = bbx_popup2(pp->bbx, pp->pan, pp->maxleft + pp->maxright, y, pp->pop->menu[pp->sel].sub, pp->chosen, pp->ptr);
      psub = bbx_panel_getptr(pp->sub);
      psub->menubar = pp->menubar;
      bbx_popup2_makemodal(pp->sub);
    }
  
  }
}


typedef struct
{
  BABYX *bbx;
  BBX_Panel *pan;
  BBX_Label **lab;
  int N;
  int labheight;
  int sel;
} QPOPUP;

static QPOPUP *qpopup(BABYX *bbx, BBX_Panel *parent, int x, int y, char **str, int N);
static void qpopup_kill(QPOPUP *qp);
static void layout(void *obj, int width, int height);
static void mousefunc(void *obj, int action, int x, int y, int buttons);

int bbx_quickpopup(BABYX *bbx, BBX_Panel *parent, int x, int y, char **str, int N)
{
  QPOPUP *qp;
  int answer;

  qp = qpopup(bbx, parent, x, y, str, N);
  /*
  XGrabPointer(bbx->dpy, qp->pan->win, True,
		 ButtonPressMask |
                 ButtonReleaseMask |
                 PointerMotionMask 
		   // FocusChangeMask |
		   // EnterWindowMask |
		   //LeaveWindowMask,
		   ,
               GrabModeAsync,
               GrabModeAsync,
	       RootWindow(bbx->dpy, DefaultScreen(bbx->dpy)),
               None,
               CurrentTime);
			     */
 
  bbx_popuppanel_makemodal(qp->pan);
  answer = qp->sel;
  qpopup_kill(qp);
  return answer;  
}

static QPOPUP *qpopup(BABYX *bbx, BBX_Panel *parent, int x, int y, char **str, int N)
{
  QPOPUP *qp;
  int maxwidth = 0;
  int maxheight = 0;
  int pwidth, pheight;
  int i;

  qp = bbx_malloc(sizeof(QPOPUP));
  qp->bbx = bbx;
  qp->pan = bbx_popuppanel(bbx, parent, "qpopup", layout, qp, x, y);
  qp->lab = bbx_malloc(N * sizeof(BBX_Label *));
  for(i=0;i<N;i++)
    qp->lab[i] = bbx_label(bbx, qp->pan, str[i]);
  for(i=0;i<N;i++)
  {
    bbx_label_getpreferredsize(qp->lab[i], &pwidth, &pheight);
    if(pwidth > maxwidth)
      maxwidth = pwidth;
    if(pheight > maxheight)
      maxheight = pheight;
  } 
  qp->N = N;
  qp->labheight = maxheight;
  qp->sel = -1;
  bbx_panel_setmousefunc(qp->pan, mousefunc, qp);
  bbx_setsize(qp->bbx, qp->pan, maxwidth + 10, maxheight * N);
  return qp;
}

static void qpopup_kill(QPOPUP *qp)
{
  int i;

  if(qp)
  {
    if(qp->lab)
    {
      for(i=0;i<qp->N;i++)
        bbx_label_kill(qp->lab[i]);
    }
    free(qp->lab);
    bbx_panel_kill(qp->pan);
  }
}

static void layout(void *obj, int width, int height)
{
  int i;
  QPOPUP *qp = obj;
 
  for(i=0;i<qp->N;i++)
    bbx_setpos(qp->bbx, qp->lab[i], 0, i*qp->labheight, width, qp->labheight);
}

static void mousefunc(void *obj, int action, int x, int y, int buttons)
{
  QPOPUP *qp = obj;
  int sel;

  if(action == BBX_MOUSE_CLICK)
  {
    //XUngrabPointer(qp->bbx->dpy, CurrentTime);
    bbx_popuppanel_dropmodal(qp->pan);
  }
  else if(action == BBX_MOUSE_MOVE)
  {
    sel = y / qp->labheight;
    if(y <= 0 || sel < 0 || sel >= qp->N)
      sel = -1;
    if(sel == qp->sel)
      return;
    if(qp->sel != -1)
    {
      bbx_label_setbackground(qp->lab[qp->sel], bbx_color("white"));
      bbx_label_setforeground(qp->lab[qp->sel], bbx_color("black"));
    }
    qp->sel = sel;
    if(qp->sel != -1)
    {
      bbx_label_setbackground(qp->lab[qp->sel], bbx_color("blue"));
      bbx_label_setforeground(qp->lab[qp->sel], bbx_color("white"));
    }
  }  
}
